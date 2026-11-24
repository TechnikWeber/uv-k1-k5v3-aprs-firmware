/* Copyright 2026 Philipp Weber (DL4PW)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *     Unless required by applicable law or agreed to in writing, software
 *     distributed under the License is distributed on an "AS IS" BASIS,
 *     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *     See the License for the specific language governing permissions and
 *     limitations under the License.
 */

#include <stdint.h>
#include <string.h>

#include "app/generic.h"
#include "audio.h"
#include "driver/st7565.h"
#include "fw_assets.h"
#include "misc.h"
#include "ui/helper.h"
#include "ui/ui.h"

#include "mailbox.h"
#include "messenger.h"
#include "msg_fw.h"
#include "msg_hw.h"
#include "msg_port.h"
#include "msg_ui.h"
#include "t9.h"

#define ROW_CHARS   18u        /* small font: 7 px per character */
#define LIST_ROWS   5u         /* lines 1-5 */
#define POPUP_MS    1500u
#define THREAD_LEN  (AX25_CALL_STR + 1u)

typedef enum {
    V_LIST = 0,
    V_READ,
    V_TO,
    V_TEXT,
    V_SETUP,
    V_EDIT_CALL,
    V_EDIT_GROUP,
    V_EDIT_STATUS,
    V_EDIT_FREQ,
    V_EDIT_KEY,
    V_EDIT_NET,
    V_EDIT_LOC,
    V_EDIT_LAT,
    V_EDIT_LON,
    V_CONFIRM,
    V_HEARD,
    V_MONITOR,
} view_t;

typedef enum { CONFIRM_DELETE = 0, CONFIRM_CLEAR } confirm_t;

enum {
    S_ENABLE = 0, S_CALL, S_SSID, S_CHANNEL, S_PUBLIC, S_LOBBY, S_NET, S_BEEP, S_FREQ, S_HOLD,
    S_VOICE, S_ENCRYPT, S_KEY, S_PATH, S_BEACON, S_BEACON_NOW, S_STATUS, S_POS, S_LOCATOR, S_LAT, S_LON,
    S_SYMBOL, S_AUDIO, S_CLEAR,
    S_COUNT
};

/* ----------------------------------------------------------- texts -- */

/* The display texts live in the firmware assets (external flash, see
 * fw_assets.h), as v7 keeps its own menu texts there. A("...") puts a text
 * there and gives its address; AT() marks an entry of an asset table. Neither
 * may be read directly: the drawing helpers (print, title, tiny, popup,
 * label_value) and acpy / acat take asset texts and plain strings alike.
 * Texts that become data (recipients, message prefixes, compared strings)
 * stay plain, because without the asset pack every asset text reads empty.
 * In the host simulator (MSG_SIM_ASSETS) an asset text is a tagged pointer,
 * so a direct read crashes the simulator instead of a radio. */
#define TXT_MAX 40u                    /* the longest text + 1 */
#define TXT_CHECK(s) _Static_assert(sizeof(s) <= TXT_MAX, "text too long, raise TXT_MAX")

#if defined(ENABLE_FEAT_F4HWN_FW_ASSETS)
extern const uint8_t __fw_assets_start[];
extern const uint8_t __fw_assets_end[];
#define A(s)  ({ TXT_CHECK(s); static const char a_[] FW_ASSET = s; (const char *)a_; })
#define AT(p) ((const char *)(p))
static inline bool is_asset(const char *s)
{
    return (uintptr_t)s >= (uintptr_t)__fw_assets_start && (uintptr_t)s < (uintptr_t)__fw_assets_end;
}
static inline void asset_copy(char *dst, const char *s)
{
    FW_ASSETS_ReadStr(dst, s, TXT_MAX);
}
#elif defined(MSG_SIM_ASSETS)
_Static_assert(sizeof(uintptr_t) == 8u, "the asset trap needs a 64-bit host");
#define SIM_TAG ((uintptr_t)0xF0u << 48)  /* a non-canonical address, never mapped */
#define A(s)  ({ TXT_CHECK(s); static const char a_[] = s; (const char *)((uintptr_t)a_ | SIM_TAG); })
#define AT(p) ((const char *)((uintptr_t)(p) | SIM_TAG))
static inline bool is_asset(const char *s)
{
    return ((uintptr_t)s & SIM_TAG) != 0;
}
static inline void asset_copy(char *dst, const char *s)
{
    strncpy(dst, (const char *)((uintptr_t)s & ~SIM_TAG), TXT_MAX - 1u);
    dst[TXT_MAX - 1u] = '\0';
}
#else
#define A(s)  ({ TXT_CHECK(s); (const char *)(s); })
#define AT(p) ((const char *)(p))
static inline bool is_asset(const char *s)
{
    (void)s;
    return false;
}
static inline void asset_copy(char *dst, const char *s)
{
    strcpy(dst, s);
}
#endif

/* s itself, or its copy in buf (TXT_MAX bytes) when s is an asset text */
static const char *txt(const char *s, char *buf)
{
    if (!is_asset(s))
        return s;
    asset_copy(buf, s);
    return buf;
}

/* strcpy and strcat that also take asset texts */
static void acpy(char *dst, const char *s)
{
    char b[TXT_MAX];
    strcpy(dst, txt(s, b));
}

static void acat(char *dst, const char *s)
{
    char b[TXT_MAX];
    strcat(dst, txt(s, b));
}

/* the short texts used in several places */
static const char T_ON[] FW_ASSET = "ON";
static const char T_OFF[] FW_ASSET = "OFF";
static const char T_NONE[] FW_ASSET = "none";
static const char T_TO[] FW_ASSET = "To ";
static const char T_NOTHING[] FW_ASSET = "Nothing heard yet";
static const char T_BAD_CALL[] FW_ASSET = "Bad callsign";
static const char T_DIGITS[] FW_ASSET = "Letters, digits";
static const char T_NO_NET[] FW_ASSET = "Set IS net first";
static const char T_NO_KEY[] FW_ASSET = "Set a key first";
static const char T_SET_CALL[] FW_ASSET = "Set your call";
static const char T_CANNOT[] FW_ASSET = "Cannot send";

static const char SETUP_LABEL[S_COUNT][14] FW_ASSET = {
    "Messenger", "Call", "SSID", "Channel", "Public BLN", "Lobby CQ", "IS net", "Beep", "Freq",
    "APRS hold", "Voice hold", "Encrypt", "Key", "Path", "Beacon", "Beacon now", "Status", "Position",
    "Locator", "Lat", "Lon", "Symbol", "APRS audio", "Clear mailbox",
};

/* APRS in IARU Region 1, North America, China, Japan, Australia, the ISS;
 * PMR446 channel 1 for licence-free use (rules: see the README) */
static const uint32_t FREQS[] = {14480000u, 14439000u, 14464000u, 14466000u, 14517500u, 14582500u,
                                 44600625u};
#define FREQ_COUNT (sizeof FREQS / sizeof FREQS[0])

static const char PATH_TXT[MSG_PATH_COUNT][8] FW_ASSET = {"DIRECT", "WIDE1", "WIDE1+2", "ARISS"};
static const char BEEP_TXT[MSG_BEEP_COUNT][8] FW_ASSET = {"OFF", "ALL", "PRIVATE"};
static const char POS_TXT[MSG_POS_COUNT][8] FW_ASSET = {"OFF", "LOCATOR", "FIXED"};
/* "(!)": experimental; label + space + value fill the 18 characters of a row */
static const char HOLD_TXT[MSG_HOLD_COUNT][7] FW_ASSET = {"100 ms", "1 s", "2 s", "5 s", "10 s",
                                                          "ALWAYS"};
static const char VOICE_TXT[MSG_VOICE_COUNT][5] FW_ASSET = {"4 s", "10 s", "20 s", "60 s"};

static view_t    gView;
static view_t    gBack;          /* where a confirm returns to                */
static confirm_t gConfirm;
static int16_t   gSel = MBX_NONE, gTop = MBX_NONE;
static char      gThread[THREAD_LEN];   /* chat filter: "DL1ABC-9" or "#GROUP", "" = all */
static uint8_t   gReadTop;       /* first text row shown in V_READ            */
static uint8_t   gSetupSel, gSetupTop;
static t9_t      gEdit;
static char      gTo[THREAD_LEN];       /* recipient: a call, or "#GROUP" / "#ALL" */
static char      gPopup[20];
static uint32_t  gPopupUntil;
static uint8_t   gHeardSel, gHeardTop;

/* ---------------------------------------------------------- drawing -- */

static void print(const char *s, uint8_t x, uint8_t line)
{
    char b[TXT_MAX];
    char row[ROW_CHARS + 1];
    strncpy(row, txt(s, b), ROW_CHARS);
    row[ROW_CHARS] = '\0';
    UI_PrintStringSmallNormal(row, x, 0, line);
}

static void invert_line(uint8_t line)
{
    for (uint8_t x = 0; x < LCD_WIDTH; x++)
        gFrameBuffer[line][x] ^= 0xFFu;
}

static void tiny(const char *s, uint8_t x, uint8_t line)
{
    char b[TXT_MAX];
    GUI_DisplaySmallest(txt(s, b), x, (uint8_t)(line * 8u + 1u), false, true);
}

static void title(const char *s)
{
    print(s, 0, 0);
    invert_line(0);
}

/* "label ..... value" in one row */
static void label_value(uint8_t line, const char *label, const char *value)
{
    char lb[TXT_MAX], vb[TXT_MAX];
    label = txt(label, lb);
    value = txt(value, vb);
    char row[ROW_CHARS + 1];
    memset(row, ' ', ROW_CHARS);
    row[ROW_CHARS] = '\0';
    const size_t ll = strlen(label);
    memcpy(row, label, ll < ROW_CHARS ? ll : ROW_CHARS);
    size_t vl = strlen(value);
    if (vl > ROW_CHARS - 1u)
        vl = ROW_CHARS - 1u;
    memcpy(row + ROW_CHARS - vl, value, vl);
    print(row, 0, line);
}

static char *put_u(char *o, uint32_t v)
{
    char rev[10];
    uint8_t n = 0;
    do {
        rev[n++] = (char)('0' + v % 10u);
        v /= 10u;
    } while (v);
    while (n)
        *o++ = rev[--n];
    *o = '\0';
    return o;
}

/* 10 Hz units -> "144.800" or "446.00625" */
static void freq_text(uint32_t f, char *out)
{
    char *o = put_u(out, f / 100000u);
    *o++ = '.';
    uint32_t k = f % 100000u;
    uint8_t digits = 5;
    while (digits > 3u && k % 10u == 0u) {
        k /= 10u;
        digits--;
    }
    for (uint8_t d = digits; d; d--) {
        uint32_t p = 1;
        for (uint8_t i = 1; i < d; i++)
            p *= 10u;
        *o++ = (char)('0' + (k / p) % 10u);
    }
    *o = '\0';
}

static void popup(const char *text)
{
    char b[TXT_MAX];
    strncpy(gPopup, txt(text, b), sizeof gPopup - 1u);
    gPopup[sizeof gPopup - 1u] = '\0';
    gPopupUntil = msg_port_ms() + POPUP_MS;
    if (gPopupUntil == 0)
        gPopupUntil = 1;
    gUpdateDisplay = true;
}

/* a dark box over lines 2-4 with the text inverted, a light margin around */
static void draw_popup(void)
{
    for (uint8_t line = 2; line <= 4; line++)
        memset(gFrameBuffer[line], 0, LCD_WIDTH);
    const uint8_t len = (uint8_t)strlen(gPopup);
    const uint8_t x = (uint8_t)((LCD_WIDTH - len * 7u) / 2u);
    print(gPopup, x, 3);
    for (uint8_t xx = 6; xx < LCD_WIDTH - 6u; xx++) {
        gFrameBuffer[2][xx] = 0xE0u;
        gFrameBuffer[3][xx] ^= 0xFFu;
        gFrameBuffer[4][xx] = 0x07u;
    }
}

/* --------------------------------------------------------- records ---- */

static bool group_in(uint8_t dir)  { return dir == MBX_DIR_BC_IN || dir == MBX_DIR_CQ_IN; }
static bool group_out(uint8_t dir) { return dir == MBX_DIR_BC_OUT || dir == MBX_DIR_CQ_OUT; }

/* The conversation a record belongs to: the other station, "#GROUP" for a
 * channel, "#ALL" for public bulletins, "CQ" / "QST" / "ALL" for the lobby. */
static void thread_of(const mbx_rec_t *r, char *out)
{
    if (r->dir == MBX_DIR_CQ_IN || r->dir == MBX_DIR_CQ_OUT) {
        strcpy(out, r->id);
    } else if (r->dir == MBX_DIR_BC_IN || r->dir == MBX_DIR_BC_OUT) {
        out[0] = '#';
        strcpy(out + 1, r->id[0] ? r->id : "ALL");
    } else {
        strcpy(out, r->peer);
    }
}

static bool matches(int16_t slot)
{
    if (!gThread[0])
        return true;
    mbx_rec_t r;
    char t[THREAD_LEN];
    if (!mbx_read(slot, &r))
        return false;
    thread_of(&r, t);
    return strcmp(t, gThread) == 0;
}

static int16_t older(int16_t s)
{
    do s = mbx_older(s); while (s != MBX_NONE && !matches(s));
    return s;
}

static int16_t newer(int16_t s)
{
    do s = mbx_newer(s); while (s != MBX_NONE && !matches(s));
    return s;
}

static int16_t newest(void)
{
    int16_t s = mbx_newest();
    return (s == MBX_NONE || matches(s)) ? s : older(s);
}

/* ----------------------------------------------------------- list ---- */

static char mark_of(int16_t slot, const mbx_rec_t *r)
{
    if (r->dir == MBX_DIR_IN || group_in(r->dir))
        return (r->flags & MBX_F_READ) ? '*' : ' ';   /* bit still set: unread */
    if (MSG_SlotPending(slot, NULL))
        return '>';
    if (group_out(r->dir))
        return ' ';
    if (!(r->flags & MBX_F_DELIVERED))
        return '+';
    if (!(r->flags & MBX_F_FAILED))
        return '!';
    return '-';
}

static void draw_list(void)
{
    char head[ROW_CHARS + 1];
    if (gThread[0]) {
        acpy(head, A("Chat "));
        strcat(head, gThread);
    } else {
        char *o = head;
        acpy(o, A("MESSAGES"));
        o += 8;
        const uint16_t unread = mbx_unread();
        if (unread) {
            *o++ = ' ';
            o = put_u(o, unread);
            acpy(o, A(" new"));
        } else {
            *o = '\0';
        }
    }
    title(head);

    const msg_config_t *cfg = MSG_GetConfig();
    if (gTop == MBX_NONE) {
        print(cfg->enabled ? A("No messages yet") : A("Messenger is off"), 0, 2);
        print(A("* write  F setup"), 0, 4);
    } else {
        int16_t s = gTop;
        for (uint8_t row = 0; row < LIST_ROWS && s != MBX_NONE; row++, s = older(s)) {
            mbx_rec_t r;
            char line[ROW_CHARS + 1];
            if (!mbx_read(s, &r)) {
                acpy(line, A(" (damaged)"));
            } else {
                /* mark, who (9), text; broadcasts: the text starts with '#',
                 * the ones sent show the channel instead of a call */
                char who[THREAD_LEN];
                if (group_out(r.dir))
                    thread_of(&r, who);
                else
                    strcpy(who, r.peer);
                memset(line, ' ', ROW_CHARS);
                line[0] = mark_of(s, &r);
                const size_t pl = strlen(who);
                memcpy(line + 1, who, pl > 9u ? 9u : pl);
                uint8_t x = 11;
                if (group_in(r.dir))
                    line[x++] = '#';
                const size_t tl = strlen(r.text);
                const size_t room = ROW_CHARS - x;
                memcpy(line + x, r.text, tl < room ? tl : room);
                line[ROW_CHARS] = '\0';
            }
            print(line, 0, (uint8_t)(1u + row));
            if (s == gSel)
                invert_line((uint8_t)(1u + row));
        }
    }

    if (!MSG_FlashOk()) {
        tiny(A("FLASH IN USE: CLEAR MAILBOX"), 0, 6);
    } else if (cfg->enabled && !msg_port_has_aprs_vfo()) {
        char warn[32];
        acpy(warn, A("NO VFO ON "));
        freq_text(cfg->freq, warn + strlen(warn));
        tiny(warn, 0, 6);
    } else if (cfg->enabled && !msg_port_aprs_rx_possible()) {
        tiny(A("SELECT APRS VFO OR DUAL WATCH"), 0, 6);
    } else if (MSG_LastTxResult() == MSG_TX_LOCKED) {
        tiny(A("TX BLOCKED: CHECK FM/SIMPLEX/LOCK"), 0, 6);
    } else if (MSG_LastTxResult() == MSG_TX_BATTERY) {
        tiny(A("TX BLOCKED: BATTERY"), 0, 6);
    } else if (gThread[0]) {
        tiny(A("MENU READ  * WRITE  EXIT ALL"), 0, 6);
    } else {
        tiny(A("* NEW  1 CHAT  2 HEARD  F SETUP"), 0, 6);
    }
}

static void list_reset(void)
{
    gSel = gTop = newest();
}

static void list_down(void)
{
    if (gSel == MBX_NONE)
        return;
    const int16_t next = older(gSel);
    if (next == MBX_NONE)
        return;
    /* is gSel the last visible row? then scroll */
    int16_t s = gTop;
    uint8_t row = 0;
    while (s != MBX_NONE && s != gSel && row < LIST_ROWS) {
        s = older(s);
        row++;
    }
    if (row >= LIST_ROWS - 1u)
        gTop = older(gTop);
    gSel = next;
}

static void list_up(void)
{
    if (gSel == MBX_NONE)
        return;
    const int16_t prev = newer(gSel);
    if (prev == MBX_NONE)
        return;
    if (gSel == gTop)
        gTop = prev;
    gSel = prev;
}

/* ----------------------------------------------------------- read ---- */

/* word wrap: start offsets of up to 8 rows */
static uint8_t wrap(const char *t, uint8_t starts[8])
{
    const uint8_t len = (uint8_t)strlen(t);
    uint8_t n = 0, pos = 0;
    while (pos < len && n < 8u) {
        starts[n++] = pos;
        if ((uint8_t)(len - pos) <= ROW_CHARS) {
            pos = len;
            break;
        }
        uint8_t cut = (uint8_t)(pos + ROW_CHARS);
        uint8_t sp = cut;
        while (sp > pos && t[sp] != ' ')
            sp--;
        pos = (sp > pos) ? (uint8_t)(sp + 1u) : cut;
    }
    return n;
}

static void draw_read(void)
{
    mbx_rec_t r;
    if (!mbx_read(gSel, &r)) {
        title(A("MESSAGE"));
        print(A("(damaged)"), 0, 2);
        return;
    }
    char head[ROW_CHARS + 8];
    char chan[THREAD_LEN];
    thread_of(&r, chan);
    switch (r.dir) {
    case MBX_DIR_IN:    acpy(head, A("From ")); strcat(head, r.peer); break;
    case MBX_DIR_OUT:   acpy(head, AT(T_TO));  strcat(head, r.peer); break;
    case MBX_DIR_BC_IN:
    case MBX_DIR_CQ_IN: strcpy(head, r.peer);  strcat(head, " "); strcat(head, chan); break;
    default:            acpy(head, AT(T_TO));  strcat(head, chan); break;
    }
    title(head);

    uint8_t starts[8];
    const uint8_t rows = wrap(r.text, starts);
    if (gReadTop + LIST_ROWS > rows)
        gReadTop = rows > LIST_ROWS ? (uint8_t)(rows - LIST_ROWS) : 0u;
    for (uint8_t i = 0; i < LIST_ROWS && gReadTop + i < rows; i++) {
        const uint8_t k = (uint8_t)(gReadTop + i);
        const uint8_t end = (k + 1u < rows) ? starts[k + 1] : (uint8_t)strlen(r.text);
        char line[ROW_CHARS + 1];
        uint8_t n = (uint8_t)(end - starts[k]);
        if (n > ROW_CHARS)
            n = ROW_CHARS;
        memcpy(line, r.text + starts[k], n);
        while (n && line[n - 1u] == ' ')
            n--;
        line[n] = '\0';
        print(line, 0, (uint8_t)(1u + i));
    }

    char st[48];
    uint8_t tries;
    switch (r.dir) {
    case MBX_DIR_IN:
        acpy(st, A("MENU REPLY   0 DELETE"));
        break;
    case MBX_DIR_BC_IN:
    case MBX_DIR_CQ_IN:
        acpy(st, A("MENU GROUP F PRIVATE 0 DEL"));
        break;
    default:
        if (MSG_SlotPending(gSel, &tries)) {
            const bool bc = group_out(r.dir);
            char *o = st;
            acpy(o, A("SENDING "));
            o = put_u(o + 8, tries);
            *o++ = '/';
            o = put_u(o, bc ? MSG_BCAST_SENDS : MSG_RETRIES);
            acpy(o, bc ? A(" F STOP") : A(" MENU NOW F STOP"));
        } else if (group_out(r.dir)) {
            acpy(st, A("SENT, NO ACK  0 DELETE"));
        } else if (!(r.flags & MBX_F_DELIVERED)) {
            acpy(st, A("DELIVERED    0 DELETE"));
        } else if (!(r.flags & MBX_F_FAILED)) {
            acpy(st, A("NO ACK  MENU RESEND"));
        } else {
            acpy(st, A("NOT CONFIRMED MENU RESEND"));
        }
        break;
    }
    if (!(r.flags & MBX_F_ENCRYPTED)) {           /* "ENC" in front of the hints */
        memmove(st + 4, st, strlen(st) + 1u);
        memcpy(st, "ENC ", 4);
        st[32] = '\0';
    }
    tiny(st, 0, 6);
}

/* ------------------------------------------------------- compose ---- */

/* encrypted text must stay shorter to fit an APRS message */
static uint8_t text_max(void)
{
    return MSG_EncryptActive() ? MC_PLAIN_MAX : APRS_TEXT_MAX;
}

/* the text editor for gTo; a message to ANSRVR starts with "CQ <net> ",
 * which posts it to the IS net */
static void open_text(void)
{
    const msg_config_t *c = MSG_GetConfig();
    char pre[4 + MSG_NET_LEN + 1] = "";
    if (c->net[0] && strcmp(gTo, MSG_NET_SERVER) == 0) {
        strcpy(pre, "CQ ");
        strcat(pre, c->net);
        strcat(pre, " ");
    }
    t9_init(&gEdit, text_max(), false, pre);
    gView = V_TEXT;
}

/* F in the recipient field: the own channel, the public bulletins, the lobby
 * (CQ) and the IS net (ANSRVR) in turn */
static void next_target(void)
{
    const msg_config_t *c = MSG_GetConfig();
    char t[4][THREAD_LEN];
    uint8_t n = 0;
    if (c->group[0]) {
        t[n][0] = '#';
        strcpy(t[n++] + 1, c->group);
    }
    strcpy(t[n++], "#ALL");
    strcpy(t[n++], "CQ");
    if (c->net[0])
        strcpy(t[n++], MSG_NET_SERVER);
    t9_commit(&gEdit);
    uint8_t i = 0;
    while (i < n && strcmp(gEdit.buf, t[i]) != 0)
        i++;
    t9_init(&gEdit, AX25_CALL_STR - 1u, true, t[i < n ? (i + 1u) % n : 0u]);
}

static void open_compose(const char *to)
{
    if (to && to[0]) {
        strncpy(gTo, to, sizeof gTo - 1u);
        gTo[sizeof gTo - 1u] = '\0';
        open_text();
    } else {
        gTo[0] = '\0';
        t9_init(&gEdit, AX25_CALL_STR - 1u, true, NULL);
        gView = V_TO;
    }
}

/* the editor text with the pending letter, char-wrapped from line first */
static void draw_editor(uint8_t first, uint8_t rows)
{
    char text[T9_MAX + 2];
    const char pend = t9_pending(&gEdit);
    memcpy(text, gEdit.buf, gEdit.cursor);
    uint8_t n = gEdit.cursor;
    if (pend)
        text[n++] = pend;
    memcpy(text + n, gEdit.buf + gEdit.cursor, (size_t)(gEdit.len - gEdit.cursor));
    n = (uint8_t)(n + gEdit.len - gEdit.cursor);
    text[n] = '\0';

    /* scroll so the cursor row is visible */
    const uint8_t crow = (uint8_t)(gEdit.cursor / ROW_CHARS);
    const uint8_t top = crow >= rows ? (uint8_t)(crow - rows + 1u) : 0u;
    for (uint8_t i = 0; i < rows; i++) {
        const uint16_t off = (uint16_t)((top + i) * ROW_CHARS);
        if (off > n)
            break;
        print(text + off, 0, (uint8_t)(first + i));
    }
    /* the cursor: the pending letter inverted, or a bar under the position */
    const uint8_t line = (uint8_t)(first + crow - top);
    const uint8_t x = (uint8_t)((gEdit.cursor % ROW_CHARS) * 7u);
    for (uint8_t k = 0; k < 7u && x + k < LCD_WIDTH; k++)
        gFrameBuffer[line][x + k] ^= pend ? 0x7Fu : 0x80u;
}

static void draw_compose(void)
{
    char head[ROW_CHARS + 1];
    char hint[40];
    if (gView == V_TO) {
        title(A("NEW MESSAGE"));
        print(A("To (callsign):"), 0, 2);
        draw_editor(3, 1);
        /* the tiny font has no clear '#': spell the targets out */
        tiny(MSG_GetConfig()->net[0] ? A("F = CHANNEL, ALL, CQ, NET") : A("F = CHANNEL, ALL, CQ"), 0, 5);
        acpy(hint, A("MENU NEXT  * "));
        strcat(hint, t9_mode_name(&gEdit));
        tiny(hint, 0, 6);
        return;
    }
    acpy(head, AT(T_TO));
    strcat(head, gTo);
    title(head);
    draw_editor(1, 4);
    char *o = hint;
    strcpy(o, t9_mode_name(&gEdit));
    o += strlen(o);
    *o++ = ' ';
    o = put_u(o, gEdit.len);
    *o++ = '/';
    o = put_u(o, gEdit.max);
    acpy(o, MSG_EncryptActive() ? A(" ENC  MENU SEND") : A("   MENU SEND"));
    tiny(hint, 0, 6);
}

static void send_now(void)
{
    t9_commit(&gEdit);
    static const char ERR[][17] FW_ASSET = {
        [MSG_SEND_OK]          = "Queued",
        [MSG_SEND_NO_CALL]     = "Set your call",
        [MSG_SEND_BAD_CALL]    = "Bad callsign",
        [MSG_SEND_EMPTY]       = "Empty message",
        [MSG_SEND_OUTBOX_FULL] = "Outbox full",
        [MSG_SEND_FLASH]       = "Flash error",
        [MSG_SEND_DISABLED]    = "Messenger off",
        [MSG_SEND_NO_KEY]      = "Set a key first",
        [MSG_SEND_HAM_BAND]    = "No crypto on ham",
        [MSG_SEND_TOO_LONG]    = "Max 40 encrypted",
        [MSG_SEND_NO_NET]      = "Set IS net first",
    };
    msg_send_result_t r;
    if (gTo[0] == '#')                 /* "#GROUP", "#ALL" = public */
        r = MSG_SendBroadcast(strcmp(gTo, "#ALL") == 0 ? "" : gTo + 1, gEdit.buf);
    else if (MSG_IsLobby(gTo))         /* "CQ", "QST", "ALL" */
        r = MSG_SendLobby(gTo, gEdit.buf);
    else
        r = MSG_Send(gTo, gEdit.buf);
    popup(AT(ERR[r]));
    if (r == MSG_SEND_OK) {
        gView = V_LIST;
        list_reset();
    }
}

/* ---------------------------------------------------------- setup ---- */

static void setup_value(uint8_t item, char *v)
{
    const msg_config_t *c = MSG_GetConfig();
    v[0] = '\0';
    switch (item) {
    case S_ENABLE:  acpy(v, c->enabled ? AT(T_ON) : AT(T_OFF)); break;
    case S_CALL:    acpy(v, c->call[0] ? c->call : A("---")); break;
    case S_SSID:    if (c->ssid) put_u(v, c->ssid); else acpy(v, AT(T_NONE)); break;
    case S_CHANNEL: if (c->group[0]) { v[0] = '#'; strcpy(v + 1, c->group); } else acpy(v, AT(T_NONE)); break;
    case S_PUBLIC:  acpy(v, c->bulletins ? AT(T_ON) : AT(T_OFF)); break;
    case S_LOBBY:   acpy(v, c->lobby ? AT(T_ON) : AT(T_OFF)); break;
    case S_NET:     acpy(v, c->net[0] ? c->net : AT(T_NONE)); break;
    case S_HOLD:    acpy(v, AT(HOLD_TXT[c->hold])); break;
    case S_VOICE:   acpy(v, AT(VOICE_TXT[c->voice])); break;
    case S_POS:     acpy(v, AT(POS_TXT[c->pos_mode])); break;
    case S_LOCATOR: acpy(v, c->locator[0] ? c->locator : A("---")); break;
    case S_LAT:     *aprs_put_coord(v, c->lat, 2, 'N', 'S') = '\0'; break;
    case S_LON:     *aprs_put_coord(v, c->lon, 3, 'E', 'W') = '\0'; break;
    case S_SYMBOL:  strcpy(v, MSG_SymbolName(c->symbol)); break;
    case S_CLEAR:   if (!MSG_FlashOk()) acpy(v, A("USED")); break;   /* 13 + 1 + 4 = a row */
    case S_BEEP:    acpy(v, AT(BEEP_TXT[c->beep])); break;
    case S_FREQ:    freq_text(c->freq, v); break;
    case S_ENCRYPT: acpy(v, !c->encrypt ? AT(T_OFF) : MSG_HamBand(c->freq) ? A("HAM: OFF") : AT(T_ON)); break;
    case S_KEY:     acpy(v, c->has_key ? A("set") : AT(T_NONE)); break;
    case S_PATH:    acpy(v, AT(PATH_TXT[c->path])); break;
    case S_BEACON:
        strcpy(v, MSG_BeaconMinutesText(c->beacon));
        if (c->beacon)
            acat(v, A(" min"));
        break;
    case S_STATUS:  strncpy(v, c->status, 8); v[8] = '\0'; break;
    case S_AUDIO:   acpy(v, c->audio ? AT(T_ON) : AT(T_OFF)); break;
    default: break;
    }
}

static void draw_setup(void)
{
    title(A("MESSENGER SETUP"));
    for (uint8_t row = 0; row < LIST_ROWS; row++) {
        const uint8_t item = (uint8_t)(gSetupTop + row);
        if (item >= S_COUNT)
            break;
        char v[24];
        setup_value(item, v);
        label_value((uint8_t)(1u + row), AT(SETUP_LABEL[item]), v);
        if (item == gSetupSel)
            invert_line((uint8_t)(1u + row));
    }
    /* what MENU and F do on some items; elsewhere the bring-up figures */
    static const uint8_t HINT_ITEM[] = {S_NET, S_FREQ, S_HOLD, S_VOICE, S_LAT, S_LON};
    static const char HINT_TXT[][32] FW_ASSET = {
        "MENU EDIT  F JOIN VIA ANSRVR", "MENU TYPE IN  F PRESETS",
        "APRS TIME BEFORE EACH LOOK AT A", "ON VFO A AFTER VOICE TX/RX",
        "MENU EDIT  F NORTH/SOUTH", "MENU EDIT  F EAST/WEST",
    };
    _Static_assert(sizeof HINT_ITEM == sizeof HINT_TXT / sizeof HINT_TXT[0], "one hint per item");
    for (uint8_t i = 0; i < sizeof HINT_ITEM; i++)
        if (HINT_ITEM[i] == gSetupSel) {
            tiny(AT(HINT_TXT[i]), 0, 6);
            return;
        }
    /* bring-up figures: frames heard / sent, DSP load, audio level */
    msg_hw_diag_t d;
    msg_hw_diag(&d);
    char diag[40];
    char *o = diag;
    memcpy(o, "RX", 2);
    o = put_u(o + 2, MSG_FramesHeard());
    memcpy(o, " TX", 3);
    o = put_u(o + 3, MSG_FramesSent());
    memcpy(o, " DSP", 4);
    o = put_u(o + 4, d.load_pm / 10u);
    *o++ = '%';
    memcpy(o, " LV", 3);
    o = put_u(o + 3, d.level_pp);
    if (d.overruns) {
        memcpy(o, " OV", 3);
        put_u(o + 3, d.overruns);
    }
    tiny(diag, 0, 6);
}

static void open_setup(uint8_t sel)
{
    gSetupSel = sel;
    gSetupTop = sel >= LIST_ROWS ? (uint8_t)(sel - LIST_ROWS + 1u) : 0u;
    gView = V_SETUP;
}

static void setup_change(int8_t dir)
{
    msg_config_t c = *MSG_GetConfig();
    switch (gSetupSel) {
    case S_ENABLE:
        if (!c.enabled && !c.call[0]) {
            popup(AT(T_SET_CALL));
            return;
        }
        c.enabled = !c.enabled;
        MSG_SetConfig(&c);
        if (c.enabled && !msg_port_has_aprs_vfo()) {
            char t[20];
            acpy(t, A("VFO to "));
            freq_text(c.freq, t + strlen(t));
            popup(t);
        }
        return;
    case S_CALL:
        t9_init(&gEdit, AX25_CALL_LEN, true, c.call);
        gView = V_EDIT_CALL;
        return;
    case S_SSID:
        c.ssid = (uint8_t)((c.ssid + 16 + dir) % 16);
        break;
    case S_CHANNEL:
        t9_init(&gEdit, MSG_GROUP_LEN, true, c.group);
        gView = V_EDIT_GROUP;
        return;
    case S_PUBLIC:
        c.bulletins = !c.bulletins;
        break;
    case S_LOBBY:
        c.lobby = !c.lobby;
        break;
    case S_NET:
        if (dir > 0) {                 /* MENU: the group name */
            t9_init(&gEdit, MSG_NET_LEN, true, c.net);
            gView = V_EDIT_NET;
        } else {                       /* F: join it */
            popup(MSG_JoinNet() == MSG_SEND_OK ? A("Join queued") : c.net[0] ? AT(T_CANNOT) : AT(T_NO_NET));
        }
        return;
    case S_HOLD:
        c.hold = (uint8_t)((c.hold + MSG_HOLD_COUNT + dir) % MSG_HOLD_COUNT);
        break;
    case S_VOICE:
        c.voice = (uint8_t)((c.voice + MSG_VOICE_COUNT + dir) % MSG_VOICE_COUNT);
        break;
    case S_POS:
        c.pos_mode = (uint8_t)((c.pos_mode + MSG_POS_COUNT + dir) % MSG_POS_COUNT);
        break;
    case S_LOCATOR:
        t9_init(&gEdit, MSG_LOCATOR_LEN, true, c.locator);
        gView = V_EDIT_LOC;
        return;
    case S_LAT:
    case S_LON: {
        int32_t *v = gSetupSel == S_LAT ? &c.lat : &c.lon;
        if (dir < 0) {                 /* F: the other hemisphere */
            *v = -*v;
            break;
        }
        /* DDMMmm / DDDMMmm: degrees * 10000 + hundredths of a minute */
        const int32_t a = *v < 0 ? -*v : *v;
        uint32_t num = (uint32_t)(a / APRS_POS_DEG * 10000 + a % APRS_POS_DEG);
        const uint8_t n = gSetupSel == S_LAT ? 6u : 7u;
        char digits[8];
        for (int8_t i = (int8_t)(n - 1u); i >= 0; i--, num /= 10u)
            digits[i] = (char)('0' + num % 10u);
        digits[n] = '\0';
        t9_init(&gEdit, n, true, digits);
        gEdit.mode = T9_MODE_DIGITS;
        gView = gSetupSel == S_LAT ? V_EDIT_LAT : V_EDIT_LON;
        return;
    }
    case S_SYMBOL:
        c.symbol = (uint8_t)((c.symbol + MSG_SYMBOL_COUNT + dir) % MSG_SYMBOL_COUNT);
        break;
    case S_BEEP:
        c.beep = (uint8_t)((c.beep + MSG_BEEP_COUNT + dir) % MSG_BEEP_COUNT);
        break;
    case S_FREQ: {
        if (dir > 0) {                 /* MENU: type it in */
            char f[12];
            freq_text(c.freq, f);
            char digits[10];
            uint8_t n = 0;
            for (const char *s = f; *s && n < 8u; s++)
                if (*s != '.')
                    digits[n++] = *s;
            digits[n] = '\0';
            t9_init(&gEdit, 8, true, digits);
            gEdit.mode = T9_MODE_DIGITS;
            gView = V_EDIT_FREQ;
            return;
        }
        uint8_t i = 0;                 /* F: the next preset */
        while (i < FREQ_COUNT && FREQS[i] != c.freq)
            i++;
        c.freq = FREQS[i + 1u >= FREQ_COUNT ? 0u : i + 1u];
        break;
    }
    case S_ENCRYPT:
        if (!c.has_key) {
            popup(AT(T_NO_KEY));
            return;
        }
        c.encrypt = !c.encrypt;
        MSG_SetConfig(&c);
        if (c.encrypt)
            popup(MSG_HamBand(c.freq) ? A("Not on ham bands") : A("Mind local rules"));
        return;
    case S_KEY:
        t9_init(&gEdit, MC_PASS_MAX, false, NULL);
        gView = V_EDIT_KEY;
        return;
    case S_PATH:
        c.path = (uint8_t)((c.path + MSG_PATH_COUNT + dir) % MSG_PATH_COUNT);
        break;
    case S_BEACON:
        c.beacon = (uint8_t)((c.beacon + MSG_BEACON_COUNT + dir) % MSG_BEACON_COUNT);
        break;
    case S_BEACON_NOW:
        popup(MSG_BeaconNow() ? A("Beacon queued") : A("Messenger is OFF"));
        return;
    case S_STATUS:
        t9_init(&gEdit, MSG_STATUS_LEN, false, c.status);
        gView = V_EDIT_STATUS;
        return;
    case S_AUDIO:
        c.audio = !c.audio;
        break;
    case S_CLEAR:
        gConfirm = CONFIRM_CLEAR;
        gBack = V_SETUP;
        gView = V_CONFIRM;
        return;
    default:
        return;
    }
    MSG_SetConfig(&c);
}

static void draw_edit_setting(void)
{
    static const char TITLE[][16] FW_ASSET = {"YOUR CALLSIGN", "CHANNEL", "BEACON STATUS",
                                              "FREQUENCY", "KEY PASSPHRASE", "IS NET (ANSRVR)",
                                              "LOCATOR", "LATITUDE", "LONGITUDE"};
    static const char LABEL[][19] FW_ASSET = {"Without SSID:", "1-5 letters/digits", "Status text:",
                                              "MHz, no dot:", "Same on all radios", "Group, 1-9 chars:",
                                              "4, 6 or 8 chars:", "DDMMmm:", "DDDMMmm:"};
    const uint8_t k = (uint8_t)(gView - V_EDIT_CALL);
    title(AT(TITLE[k]));
    print(AT(LABEL[k]), 0, 2);
    draw_editor(3, (gView == V_EDIT_STATUS || gView == V_EDIT_KEY) ? 2u : 1u);
    if (gView == V_EDIT_GROUP)
        tiny(A("EMPTY = NO CHANNEL"), 0, 5);
    if (gView == V_EDIT_FREQ)
        tiny(A("14480000 = 144.800"), 0, 5);
    if (gView == V_EDIT_KEY)
        tiny(A("16+ CHARS, EMPTY = NO KEY"), 0, 5);
    if (gView == V_EDIT_NET)
        tiny(A("E.G. HOTG, EMPTY = NONE"), 0, 5);
    if (gView == V_EDIT_LOC)
        tiny(A("E.G. JO62QM, EMPTY = NONE"), 0, 5);
    if (gView == V_EDIT_LAT)
        tiny(A("485090 = 48 50.90"), 0, 5);
    if (gView == V_EDIT_LON)
        tiny(A("0021625 = 002 16.25"), 0, 5);
    char hint[40];
    acpy(hint, A("MENU SAVE  * "));
    strcat(hint, t9_mode_name(&gEdit));
    tiny(hint, 0, 6);
}

static void save_edit_setting(void)
{
    t9_commit(&gEdit);
    msg_config_t c = *MSG_GetConfig();
    const view_t from = gView;
    if (gView == V_EDIT_CALL) {
        ax25_addr_t a;
        if (gEdit.len && (!ax25_addr_parse(gEdit.buf, &a) || a.ssid || strchr(gEdit.buf, '-'))) {
            popup(AT(T_BAD_CALL));
            return;
        }
        strcpy(c.call, gEdit.buf);
        if (!c.call[0])
            c.enabled = 0;
    } else if (gView == V_EDIT_GROUP) {
        for (uint8_t i = 0; i < gEdit.len; i++)
            if (gEdit.buf[i] == '-') {
                popup(AT(T_DIGITS));
                return;
            }
        if (strcmp(gEdit.buf, "ALL") == 0) {
            popup(A("ALL is reserved"));
            return;
        }
        strcpy(c.group, gEdit.buf);
    } else if (gView == V_EDIT_FREQ) {
        /* "14480000": three digits of MHz, up to five decimals */
        bool digits = gEdit.len >= 4u && gEdit.buf[0] != '0';
        for (uint8_t i = 0; i < gEdit.len; i++)
            digits &= gEdit.buf[i] >= '0' && gEdit.buf[i] <= '9';
        if (!digits) {
            popup(A("e.g. 14480000"));
            return;
        }
        uint32_t f = 0;
        for (uint8_t i = 0; i < 8u; i++)
            f = f * 10u + (uint32_t)(i < gEdit.len ? gEdit.buf[i] - '0' : 0);
        c.freq = f;
    } else if (gView == V_EDIT_KEY) {
        MSG_SetKey(gEdit.buf);         /* derives the key; the passphrase is not kept */
        memset(&gEdit, 0, sizeof gEdit);
        popup(MSG_GetConfig()->has_key ? A("Key saved") : A("Key removed"));
        open_setup(S_KEY);
        return;
    } else if (gView == V_EDIT_NET) {
        if (strchr(gEdit.buf, '-')) {
            popup(AT(T_DIGITS));
            return;
        }
        strcpy(c.net, gEdit.buf);
    } else if (gView == V_EDIT_LOC) {
        int32_t la, lo;
        if (gEdit.len && !aprs_locator_to_pos(gEdit.buf, &la, &lo)) {
            popup(A("e.g. JO62QM"));
            return;
        }
        strcpy(c.locator, gEdit.buf);
        if (gEdit.len)
            c.pos_mode = MSG_POS_LOCATOR;   /* a locator typed in is meant to be used */
    } else if (gView == V_EDIT_LAT || gView == V_EDIT_LON) {
        /* DDMMmm or DDDMMmm, the hemisphere stays */
        const bool lat = gView == V_EDIT_LAT;
        bool ok = gEdit.len == (lat ? 6u : 7u);
        int32_t num = 0;
        for (uint8_t i = 0; i < gEdit.len; i++) {
            const char ch = gEdit.buf[i];
            ok &= ch >= '0' && ch <= '9';      /* '*' switches to letters */
            num = num * 10 + (ch - '0');
        }
        const int32_t min = num % 10000;
        const int32_t v = num / 10000 * APRS_POS_DEG + min;
        if (!ok || min >= APRS_POS_DEG || v > (lat ? APRS_LAT_MAX : APRS_LON_MAX)) {
            popup(lat ? A("e.g. 485090") : A("e.g. 0021625"));
            return;
        }
        int32_t *t = lat ? &c.lat : &c.lon;
        *t = *t < 0 ? -v : v;
        c.pos_mode = MSG_POS_FIXED;
    } else {
        strcpy(c.status, gEdit.buf);
    }
    MSG_SetConfig(&c);
    /* back to the item; the first setup goes on from the callsign to "Messenger ON" */
    static const uint8_t BACK[] = {S_CALL, S_CHANNEL, S_STATUS, S_FREQ, S_KEY, S_NET, S_LOCATOR,
                                   S_LAT, S_LON};
    open_setup(from == V_EDIT_CALL && !c.enabled ? (uint8_t)S_ENABLE : BACK[from - V_EDIT_CALL]);
}

/* ---------------------------------------------------------- heard ---- */

static void draw_heard(void)
{
    title(A("STATIONS HEARD"));
    char call[AX25_CALL_STR];
    uint32_t age;
    if (!MSG_Heard(0, call, &age)) {
        print(AT(T_NOTHING), 0, 2);
        return;
    }
    for (uint8_t row = 0; row < LIST_ROWS && MSG_Heard((uint8_t)(gHeardTop + row), call, &age); row++) {
        char a[8];
        char *o = put_u(a, age < 60u ? age : age < 3600u ? age / 60u : age / 3600u);
        *o++ = age < 60u ? 's' : age < 3600u ? 'm' : 'h';
        *o = '\0';
        label_value((uint8_t)(1u + row), call, a);
        if (gHeardTop + row == gHeardSel)
            invert_line((uint8_t)(1u + row));
    }
    tiny(A("MENU WRITE  EXIT BACK"), 0, 6);
}

/* -------------------------------------------------------- monitor ---- */

/* every frame heard, raw, newest first; 32 tiny characters a row */
static void draw_monitor(void)
{
    title(A("MONITOR"));
    uint8_t line = 1;
    const char *e;
    for (uint8_t i = gHeardTop; line < 7u && (e = MSG_MonitorEntry(i)) != NULL; i++) {
        tiny(e, 0, line++);
        if (e[33] && line < 7u)
            tiny(e + 33, 0, line++);
    }
    if (line == 1u)
        print(AT(T_NOTHING), 0, 2);
}

/* -------------------------------------------------------- confirm ---- */

static void draw_confirm(void)
{
    const bool clear = gConfirm == CONFIRM_CLEAR;
    title(clear ? A("CLEAR MAILBOX") : A("DELETE"));
    if (clear && !MSG_FlashOk()) {     /* clearing takes another program's data */
        print(A("The area holds"), 0, 2);
        print(A("other data, e.g."), 0, 3);
        print(A("apps. Erase it?"), 0, 4);
        tiny(A("MENU ERASE   EXIT KEEP"), 0, 6);
        return;
    }
    print(clear ? A("Delete all") : A("Delete this"), 0, 2);
    print(A("messages?"), 0, 3);
    tiny(A("MENU YES   EXIT NO"), 0, 6);
}

/* ------------------------------------------------------------ API ---- */

void MSG_OpenScreen(void)
{
    gPopupUntil = 0;
    gThread[0] = '\0';
    if (!MSG_GetConfig()->call[0]) {   /* first start: the callsign first */
        t9_init(&gEdit, AX25_CALL_LEN, true, NULL);
        gView = V_EDIT_CALL;
    } else {
        gView = V_LIST;
        list_reset();
    }
    GUI_SelectNextDisplay(DISPLAY_MESSENGER);
}

/* Side key: the messenger on or off; the status bar shows APRS while it is
 * on. Without a callsign the messenger opens for the first start. With the
 * key beep on, ON gives the single high beep every side key gives, OFF a
 * low double beep. */
void MSG_ToggleOnOff(void)
{
    msg_config_t c = *MSG_GetConfig();
    if (!c.call[0]) {
        MSG_OpenScreen();
        return;
    }
    c.enabled = !c.enabled;
    MSG_SetConfig(&c);
    if (!c.enabled)
        gBeepToPlay = BEEP_500HZ_60MS_DOUBLE_BEEP_OPTIONAL;
    gUpdateStatus = true;
}

void MSG_DisplayScreen(void)
{
    UI_DisplayClear();
    switch (gView) {
    case V_LIST:        draw_list(); break;
    case V_READ:        draw_read(); break;
    case V_TO:
    case V_TEXT:        draw_compose(); break;
    case V_SETUP:       draw_setup(); break;
    case V_EDIT_CALL:
    case V_EDIT_GROUP:
    case V_EDIT_STATUS:
    case V_EDIT_FREQ:
    case V_EDIT_KEY:
    case V_EDIT_NET:
    case V_EDIT_LOC:
    case V_EDIT_LAT:
    case V_EDIT_LON:    draw_edit_setting(); break;
    case V_CONFIRM:     draw_confirm(); break;
    case V_HEARD:       draw_heard(); break;
    case V_MONITOR:     draw_monitor(); break;
    }
    if (gPopupUntil)
        draw_popup();
    ST7565_BlitFullScreen();
}

static void open_read(void)
{
    if (gSel == MBX_NONE)
        return;
    mbx_rec_t r;
    if (mbx_read(gSel, &r) && (r.flags & MBX_F_READ))
        mbx_set_flags(gSel, MBX_F_READ);
    gReadTop = 0;
    gView = V_READ;
    gUpdateStatus = true;
}

static void keys_editor(KEY_Code_t key, bool held)
{
    const uint32_t now = msg_port_ms();
    if (key <= KEY_9 && !held) {
        t9_digit(&gEdit, (uint8_t)key, now);
        return;
    }
    switch (key) {
    case KEY_STAR: if (!held) t9_next_mode(&gEdit); break;
    case KEY_UP:   t9_left(&gEdit); break;
    case KEY_DOWN: t9_right(&gEdit); break;
    default: break;
    }
}

static bool editing(void)
{
    return gView == V_TO || gView == V_TEXT || (gView >= V_EDIT_CALL && gView <= V_EDIT_LON);
}

static view_t editor_exit(void)
{
    return (gView == V_TO || gView == V_TEXT) ? V_LIST : V_SETUP;
}

void MSG_ProcessKeys(KEY_Code_t Key, bool bKeyPressed, bool bKeyHeld)
{
    if (Key == KEY_PTT) {
        GENERIC_Key_PTT(bKeyPressed);
        return;
    }
    if (!bKeyPressed)
        return;                        /* act on press and on repeats */
    const bool held = bKeyHeld;
    const bool repeatable = Key == KEY_UP || Key == KEY_DOWN;
    /* A long EXIT only cancels an editor. Its press already acted; outside an
     * editor the hold must not act again (it would leave the messages screen). */
    if (held && !repeatable && (Key != KEY_EXIT || !editing()))
        return;

    if (gPopupUntil) {                 /* any key closes a popup */
        gPopupUntil = 0;
        gUpdateDisplay = true;
        if (Key != KEY_EXIT)
            return;
    }
    gBeepToPlay = BEEP_1KHZ_60MS_OPTIONAL;
    gUpdateDisplay = true;

    if (editing()) {
        if (Key == KEY_EXIT) {
            if (held) {                /* long EXIT: leave the editor */
                gView = editor_exit();
                if (gView == V_LIST)
                    list_reset();
            } else if (gEdit.len || t9_pending(&gEdit)) {
                t9_backspace(&gEdit);
            } else if (gView == V_TEXT) {
                t9_init(&gEdit, AX25_CALL_STR - 1u, true, gTo);   /* back to the recipient */
                gView = V_TO;
            } else {
                gView = editor_exit();
                if (gView == V_LIST)
                    list_reset();
            }
            return;
        }
        if (gView == V_TO && Key == KEY_F) {       /* channel, ALL, CQ, IS net */
            next_target();
            return;
        }
        if (Key == KEY_MENU) {
            if (gView == V_TO) {
                t9_commit(&gEdit);
                ax25_addr_t a;
                if (gEdit.buf[0] == '#') {          /* a channel: "#WEBER", "#ALL" */
                    strcpy(gTo, gEdit.buf);
                } else if (!ax25_addr_parse(gEdit.buf, &a) || a.repeated) {
                    popup(AT(T_BAD_CALL));
                    return;
                } else {
                    ax25_addr_format(&a, gTo);
                }
                open_text();
            } else if (gView == V_TEXT) {
                send_now();
            } else {
                save_edit_setting();
            }
            return;
        }
        keys_editor(Key, held);
        return;
    }

    switch (gView) {
    case V_LIST:
        switch (Key) {
        case KEY_UP:   list_up(); break;
        case KEY_DOWN: list_down(); break;
        case KEY_MENU: open_read(); break;
        case KEY_STAR: open_compose(gThread[0] ? gThread : NULL); break;
        case KEY_1: {                  /* the conversation of the selected message */
            mbx_rec_t r;
            if (!gThread[0] && gSel != MBX_NONE && mbx_read(gSel, &r)) {
                thread_of(&r, gThread);
                list_reset();
            }
            break;
        }
        case KEY_2:                    /* the stations heard */
        case KEY_3:                    /* the monitor */
            gHeardSel = gHeardTop = 0;
            gView = Key == KEY_2 ? V_HEARD : V_MONITOR;
            break;
        case KEY_F:
            open_setup(S_ENABLE);
            break;
        case KEY_EXIT:
            if (gThread[0]) {          /* back to all messages */
                gThread[0] = '\0';
                list_reset();
            } else {
                GUI_SelectNextDisplay(DISPLAY_MAIN);
            }
            break;
        default: break;
        }
        break;

    case V_READ: {
        mbx_rec_t r;
        const bool ok = mbx_read(gSel, &r);
        switch (Key) {
        case KEY_UP:   if (gReadTop) gReadTop--; break;
        case KEY_DOWN: gReadTop++; break;          /* clamped while drawing */
        case KEY_MENU:
            if (!ok)
                break;
            if (r.dir == MBX_DIR_IN) {
                open_compose(r.peer);
            } else if (group_in(r.dir)) {
                char chan[THREAD_LEN];
                thread_of(&r, chan);
                open_compose(chan);    /* answer in the same channel or lobby */
            } else if (r.dir == MBX_DIR_OUT && (r.flags & MBX_F_DELIVERED)) {
                /* not confirmed: again; still being sent: the next try now */
                popup(MSG_Resend(gSel) == MSG_SEND_OK ? A("Sending now") : AT(T_CANNOT));
            }
            break;
        case KEY_F:
            if (ok && group_in(r.dir))
                open_compose(r.peer);  /* private answer to a broadcast */
            else if (MSG_Stop(gSel))
                popup(A("Sending stopped"));
            break;
        case KEY_0:
            gConfirm = CONFIRM_DELETE;
            gBack = V_READ;
            gView = V_CONFIRM;
            break;
        case KEY_EXIT:
            gView = V_LIST;
            break;
        default: break;
        }
        break;
    }

    case V_SETUP:
        switch (Key) {
        case KEY_UP:
            gSetupSel = (uint8_t)(gSetupSel ? gSetupSel - 1u : S_COUNT - 1u);
            break;
        case KEY_DOWN:
            gSetupSel = (uint8_t)((gSetupSel + 1u) % S_COUNT);
            break;
        case KEY_MENU: setup_change(+1); break;
        case KEY_F:    setup_change(-1); break;
        case KEY_EXIT:
            gView = V_LIST;
            list_reset();
            break;
        default: break;
        }
        if (gSetupSel < gSetupTop)
            gSetupTop = gSetupSel;
        else if (gSetupSel >= gSetupTop + LIST_ROWS)
            gSetupTop = (uint8_t)(gSetupSel - LIST_ROWS + 1u);
        break;

    case V_HEARD: {
        char call[AX25_CALL_STR];
        uint32_t age;
        switch (Key) {
        case KEY_UP:
            if (gHeardSel)
                gHeardSel--;
            break;
        case KEY_DOWN:
            if (MSG_Heard((uint8_t)(gHeardSel + 1u), call, &age))
                gHeardSel++;
            break;
        case KEY_MENU:
            if (MSG_Heard(gHeardSel, call, &age))
                open_compose(call);
            break;
        case KEY_EXIT:
            gView = V_LIST;
            break;
        default: break;
        }
        if (gHeardSel < gHeardTop)
            gHeardTop = gHeardSel;
        else if (gHeardSel >= gHeardTop + LIST_ROWS)
            gHeardTop = (uint8_t)(gHeardSel - LIST_ROWS + 1u);
        break;
    }

    case V_MONITOR:                    /* gHeardTop: the first frame shown */
        if (Key == KEY_UP && gHeardTop)
            gHeardTop--;
        else if (Key == KEY_DOWN && MSG_MonitorEntry((uint8_t)(gHeardTop + 1u)))
            gHeardTop++;
        else if (Key == KEY_EXIT)
            gView = V_LIST;
        break;

    case V_CONFIRM:
        if (Key == KEY_MENU) {
            if (gConfirm == CONFIRM_CLEAR) {
                MSG_ClearMailbox();
                popup(A("Mailbox cleared"));
                gThread[0] = '\0';
                list_reset();
                gView = V_SETUP;
            } else {
                MSG_Delete(gSel);
                list_reset();
                if (gSel == MBX_NONE && gThread[0]) {   /* the chat is empty now */
                    gThread[0] = '\0';
                    list_reset();
                }
                gView = V_LIST;
            }
            gUpdateStatus = true;
        } else if (Key == KEY_EXIT) {
            gView = gBack;
        }
        break;

    default:
        break;
    }
}

void msg_ui_tick(void)
{
    if (gScreenToDisplay != DISPLAY_MESSENGER)
        return;
    const uint32_t now = msg_port_ms();
    if (gView == V_LIST && (gSel == MBX_NONE || gSel == gTop)) {
        const int16_t latest = newest();
        if (latest != gTop) {
            gSel = gTop = latest;
            gUpdateDisplay = true;
        }
    }
    if (editing() && t9_tick(&gEdit, now))
        gUpdateDisplay = true;
    if (gPopupUntil && (int32_t)(now - gPopupUntil) >= 0) {
        gPopupUntil = 0;
        gUpdateDisplay = true;
    }
    /* the setup screen shows live figures, the list the sending state */
    static uint8_t refresh;
    if ((gView == V_SETUP || gView == V_LIST || gView == V_READ || gView >= V_HEARD) && ++refresh >= 50u) {
        refresh = 0;
        gUpdateDisplay = true;
    }
}
