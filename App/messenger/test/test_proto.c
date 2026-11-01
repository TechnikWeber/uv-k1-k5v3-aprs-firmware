/* Host test: AX.25 frames and APRS message parsing/formatting. */
#include <stdint.h>

#include "test_util.h"
#include "../aprs_ax25.h"
#include "../aprs_msg.h"

static uint16_t build(uint8_t *out, const char *src, const char *dst,
                      const char *const *path, uint8_t npath, const char *info)
{
    ax25_addr_t s, d, p[AX25_MAX_DIGIS];
    ax25_addr_parse(src, &s);
    ax25_addr_parse(dst, &d);
    for (uint8_t i = 0; i < npath; i++)
        ax25_addr_parse(path[i], &p[i]);
    return ax25_build_ui(out, AX25_FRAME_MAX, &d, &s, p, npath, info, (uint16_t)strlen(info));
}

static void parse_msg(const char *src, const char *info, aprs_msg_t *m)
{
    uint8_t f[AX25_FRAME_MAX];
    const char *path[] = {"WIDE1-1"};
    uint16_t n = build(f, src, "APRS", path, 1, info);
    ax25_frame_t fr;
    CHECK(n > 0);
    CHECK(ax25_parse_ui(f, n, &fr));
    aprs_msg_parse(&fr, m);
}

static void test_crc(void)
{
    /* CRC-16/X.25 check value */
    CHECK_INT(ax25_fcs((const uint8_t *)"123456789", 9), 0x906E);

    /* residue over a frame and its FCS */
    uint8_t f[AX25_FRAME_MAX];
    uint16_t n = build(f, "DL4PW-7", "APZK5M", 0, 0, ">test");
    uint16_t crc = 0xFFFF;
    for (uint16_t i = 0; i < n; i++)
        crc = ax25_crc_byte(crc, f[i]);
    CHECK_INT(crc, AX25_FCS_GOOD);
}

static void test_addr(void)
{
    ax25_addr_t a;
    char s[AX25_CALL_STR];
    CHECK(ax25_addr_parse("dl4pw-7", &a));
    CHECK_STR(a.call, "DL4PW");
    CHECK_INT(a.ssid, 7);
    ax25_addr_format(&a, s);
    CHECK_STR(s, "DL4PW-7");
    CHECK(ax25_addr_parse("DB0XYZ-15*", &a));
    CHECK(a.repeated);
    CHECK_INT(a.ssid, 15);
    CHECK(ax25_addr_parse("DL4PW", &a));
    ax25_addr_format(&a, s);
    CHECK_STR(s, "DL4PW");
    CHECK(!ax25_addr_parse("", &a));
    CHECK(!ax25_addr_parse("TOOLONG1", &a));
    CHECK(!ax25_addr_parse("DL4PW-16", &a));
    CHECK(!ax25_addr_parse("DL4PW-", &a));
    CHECK(!ax25_addr_parse("DL 4PW", &a));
}

static void test_frame(void)
{
    uint8_t f[AX25_FRAME_MAX];
    const char *path[] = {"DB0ABC-10*", "WIDE2-1"};
    uint16_t n = build(f, "DL4PW-7", "APZK5M", path, 2, ":DL1ABC   :Hallo{12");
    CHECK_INT(n, 7 * 4 + 2 + 19 + 2);
    /* destination C bit, source not, end of address on the last digi */
    CHECK_INT(f[6] & 0x80, 0x80);
    CHECK_INT(f[13] & 0x81, 0x00);
    CHECK_INT(f[27] & 0x01, 0x01);

    ax25_frame_t fr;
    CHECK(ax25_parse_ui(f, n, &fr));
    CHECK_STR(fr.src.call, "DL4PW");
    CHECK_INT(fr.src.ssid, 7);
    CHECK_STR(fr.dst.call, "APZK5M");
    CHECK_INT(fr.ndigi, 2);
    CHECK(fr.digi[0].repeated);
    CHECK(!fr.digi[1].repeated);
    CHECK_INT(fr.info_len, 19);
    CHECK(memcmp(fr.info, ":DL1ABC   :Hallo{12", 19) == 0);

    /* not UI: wrong control byte */
    f[28] = 0x13;
    CHECK(!ax25_parse_ui(f, n, &fr));

    /* too small a buffer */
    ax25_addr_t s, d;
    ax25_addr_parse("DL4PW", &s);
    ax25_addr_parse("APRS", &d);
    CHECK_INT(ax25_build_ui(f, 20, &d, &s, 0, 0, "toolong", 7), 0);
}

static void test_msg_parse(void)
{
    aprs_msg_t m;
    ax25_addr_t me;
    ax25_addr_parse("DL4PW", &me);

    parse_msg("DL1ABC-9", ":DL4PW    :Hallo Philipp{42", &m);
    CHECK_INT(m.kind, APRS_MSG_TEXT);
    CHECK_STR(m.from.call, "DL1ABC");
    CHECK_INT(m.from.ssid, 9);
    CHECK_STR(m.to, "DL4PW");
    CHECK_STR(m.text, "Hallo Philipp");
    CHECK_STR(m.id, "42");
    CHECK(aprs_msg_is_for(&m, &me));

    ax25_addr_t me7;
    ax25_addr_parse("DL4PW-7", &me7);
    CHECK(!aprs_msg_is_for(&m, &me7));

    parse_msg("DL1ABC", ":dl4pw    :klein", &m);
    CHECK(aprs_msg_is_for(&m, &me));
    CHECK_STR(m.id, "");
    CHECK_STR(m.text, "klein");

    parse_msg("DL1ABC", ":DL4PW    :ack42", &m);
    CHECK_INT(m.kind, APRS_MSG_ACK);
    CHECK_STR(m.id, "42");

    parse_msg("DL1ABC", ":DL4PW    :rej7\r\n", &m);
    CHECK_INT(m.kind, APRS_MSG_REJ);
    CHECK_STR(m.id, "7");

    parse_msg("DL1ABC", ":DL4PW    :ackAB}CD", &m);
    CHECK_INT(m.kind, APRS_MSG_ACK);
    CHECK_STR(m.id, "AB");

    parse_msg("DL1ABC", ":DL4PW    :acknowledged!", &m);
    CHECK_INT(m.kind, APRS_MSG_TEXT);
    CHECK_STR(m.text, "acknowledged!");

    parse_msg("DL1ABC", ":DL4PW    :reply ack{MM}AA", &m);
    CHECK_INT(m.kind, APRS_MSG_TEXT);
    CHECK_STR(m.text, "reply ack");
    CHECK_STR(m.id, "MM");
    CHECK_STR(m.reply_ack, "AA");

    parse_msg("DL1ABC", ":DL4PW    :a {brace} inside", &m);
    CHECK_STR(m.text, "a {brace} inside");
    CHECK_STR(m.id, "");

    /* UTF-8 umlauts (APRSdroid), Latin-1, emoji */
    parse_msg("DL1ABC", ":DL4PW    :Gr\xC3\xBC\xC3\x9F" "e \xE4 \xF0\x9F\x98\x80!", &m);
    CHECK_STR(m.text, "Gruesse ae ?!");

    /* positions and status are no messages */
    parse_msg("DL1ABC", "!4850.90N/00216.25E-test", &m);
    CHECK_INT(m.kind, APRS_MSG_NONE);
    parse_msg("DL1ABC", ">status", &m);
    CHECK_INT(m.kind, APRS_MSG_NONE);
    parse_msg("DL1ABC", ":SHORT:x", &m);
    CHECK_INT(m.kind, APRS_MSG_NONE);

    /* third party, gated from APRS-IS by an iGate */
    parse_msg("DB0XYZ-10", "}DL2XYZ-5>APDR16,TCPIP,DB0XYZ-10*::DL4PW    :via Internet{a1", &m);
    CHECK_INT(m.kind, APRS_MSG_TEXT);
    CHECK(m.third_party);
    CHECK_STR(m.from.call, "DL2XYZ");
    CHECK_INT(m.from.ssid, 5);
    CHECK_STR(m.text, "via Internet");
    CHECK_STR(m.id, "a1");

    parse_msg("DB0XYZ-10", "}broken", &m);
    CHECK_INT(m.kind, APRS_MSG_NONE);
}

static void test_msg_format(void)
{
    char out[120];
    uint16_t n = aprs_msg_format_text(out, sizeof out, "dl1abc-9", "Hi|there{x", "12");
    out[n] = 0;
    CHECK_STR(out, ":DL1ABC-9 :Hi-there-x{12");

    n = aprs_msg_format_text(out, sizeof out, "DL1ABC", "no id", 0);
    out[n] = 0;
    CHECK_STR(out, ":DL1ABC   :no id");

    n = aprs_msg_format_ack(out, sizeof out, "DL1ABC-9", "42", false);
    out[n] = 0;
    CHECK_STR(out, ":DL1ABC-9 :ack42");

    n = aprs_msg_format_ack(out, sizeof out, "DL1ABC", "7", true);
    out[n] = 0;
    CHECK_STR(out, ":DL1ABC   :rej7");

    n = aprs_msg_format_status(out, sizeof out, "QRV");
    out[n] = 0;
    CHECK_STR(out, ">QRV");

    char text[APRS_TEXT_MAX + 2];
    memset(text, 'x', sizeof text - 1);
    text[sizeof text - 1] = 0;           /* 68 characters: too long */
    CHECK_INT(aprs_msg_format_text(out, sizeof out, "DL1ABC", text, "1"), 0);
    text[APRS_TEXT_MAX] = 0;             /* 67: fits */
    CHECK_INT(aprs_msg_format_text(out, sizeof out, "DL1ABC", text, "1"), 11 + 67 + 2);
}

static void test_position(void)
{
    char out[64];
    /* 48 50.90 N, 2 16.25 E: the APRS TX app's default */
    uint16_t n = aprs_msg_format_position(out, sizeof out, 48 * 6000 + 5090, 2 * 6000 + 1625,
                                          "/[", "QRV");
    out[n] = 0;
    CHECK_STR(out, "=4850.90N/00216.25E[QRV");
    /* south and west, three-digit longitude, no comment */
    n = aprs_msg_format_position(out, sizeof out, -(33 * 6000 + 5212), -(70 * 6000 + 3999),
                                 "/-", "");
    out[n] = 0;
    CHECK_STR(out, "=3352.12S/07039.99W-");
    n = aprs_msg_format_position(out, sizeof out, 90 * 6000, -180 * 6000, "/>", "");
    out[n] = 0;
    CHECK_STR(out, "=9000.00N/18000.00W>");
    CHECK_INT(aprs_msg_format_position(out, sizeof out, 90 * 6000 + 1, 0, "/[", ""), 0);
    CHECK_INT(aprs_msg_format_position(out, sizeof out, 0, 0, "[", ""), 0);
    CHECK_INT(aprs_msg_format_position(out, 20, 0, 0, "/[", "x"), 0);   /* does not fit */

    /* Maidenhead: the centre of the square */
    int32_t lat, lon;
    CHECK(aprs_locator_to_pos("JO62QM", &lat, &lon));      /* Berlin */
    CHECK_INT(lat, 52 * 6000 + 3125);                       /* 52 31.25 N */
    CHECK_INT(lon, 13 * 6000 + 2250);                       /* 13 22.50 E */
    CHECK(aprs_locator_to_pos("jo62qm", &lat, &lon));      /* any case */
    CHECK_INT(lon, 13 * 6000 + 2250);
    CHECK(aprs_locator_to_pos("JO62", &lat, &lon));
    CHECK_INT(lat, 52 * 6000 + 3000);                       /* 52 30 N */
    CHECK_INT(lon, 13 * 6000);                              /* 13 00 E */
    CHECK(aprs_locator_to_pos("JO62QM15", &lat, &lon));
    CHECK_INT(lat, 52 * 6000 + 3000 + 5 * 25 + 12);         /* 52 31.37 N */
    CHECK_INT(lon, 13 * 6000 + 2000 + 50 + 25);             /* 13 20.75 E */
    CHECK(aprs_locator_to_pos("AA00AA", &lat, &lon));      /* the south-west corner */
    CHECK_INT(lat, -90 * 6000 + 125);
    CHECK_INT(lon, -180 * 6000 + 250);
    CHECK(aprs_locator_to_pos("RR99XX", &lat, &lon));      /* the north-east corner */
    CHECK_INT(lat, 90 * 6000 - 125);
    CHECK_INT(lon, 180 * 6000 - 250);
    CHECK(aprs_locator_to_pos("FF44AA", &lat, &lon));      /* south and west */
    CHECK(lat < 0 && lon < 0);
    CHECK(!aprs_locator_to_pos("", &lat, &lon));
    CHECK(!aprs_locator_to_pos("JO6", &lat, &lon));
    CHECK(!aprs_locator_to_pos("SO62QM", &lat, &lon));     /* fields stop at R */
    CHECK(!aprs_locator_to_pos("JO62YM", &lat, &lon));     /* subsquares stop at X */
    CHECK(!aprs_locator_to_pos("JOA2QM", &lat, &lon));
    CHECK(!aprs_locator_to_pos("JO62QM1", &lat, &lon));
    CHECK(!aprs_locator_to_pos("JO62QM1A", &lat, &lon));
}

int main(void)
{
    test_crc();
    test_addr();
    test_frame();
    test_msg_parse();
    test_msg_format();
    test_position();
    return TEST_REPORT("proto");
}
