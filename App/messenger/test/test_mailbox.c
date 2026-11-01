/* Host test: the mailbox log on a simulated NOR flash. */
#include <stdint.h>

#include "test_util.h"
#include "flash_sim.h"
#include "../mailbox.h"

static int count_walk(void)
{
    int n = 0;
    uint32_t prev = 0xFFFFFFFFu;
    mbx_rec_t r;
    for (int16_t s = mbx_newest(); s != MBX_NONE; s = mbx_older(s)) {
        CHECK(mbx_read(s, &r));
        CHECK(r.seq < prev);           /* strictly older each step */
        prev = r.seq;
        n++;
        if (n > (int)MBX_SLOTS)
            break;
    }
    return n;
}

static void test_basic(void)
{
    sim_reset(0xFF);
    mbx_init();
    CHECK_INT(mbx_newest(), MBX_NONE);
    CHECK_INT(mbx_unread(), 0);
    CHECK_INT(mbx_count(), 0);

    int16_t a = mbx_append(MBX_DIR_IN, "DL1ABC-9", "42", "Hallo Philipp", 0);
    int16_t b = mbx_append(MBX_DIR_OUT, "DL1ABC-9", "1", "Hallo zurueck", 0);
    int16_t c = mbx_append(MBX_DIR_IN, "DL2XYZ", "", "ohne Nummer", 0);
    CHECK(a >= 0 && b >= 0 && c >= 0);
    CHECK_INT(mbx_unread(), 2);
    CHECK_INT(mbx_count(), 3);
    CHECK_INT(mbx_newest(), c);
    CHECK_INT(mbx_older(c), b);
    CHECK_INT(mbx_older(b), a);
    CHECK_INT(mbx_older(a), MBX_NONE);
    CHECK_INT(mbx_newer(a), b);
    CHECK_INT(mbx_newer(c), MBX_NONE);

    mbx_rec_t r;
    CHECK(mbx_read(a, &r));
    CHECK_STR(r.peer, "DL1ABC-9");
    CHECK_STR(r.id, "42");
    CHECK_STR(r.text, "Hallo Philipp");
    CHECK_INT(r.dir, MBX_DIR_IN);

    mbx_set_flags(a, MBX_F_READ);
    CHECK_INT(mbx_unread(), 1);
    mbx_set_flags(a, MBX_F_READ);      /* twice: no change */
    CHECK_INT(mbx_unread(), 1);

    CHECK_INT(mbx_find_outgoing("DL1ABC-9", "1"), b);
    CHECK_INT(mbx_find_outgoing("DL1ABC-9", "2"), MBX_NONE);
    mbx_set_flags(b, MBX_F_DELIVERED);
    CHECK_INT(mbx_find_outgoing("DL1ABC-9", "1"), MBX_NONE);

    mbx_set_flags(c, MBX_F_DELETED);
    CHECK_INT(mbx_unread(), 0);
    CHECK_INT(mbx_count(), 2);
    CHECK_INT(mbx_newest(), b);

    /* a reboot finds the same state */
    mbx_init();
    CHECK_INT(mbx_unread(), 0);
    CHECK_INT(mbx_count(), 2);
    CHECK_INT(mbx_newest(), b);
    CHECK(mbx_read(b, &r));
    CHECK(!(r.flags & MBX_F_DELIVERED));
    /* a record flagged encrypted keeps that flag */
    int16_t e = mbx_append(MBX_DIR_IN, "DL5ZZZ", "9", "geheim", MBX_F_ENCRYPTED);
    CHECK(mbx_read(e, &r));
    CHECK(!(r.flags & MBX_F_ENCRYPTED));
    CHECK(r.flags & MBX_F_READ);
    CHECK(mbx_read(a, &r));
    CHECK(r.flags & MBX_F_ENCRYPTED);
    CHECK_INT(sim_violations, 0);
}

static void test_power_loss(void)
{
    sim_reset(0xFF);
    mbx_init();
    int16_t a = mbx_append(MBX_DIR_IN, "DL1ABC", "1", "eins", 0);
    /* the body is written, the commit byte is lost */
    sim_drop_programs = 0;
    {
        /* simulate by programming a body by hand into the next slot */
        mbx_rec_t junk;
        memset(&junk, 0x00, sizeof junk);
        junk.commit = 0xFF;
        msg_flash_program(MBX_LOG_BASE + (uint32_t)(a + 1) * MBX_REC_SIZE, &junk, sizeof junk);
    }
    mbx_init();                         /* reboot */
    CHECK_INT(mbx_count(), 1);
    int16_t b = mbx_append(MBX_DIR_IN, "DL1ABC", "2", "zwei", 0);
    CHECK_INT(b, a + 2);                /* the cut slot is skipped */
    CHECK_INT(mbx_older(b), a);
    CHECK_INT(count_walk(), 2);

    /* both programs of an append lost: the slot stays blank */
    sim_drop_programs = 2;
    int16_t c = mbx_append(MBX_DIR_IN, "DL1ABC", "3", "drei", 0);
    CHECK(c != b + 1);                  /* written into the slot after */
    CHECK(c >= 0);
    CHECK_INT(count_walk(), 3);

    /* a corrupt record (bad CRC) is not returned */
    const uint8_t zero = 0;
    msg_flash_program(MBX_LOG_BASE + (uint32_t)c * MBX_REC_SIZE + 29u, &zero, 1);   /* inside the text */
    mbx_rec_t r;
    CHECK(!mbx_read(c, &r));
    CHECK_INT(sim_violations, 0);
}

static void test_wrap(void)
{
    sim_reset(0xFF);
    mbx_init();
    char text[32], id[8];
    int16_t last = MBX_NONE;
    const int total = (int)MBX_SLOTS * 2 + 77;
    for (int i = 0; i < total; i++) {
        snprintf(text, sizeof text, "Nachricht %d", i);
        snprintf(id, sizeof id, "%d", i % 1000);
        last = mbx_append((uint8_t)(i % 3 == 0 ? MBX_DIR_OUT : MBX_DIR_IN), "DL1ABC", id, text, 0);
        CHECK(last >= 0);
    }
    CHECK_INT(mbx_newest(), last);
    const int n = count_walk();
    CHECK(n > (int)(MBX_SLOTS - MBX_PER_SECTOR));   /* at most one sector lost */
    CHECK(n <= (int)MBX_SLOTS);
    CHECK_INT(mbx_count(), n);

    mbx_rec_t r;
    CHECK(mbx_read(last, &r));
    snprintf(text, sizeof text, "Nachricht %d", total - 1);
    CHECK_STR(r.text, text);

    /* the unread count survives a reboot */
    const uint16_t unread = mbx_unread();
    mbx_init();
    CHECK_INT(mbx_unread(), unread);
    CHECK_INT(mbx_count(), n);
    CHECK_INT(mbx_newest(), last);
    CHECK_INT(count_walk(), n);
    CHECK_INT(sim_violations, 0);

    mbx_clear();
    CHECK_INT(mbx_count(), 0);
    CHECK_INT(mbx_newest(), MBX_NONE);
}

static void test_garbage(void)
{
    /* a region with another program's data: nothing valid, and nothing of
     * it is overwritten until the user clears the mailbox */
    for (size_t i = 0; i < sizeof sim; i++)
        sim[i] = (uint8_t)(i * 37u + 11u);
    static uint8_t before[SIM_SIZE];
    memcpy(before, sim, sizeof sim);
    sim_violations = sim_erases = 0;
    mbx_init();
    CHECK(!mbx_writable());
    CHECK_INT(mbx_append(MBX_DIR_IN, "DL1ABC", "1", "neu", 0), MBX_NONE);
    mbx_set_flags(0, MBX_F_READ);
    CHECK(memcmp(before, sim, sizeof sim) == 0);
    CHECK_INT(sim_erases, 0);

    mbx_clear();                        /* the user takes the region over */
    CHECK(mbx_writable());
    int16_t s = mbx_append(MBX_DIR_IN, "DL1ABC", "1", "neu", 0);
    CHECK(s >= 0);
    mbx_rec_t r;
    CHECK(mbx_read(s, &r));
    CHECK_STR(r.text, "neu");
    CHECK_INT(sim_violations, 0);
}

static void test_ownership(void)
{
    /* one foreign sector (an F4HWN v7 firmware asset pack starts "FWA1")
     * makes the whole log read-only */
    sim_reset(0xFF);
    mbx_init();
    CHECK(mbx_writable());
    CHECK(mbx_append(MBX_DIR_IN, "DL1ABC", "1", "eins", 0) >= 0);
    const uint32_t foreign = MBX_LOG_BASE + 7u * MSG_FLASH_SECTOR;
    msg_flash_program(foreign, "FWA1", 4);
    mbx_init();
    CHECK(!mbx_writable());
    CHECK_INT(mbx_count(), 1);          /* what is ours stays readable */
    CHECK_INT(mbx_append(MBX_DIR_IN, "DL1ABC", "2", "zwei", 0), MBX_NONE);

    /* our own leftovers are fine: cut records and records of any version */
    sim_reset(0xFF);
    mbx_init();
    CHECK(mbx_append(MBX_DIR_IN, "DL1ABC", "1", "eins", 0) >= 0);
    uint8_t cut[MBX_REC_SIZE];
    memset(cut, 0x00, sizeof cut);
    cut[0] = 0xFF;                      /* everything but the commit byte */
    msg_flash_program(MBX_LOG_BASE + 3u * MBX_REC_SIZE, cut, sizeof cut);
    const uint8_t mark = MBX_MARK;
    msg_flash_program(MBX_LOG_BASE + 9u * MBX_REC_SIZE, &mark, 1);
    mbx_init();
    CHECK(mbx_writable());
    CHECK(mbx_sector_ours(MBX_LOG_BASE, MBX_MARK));
    CHECK(!mbx_sector_ours(MBX_LOG_BASE, 0xC7u));
    CHECK_INT(sim_violations, 0);
}

int main(void)
{
    test_basic();
    test_power_loss();
    test_wrap();
    test_garbage();
    test_ownership();
    return TEST_REPORT("mailbox");
}
