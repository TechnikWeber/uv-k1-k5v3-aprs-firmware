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

/* The messenger core: settings, received frames, acknowledgements, the outbox
 * with its retries, the status beacon and the transmit scheduler. Pure C over
 * msg_port.h, mailbox.h and msg_flash.h; driven by MSG_Task() every 10 ms. */

#ifndef MESSENGER_MESSENGER_H
#define MESSENGER_MESSENGER_H

#include <stdbool.h>
#include <stdint.h>

#include "aprs_ax25.h"
#include "aprs_msg.h"
#include "msg_crypto.h"
#include "msg_port.h"

#define MSG_STATUS_LEN    20u
#define MSG_DEFAULT_FREQ  14480000u     /* 144.800 MHz (IARU Region 1), 10 Hz units */
#define MSG_TOCALL        "APZK5M"      /* APZ: experimental software      */
#define MSG_OUTBOX_MAX    4u            /* messages waiting for an ack     */
#define MSG_RETRIES       5u            /* transmissions of one message    */
#define MSG_GROUP_LEN     5u            /* channel = APRS bulletin group   */
#define MSG_BCAST_SENDS   2u            /* a broadcast goes out twice      */
#define MSG_LOCATOR_LEN   8u            /* Maidenhead locator, 4/6/8 chars */
#define MSG_NET_LEN       9u            /* ANSRVR group name               */
#define MSG_NET_SERVER    "ANSRVR"      /* APRS-IS group message server    */
#define MSG_HEARD_MAX     8u            /* station list                    */
#define MSG_MON_MAX       8u            /* monitor: frames kept (power of 2) */
#define MSG_MON_LEN       66u           /* 2 rows of 32 tiny characters + NUL */

enum { MSG_PATH_DIRECT = 0, MSG_PATH_WIDE1, MSG_PATH_WIDE1_WIDE2, MSG_PATH_COUNT };
enum { MSG_BEACON_OFF = 0, MSG_BEACON_10, MSG_BEACON_20, MSG_BEACON_30, MSG_BEACON_60,
       MSG_BEACON_COUNT };
enum { MSG_BEEP_OFF = 0, MSG_BEEP_ALL, MSG_BEEP_PRIVATE, MSG_BEEP_COUNT };
/* where the position of the beacon comes from */
enum { MSG_POS_OFF = 0, MSG_POS_LOCATOR, MSG_POS_FIXED, MSG_POS_COUNT };
/* dual watch: how long the receiver stays on the APRS VFO each time before
 * it looks at the other VFO for 0.1 s; ALWAYS keeps it there. A packet is
 * caught by its preamble when it starts during that look; the receiver then
 * stays until the packet is over. 100 ms is about as fast as plain dual
 * watch (experimental). */
enum { MSG_HOLD_100MS = 0, MSG_HOLD_1S, MSG_HOLD_2S, MSG_HOLD_5S, MSG_HOLD_10S,
       MSG_HOLD_ALWAYS, MSG_HOLD_COUNT };
/* how long the receiver stays on the voice VFO after a voice transmission or
 * reception there, before dual watch goes back to APRS */
enum { MSG_VOICE_4S = 0, MSG_VOICE_10S, MSG_VOICE_20S, MSG_VOICE_60S, MSG_VOICE_COUNT };
#define MSG_SYMBOL_COUNT  6u

typedef struct {
    char     call[AX25_CALL_LEN + 1];   /* "" until the user sets it        */
    uint8_t  ssid;
    uint8_t  enabled;                   /* background reception on/off      */
    uint8_t  path;                      /* MSG_PATH_*                       */
    uint8_t  beacon;                    /* MSG_BEACON_*                     */
    uint8_t  beep;                      /* MSG_BEEP_*                       */
    uint8_t  audio;                     /* APRS bursts on the speaker       */
    uint8_t  bulletins;                 /* public bulletins (BLNn, BLNx) too */
    uint32_t freq;                      /* APRS frequency, 10 Hz units      */
    uint16_t next_id;                   /* message number, 1..999           */
    char     status[MSG_STATUS_LEN + 1];/* beacon text                      */
    char     group[MSG_GROUP_LEN + 1];  /* own channel, "" = none           */
    uint8_t  next_bln;                  /* bulletin line 0..9 for the next broadcast */
    uint8_t  encrypt;                   /* encrypt what is sent (needs a key) */
    uint8_t  has_key;
    uint8_t  key[MC_KEY_LEN];           /* derived from the passphrase      */
    uint8_t  pos_mode;                  /* MSG_POS_*: position in the beacon */
    uint8_t  symbol;                    /* beacon symbol, see MSG_SymbolName */
    uint8_t  hold;                      /* MSG_HOLD_*                       */
    uint8_t  lobby;                     /* messages to CQ, QST, ALL too     */
    int32_t  lat;                       /* fixed position, 1/100', north +  */
    int32_t  lon;                       /*                 1/100', east +   */
    char     locator[MSG_LOCATOR_LEN + 1]; /* "JO62QM", "" = none           */
    char     net[MSG_NET_LEN + 1];      /* ANSRVR group, "" = none          */
    uint8_t  voice;                     /* MSG_VOICE_* (in the padding of v4) */
} msg_config_t;

typedef enum {
    MSG_SEND_OK = 0,
    MSG_SEND_NO_CALL,                   /* own callsign not set             */
    MSG_SEND_BAD_CALL,                  /* recipient is not a callsign      */
    MSG_SEND_EMPTY,
    MSG_SEND_OUTBOX_FULL,
    MSG_SEND_FLASH,
    MSG_SEND_DISABLED,
    MSG_SEND_NO_KEY,                    /* encryption on, no key set        */
    MSG_SEND_HAM_BAND,                  /* encryption on an amateur band    */
    MSG_SEND_TOO_LONG,                  /* encrypted text over MC_PLAIN_MAX */
    MSG_SEND_NO_NET,                    /* no ANSRVR group set              */
} msg_send_result_t;

/* Loads the settings and scans the mailbox. */
void               MSG_CoreInit(void);
void MSG_Delete(int16_t slot);
void MSG_ClearMailbox(void);
/* Every 10 ms from the main loop. */
void               MSG_Task(void);

const msg_config_t *MSG_GetConfig(void);
void               MSG_SetConfig(const msg_config_t *cfg);   /* saves to flash */
/* Own station as an address; false while no callsign is set. */
bool               MSG_MyAddr(ax25_addr_t *out);

/* Queues a new message (stored in the outbox at once). */
msg_send_result_t  MSG_Send(const char *to, const char *text);
/* A broadcast: group bulletin "BLNnGROUP", or a public bulletin "BLNn" for
 * group "". Not acknowledged; sent twice. */
msg_send_result_t  MSG_SendBroadcast(const char *group, const char *text);
/* The lobby: a message to CQ, QST or ALL, read by every station that shows
 * them (Yaesu and Kenwood group codes). Not acknowledged; sent twice. */
msg_send_result_t  MSG_SendLobby(const char *to, const char *text);
/* to is "CQ", "QST" or "ALL" (any case). */
bool               MSG_IsLobby(const char *to);
/* Joins the ANSRVR group of the settings: "J GROUP" to ANSRVR, which then
 * relays the group's messages; "CQ GROUP text" to ANSRVR posts to it. */
msg_send_result_t  MSG_JoinNet(void);
/* Sends a retry of an outgoing message now (from the mailbox). */
msg_send_result_t  MSG_Resend(int16_t slot);

/* Messages waiting for an ack, frames waiting to go out. */
uint8_t            MSG_OutboxCount(void);
/* This outgoing record is still being sent: transmissions so far in *tries. */
bool               MSG_SlotPending(int16_t slot, uint8_t *tries);
bool               MSG_TxPending(void);
/* Last transmit problem, MSG_TX_OK once a frame went out. */
msg_tx_result_t    MSG_LastTxResult(void);
/* Frames heard / transmitted since boot (settings screen). */
uint16_t           MSG_FramesHeard(void);
uint16_t           MSG_FramesSent(void);
/* Station list, newest first: entry i (call, seconds since last heard);
 * false past the end. */
bool               MSG_Heard(uint8_t i, char *call, uint32_t *age_s);
/* Monitor: the last MSG_MON_MAX frames heard, any kind, as "CALL:info"
 * (unprintable bytes as '.'), newest first; NULL past the end. RAM only.
 * Two rows: e and, when e[33] is set, e + 33 (each ends with a NUL). */
const char        *MSG_MonitorEntry(uint8_t i);
/* Sends a beacon as soon as the channel is clear, also with Beacon OFF; the
 * interval starts again from it. false while the messenger is off. */
bool               MSG_BeaconNow(void);

/* The beacon position, from the locator or the fixed position; false: none. */
bool               MSG_Position(int32_t *lat, int32_t *lon);
const char        *MSG_SymbolName(uint8_t symbol);   /* "PERSON", "HOUSE", ... */
/* Dual watch hold on the APRS VFO per visit, in ms, 0 = always (msg_port.c). */
uint32_t           MSG_HoldMs(void);
/* Dual watch hold on the voice VFO after voice TX or RX there, in ms. */
uint32_t           MSG_VoiceHoldMs(void);
/* The settings and the mailbox can be written: their flash region holds no
 * other program's data. MSG_ClearMailbox() takes the region over. */
bool               MSG_FlashOk(void);

/* The key from a passphrase ("" removes it and switches encryption off). */
void               MSG_SetKey(const char *pass);
/* 144-146 and 430-440 MHz (IARU Region 1): encryption is refused there. */
bool               MSG_HamBand(uint32_t freq);
/* Encryption is on and may be used on the APRS frequency. */
bool               MSG_EncryptActive(void);

/* Exposed for the host tests. */
void               MSG_ConfigDefaults(msg_config_t *cfg);
const char        *MSG_BeaconMinutesText(uint8_t beacon);   /* "OFF", "10", ... */
uint8_t            MSG_BeaconMinutes(uint8_t beacon);

#endif
