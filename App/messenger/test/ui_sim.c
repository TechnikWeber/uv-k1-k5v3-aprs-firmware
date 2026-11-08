/* Host UI simulator: runs the real messages screen (msg_ui.c) and messenger
 * core on the host, with the firmware's own font tables, presses keys like a
 * user would, and dumps each screen as a 128x64 bitmap (OUTDIR/NN_name.lcd:
 * the status line, then the 7 frame lines). test/render_lcd.py turns them
 * into PNG files. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "test_util.h"
#include "flash_sim.h"

#include "audio.h"
#include "driver/st7565.h"
#include "font.h"
#include "misc.h"
#include "ui/helper.h"
#include "ui/ui.h"

#include "../mailbox.h"
#include "../messenger.h"
#include "../msg_fw.h"
#include "../msg_hw.h"
#include "../msg_port.h"
#include "../msg_ui.h"

/* ------------------------------------------------ firmware stand-ins -- */

uint8_t           gStatusLine[LCD_WIDTH];
uint8_t           gFrameBuffer[FRAME_LINES][LCD_WIDTH];
bool              gUpdateDisplay;
uint8_t           gUpdateStatus;
GUI_DisplayType_t gScreenToDisplay;
BEEP_Type_t       gBeepToPlay;

void GUI_SelectNextDisplay(GUI_DisplayType_t d) { gScreenToDisplay = d; gUpdateDisplay = true; }
void GENERIC_Key_PTT(bool pressed) { (void)pressed; }
void ST7565_BlitFullScreen(void) {}
void UI_DisplayClear(void) { memset(gFrameBuffer, 0, sizeof gFrameBuffer); }

/* the same glyph code as ui/helper.c */
static void print_buffer(const char *s, uint8_t *buffer)
{
    const size_t len = strlen(s);
    for (size_t i = 0; i < len; i++) {
        if (s[i] > ' ' && s[i] < 127) {
            const unsigned index = (unsigned)(s[i] - ' ' - 1);
            const uint32_t offset = (uint32_t)(i * (FONT_SMALL_WIDTH + 1u) + 1u);
            uint16_t bit = (uint16_t)(index * FONT_SMALL_WIDTH * 7u);
            for (uint8_t column = 0; column < FONT_SMALL_WIDTH; column++) {
                const uint16_t byte = bit >> 3;
                const uint16_t packed = gFontSmallPacked[byte] | ((uint16_t)gFontSmallPacked[byte + 1u] << 8);
                if (offset + column < LCD_WIDTH * 2u)
                    buffer[offset + column] = (packed >> (bit & 7u)) & 0x7Fu;
                bit += 7u;
            }
        }
    }
}

void UI_PrintStringSmallNormal(const char *s, uint8_t start, uint8_t end, uint8_t line)
{
    const size_t len = strlen(s);
    if (end > start)
        start = (uint8_t)(start + (((end - start) - len * (FONT_SMALL_WIDTH + 1u)) + 1u) / 2u);
    print_buffer(s, gFrameBuffer[line] + start);
}

static void put_pixel(uint8_t x, uint8_t y, bool fill)
{
    if (x >= LCD_WIDTH || y >= FRAME_LINES * 8u)
        return;
    if (fill)
        gFrameBuffer[y / 8u][x] |= (uint8_t)(1u << (y % 8u));
    else
        gFrameBuffer[y / 8u][x] &= (uint8_t)~(1u << (y % 8u));
}

void GUI_DisplaySmallest(const char *s, uint8_t x, uint8_t y, bool statusbar, bool fill)
{
    (void)statusbar;
    uint8_t c;
    while ((c = (uint8_t)*s++) != '\0') {
        c = (uint8_t)(c - 0x20u);
        for (int i = 0; i < 3; ++i) {
            uint8_t pixels = gFont3x5[c][i];
            for (int j = 0; j < 6; ++j) {
                if (pixels & 1u)
                    put_pixel((uint8_t)(x + i), (uint8_t)(y + j), fill);
                pixels >>= 1;
            }
        }
        x = (uint8_t)(x + 4u);
    }
}

/* ---------------------------------------------------------- the port -- */

static uint32_t now_ms = 5000;
static uint8_t  rx_q[4][AX25_FRAME_MAX];
static uint16_t rx_len[4];
static int      rx_n;

uint32_t msg_port_ms(void) { return now_ms; }
uint32_t msg_port_random(void) { static uint32_t r = 7; r = r * 1103515245u + 12345u; return r >> 8; }
bool     msg_port_channel_clear(void) { return true; }
void     msg_port_notify(msg_notify_t kind) { (void)kind; }
static bool sim_vfo = true;        /* a VFO on the APRS frequency */
bool     msg_port_has_aprs_vfo(void) { return sim_vfo; }
bool     msg_port_aprs_rx_possible(void) { return true; }
msg_tx_result_t msg_port_transmit(const uint8_t *f, uint16_t l) { (void)f; (void)l; now_ms += 1000; return MSG_TX_OK; }

bool msg_port_take_frame(uint8_t *buf, uint16_t *len)
{
    if (!rx_n)
        return false;
    rx_n--;
    memcpy(buf, rx_q[rx_n], rx_len[rx_n]);
    *len = rx_len[rx_n];
    return true;
}

void msg_hw_diag(msg_hw_diag_t *d)
{
    memset(d, 0, sizeof *d);
    d->load_pm = 37;
    d->level_pp = 612;
}

/* ------------------------------------------------------------ driving -- */

static void rx(const char *src, const char *info)
{
    ax25_addr_t s, d, p;
    ax25_addr_parse(src, &s);
    ax25_addr_parse("APDR16", &d);
    ax25_addr_parse("WIDE1-1", &p);
    rx_len[rx_n] = ax25_build_ui(rx_q[rx_n], AX25_FRAME_MAX, &d, &s, &p, 1, info,
                                 (uint16_t)strlen(info));
    rx_n++;
}

static void run_ms(uint32_t ms)
{
    for (uint32_t t = 0; t < ms; t += 10) {
        MSG_Task();
        msg_ui_tick();
        now_ms += 10;
    }
}

static void key(KEY_Code_t k)
{
    MSG_ProcessKeys(k, true, false);
    MSG_ProcessKeys(k, false, false);
    now_ms += 150;
    run_ms(10);
}

static void key_long(KEY_Code_t k)
{
    MSG_ProcessKeys(k, true, false);
    MSG_ProcessKeys(k, true, true);
    MSG_ProcessKeys(k, false, true);
    run_ms(10);
}

/* multi-tap: digits, ' ' waits for the letter to settle */
static void type(const char *seq)
{
    for (; *seq; seq++) {
        if (*seq == ' ')
            run_ms(1100);
        else
            key((KEY_Code_t)(*seq - '0'));
    }
}

static const char *g_out;

/* 3x5 text in the status line, one pixel down, as GUI_DisplaySmallest draws it */
static void status_text(const char *s, uint8_t x)
{
    for (; *s; s++, x = (uint8_t)(x + 4u))
        for (uint8_t i = 0; i < 3u; i++)
            gStatusLine[x + i] |= (uint8_t)(gFont3x5[(uint8_t)(*s - 0x20)][i] << 1);
}

/* the status line as ui/status.c draws it here: "APRS" in the dual watch
 * slot (x 18) while the messenger listens, the envelope, the battery */
static void status_line(void)
{
    memset(gStatusLine, 0, sizeof gStatusLine);
    if (MSG_GetConfig()->enabled)
        status_text("APRS", 18);
    static const uint8_t mail[9] = {0x7E, 0x46, 0x4A, 0x52, 0x62, 0x52, 0x4A, 0x46, 0x7E};
    static const uint8_t batt[13] = {0x3E, 0x22, 0x7F, 0x41, 0x5D, 0x5D, 0x41, 0x5D, 0x5D, 0x41, 0x5D, 0x5D, 0x7F};
    if (mbx_unread())
        memcpy(gStatusLine + 104, mail, sizeof mail);
    memcpy(gStatusLine + 115, batt, sizeof batt);
}

static void shot(const char *name)
{
    MSG_DisplayScreen();
    status_line();
    char path[512];
    snprintf(path, sizeof path, "%s/%s.lcd", g_out, name);
    FILE *f = fopen(path, "wb");
    if (!f) {
        printf("cannot write %s\n", path);
        g_failed++;
        return;
    }
    fwrite(gStatusLine, 1, sizeof gStatusLine, f);
    fwrite(gFrameBuffer, 1, sizeof gFrameBuffer, f);
    fclose(f);
    g_checks++;
}

int main(int argc, char **argv)
{
    g_out = argc > 1 ? argv[1] : ".";
    sim_reset(0xFF);
    MSG_CoreInit();

    /* first start: no callsign yet */
    MSG_OpenScreen();
    type("3 555 ");                    /* D L */
    key(KEY_STAR);                     /* 123 */
    key(KEY_4);
    key(KEY_STAR);                     /* ABC */
    type("7 9 ");                      /* P W */
    shot("01_first_start");
    key(KEY_MENU);                     /* saved: on to "Messenger" */
    key(KEY_MENU);                     /* ON */
    key(KEY_DOWN);                     /* Call */
    key(KEY_DOWN);                     /* SSID: 0 -> 7 */
    for (int i = 0; i < 7; i++)
        key(KEY_MENU);
    key(KEY_DOWN);                     /* Channel */
    key(KEY_MENU);
    type("9 33 22 33 777 ");           /* W E B E R */
    key(KEY_MENU);
    shot("02_setup");
    for (int i = 0; i < 9; i++)
        key(KEY_DOWN);                 /* ... Key */
    for (int i = 0; i < 4; i++)
        key(KEY_UP);                   /* back to Freq: rows Freq to Key */
    CHECK_INT(MSG_GetConfig()->freq, 14480000u);
    shot("03_setup_radio");
    key(KEY_DOWN);                     /* APRS hold: 100 ms, the default */
    CHECK_INT(MSG_GetConfig()->hold, MSG_HOLD_100MS);
    shot("extra/setup_fast");
    key(KEY_MENU);
    key(KEY_MENU);                     /* 1 s, 2 s */
    CHECK_INT(MSG_GetConfig()->hold, MSG_HOLD_2S);

    /* history */
    MSG_Send("DL3QQQ", "On my way, will call you later");
    run_ms(4000);
    rx("DL3QQQ", ":DL4PW-7  :ack1");
    run_ms(100);
    rx("DL1ABC-9", ":DL4PW-7  :Hi Philipp, are you QRV on 145.500 tonight?{12");
    run_ms(4000);
    rx("DL5ZZZ-7", ":BLN0WEBER:Dinner at 7 pm, who is coming?");
    run_ms(4000);
    MSG_Send("DL1ABC-9", "Sure!");
    run_ms(4000);
    rx("DL1ABC-9", ":DL4PW-7  :ack2");
    run_ms(100);
    rx("DL1ABC-9", ":DL4PW-7  :Great, see you then{13");
    run_ms(4000);

    key(KEY_EXIT);
    shot("04_list");
    sim_vfo = false;
    shot("extra/no_vfo");              /* the warning line */
    sim_vfo = true;

    key(KEY_DOWN); key(KEY_DOWN);      /* the channel message */
    key(KEY_MENU);
    shot("05_read_channel");
    key(KEY_MENU);                     /* answer in the channel */
    type("222 666 88 66 8 0 6 33 0 444 66");    /* Count me in */
    run_ms(1100);
    shot("06_write_channel");
    key(KEY_MENU);
    run_ms(2000);

    key(KEY_DOWN);                     /* "Great, see you then" */
    key(KEY_1);                        /* chat with DL1ABC-9 */
    shot("07_chat");
    key(KEY_DOWN);                     /* "Sure!" */
    key(KEY_MENU);
    shot("08_read_delivered");
    key(KEY_EXIT);
    key(KEY_STAR);                     /* write in the chat: straight to the text */
    type("7777 33 33 0 999 666 88 0 7777 666 666 66");   /* See you soon */
    run_ms(1100);
    shot("09_write_private");
    key_long(KEY_EXIT);
    key(KEY_EXIT);                     /* leave the chat */

    key(KEY_STAR);                     /* new: recipient, F = channel */
    shot("10_new_to");
    key_long(KEY_EXIT);

    /* encryption, on a licence-free frequency */
    key(KEY_F);                        /* setup */
    for (int i = 0; i < 8; i++)
        key(KEY_DOWN);                 /* Freq */
    for (int i = 0; i < 6; i++)
        key(KEY_F);                    /* presets ... 446.00625 */
    for (int i = 0; i < 4; i++)
        key(KEY_DOWN);                 /* APRS hold, Voice hold, Encrypt, Key */
    key(KEY_MENU);
    type("7777 33 222 777 33 8 0 444 7777 0 7777 33 222 777 33 8 ");   /* Secret is secret */
    shot("11_key");
    key(KEY_MENU);                     /* saved */
    run_ms(2000);
    key(KEY_UP);                       /* Encrypt */
    key(KEY_MENU);                     /* ON */
    run_ms(2000);
    shot("12_setup_crypto");
    key(KEY_EXIT);
    key(KEY_STAR);
    key(KEY_F);                        /* to the channel */
    key(KEY_MENU);
    type("222 666 6 444 66 4 ");       /* Coming */
    shot("13_write_encrypted");

    /* the stations heard */
    key_long(KEY_EXIT);                /* leave the editor */
    key(KEY_2);
    shot("14_heard");
    key(KEY_EXIT);
    key(KEY_3);                        /* the monitor */
    shot("extra/monitor");
    key(KEY_EXIT);

    /* the lobby: a CQ call */
    rx("DK2AB-9", ":CQ       :Anyone QRV today? I am in Munich{4");
    run_ms(100);
    key(KEY_MENU);                     /* the newest: the CQ call */
    shot("15_read_cq");
    key(KEY_EXIT);

    /* the beacon position from a locator */
    key(KEY_F);
    for (int i = 0; i < 18; i++)
        key(KEY_DOWN);                 /* Locator */
    key(KEY_MENU);
    type("5 666 ");                    /* J O */
    key(KEY_STAR);                     /* 123 */
    key(KEY_6);
    key(KEY_2);
    key(KEY_STAR);                     /* ABC */
    type("77 6 ");                     /* Q M */
    key(KEY_MENU);                     /* saved: Position = LOCATOR */
    CHECK_STR(MSG_GetConfig()->locator, "JO62QM");
    CHECK_INT(MSG_GetConfig()->pos_mode, MSG_POS_LOCATOR);
    shot("16_setup_position");

    /* back on 144.800 (no encryption on amateur bands), an IS net */
    for (int i = 0; i < 10; i++)
        key(KEY_UP);                   /* Freq */
    key(KEY_F);                        /* next preset: 144.800 */
    key(KEY_UP);
    key(KEY_UP);                       /* IS net */
    key(KEY_MENU);
    type("44 666 8 4 ");               /* H O T G */
    key(KEY_MENU);                     /* saved */
    CHECK_STR(MSG_GetConfig()->net, "HOTG");
    shot("17_setup_net");
    key(KEY_EXIT);

    /* F in the recipient: #WEBER, #ALL, CQ, ANSRVR; the text starts "CQ HOTG " */
    key(KEY_STAR);
    for (int i = 0; i < 4; i++)
        key(KEY_F);
    key(KEY_MENU);
    type("44 33 555 555 666 ");        /* hello */
    shot("18_write_net");
    key_long(KEY_EXIT);

    /* everyone in reach: F three times gives CQ */
    key(KEY_STAR);
    for (int i = 0; i < 3; i++)
        key(KEY_F);                    /* #WEBER, #ALL, CQ */
    shot("extra/new_cq");
    key(KEY_MENU);
    type("44 444 0 2 555 555 ");       /* Hi all */
    shot("extra/write_cq");
    key_long(KEY_EXIT);

    /* an own message being sent: MENU sends the next try now, F stops */
    {
        msg_config_t c = *MSG_GetConfig();
        c.encrypt = 0;
        MSG_SetConfig(&c);
    }
    CHECK_INT(MSG_Send("DL1ABC", "Are you there?"), MSG_SEND_OK);
    run_ms(4000);
    MSG_OpenScreen();
    key(KEY_MENU);
    shot("extra/read_sending");
    key(KEY_F);
    CHECK(!MSG_SlotPending(mbx_newest(), NULL));
    key(KEY_EXIT);

    /* another program's data in the messenger's flash area */
    msg_flash_program(MBX_LOG_BASE + 9u * MSG_FLASH_SECTOR, "FWA1", 4);
    mbx_init();
    MSG_OpenScreen();
    shot("extra/flash_in_use");
    key(KEY_F);
    key(KEY_UP);                       /* wraps to Clear mailbox: "USED" */
    shot("extra/setup_used");
    key(KEY_MENU);
    shot("extra/clear_foreign");
    key(KEY_EXIT);

    /* Arrival while the empty mailbox is already open must be selectable. */
    MSG_ClearMailbox();
    MSG_OpenScreen();
    rx("DL1ABC", ":DL4PW-7  :live arrival{90");
    run_ms(100);
    CHECK_INT(mbx_unread(), 1);
    key(KEY_MENU);
    CHECK_INT(mbx_unread(), 0);
    return TEST_REPORT("ui_sim");
}
