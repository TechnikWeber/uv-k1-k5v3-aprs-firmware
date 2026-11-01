# APRS-Messenger: Kurzanleitung

*English: [QUICKSTART.md](QUICKSTART.md). Ausführliche Anleitung (englisch): [README.md](README.md).*

Alpha-Firmware: Die Funkteile sind noch nicht auf einem Gerät getestet. Senden
erfordert eine Amateurfunklizenz; das Gerät sendet Bestätigungen und Baken
selbstständig, deshalb immer mit Antenne betreiben.

## 1. Flashen

1. Mit der offiziellen F4HWN-Fusion auf dem Gerät die Kalibrierung in
   [UV Studio](https://armel.github.io/uvstudio/#dump-calib) sichern (einmal
   pro Gerät).
2. Die Messenger-Version über den
   [Web-Flasher](https://technikweber.github.io/uv-k1-k5v3-aprs-firmware/)
   flashen. UV-K1 und UV-K5 V3 nutzen dieselbe Datei (nicht für UV-K5 V1/V2).
3. UV-K1: Die Pfeiltasten sind LINKS / RECHTS. Überall unten gilt
   OBEN = LINKS, UNTEN = RECHTS.

## 2. Der APRS-Kanal auf VFO B

Der Messenger hört und sendet auf einem VFO, den du selbst auf die
APRS-Frequenz stellst; er verstellt nie einen VFO. VFO B so einstellen (oder
einmal als Speicherkanal `APRS` mit **ChSave** / **ChName** ablegen, geht auch
mit dem CHIRP-Treiber dieses Forks):

| Menü | Wert | Warum |
|---|---|---|
| Frequenz | **144,800** (Europa/Afrika; Nordamerika 144,390, Australien 145,175) | muss *Freq* im Messenger-Setup entsprechen |
| **Mode** | FM | AM/USB können kein APRS |
| **W/N** | WIDE | APRS nutzt Breitband-FM |
| **TxODir** | OFF | Simplex: mit Ablage verweigert der Messenger das Senden |
| **RxCTCS / RxDCS** | OFF | eine Subton-Sperre würde die Pakete stummschalten |
| **TxCTCS / TxDCS** | OFF | kein Subton unter den Daten |
| **Compnd** | OFF | der Kompander verzerrt die Töne |
| **Power** | MID oder HIGH | um den nächsten Digipeater zu erreichen (LOW für Tests) |
| **Sql** | 1 oder 2 | der Messenger wartet auf einen freien Kanal |

**F + 2** wechselt zwischen VFO A und B, **F + 3** zwischen Frequenz- und
Speicherbetrieb; eine Frequenz tippst du direkt auf der Tastatur ein.

## 3. Sprechfunk und APRS zusammen

- VFO A: dein Sprechfunkkanal, als Haupt-VFO gewählt (**F + 2**).
- **RxMode** = `MAIN TX / DUAL RX`: Die Sprechtaste sendet immer auf VFO A.
  Nicht `DUAL RX / RESPOND` (die Sprechtaste könnte auf 144,800 senden) und
  nicht `CROSS BAND`.
- Nur APRS: **RxMode** = `MAIN ONLY` mit VFO B als Haupt-VFO.
- Das Gerät hat einen Empfänger. *APRS hold* im Messenger-Setup legt fest, wie
  er geteilt wird (siehe unten).

## 4. Der Messenger

1. Menü → **F1Shrt** / **F1Long** / **F2Shrt** / **F2Long** / **M Long** →
   **MESSAGES**: Diese Taste öffnet den Messenger.
2. Erster Start: Rufzeichen ohne SSID eintippen, MENU. Dann *Messenger* →
   MENU = ON.
3. Optional: *Channel*, *Locator*, *IS net* (unten).

### Einstellungen (F im Postfach)

| Punkt | Bedeutung | Empfehlung |
|---|---|---|
| Messenger | Empfang im Hintergrund an/aus | ON |
| Call / SSID | deine Station; SSID 7 = Handfunkgerät, 9 = mobil, 0 = keine | 7 |
| Channel | deine Gruppe (1-5 Buchstaben/Ziffern); Rundsprüche erreichen alle Geräte mit demselben Channel (APRS-Gruppen-Bulletins) | nach Absprache |
| Public BLN | auch öffentliche Bulletins behalten (Clubnews, Runden) | OFF |
| Lobby CQ | Nachrichten an CQ, QST, ALL behalten (Gruppencodes von Yaesu/Kenwood) | ON |
| IS net | ANSRVR-Gruppe im Internet (z. B. `HOTG`); MENU bearbeitet, **F tritt bei** | leer |
| Beep | ALL (privat, Channel, Lobby), PRIVATE, OFF | ALL |
| Freq | Frequenz, auf der der Messenger arbeitet; MENU zum Eintippen, F schaltet durch Vorgaben | = VFO B |
| APRS hold | ALWAYS: Empfänger bleibt auf VFO B. 10/30/60 s: danach kurzer Blick auf VFO A | ALWAYS |
| Encrypt / Key | Verschlüsselung, nur außerhalb der Amateurfunkbänder (auf 144-146 / 430-440 MHz gesperrt) | OFF |
| Path | DIRECT, WIDE1, WIDE1+2 (Digipeater-Sprünge) | WIDE1+2 |
| Beacon | OFF, 10, 20, 30, 60 min; iGates liefern Internet-Nachrichten nur an Stationen, die sie gehört haben | 30 min |
| Status | Bakentext (auch Kommentar der Position) | freier Text |
| Position | OFF, LOCATOR, FIXED: Position in der Bake | LOCATOR |
| Locator / Lat / Lon | z. B. `JO62QM`; oder `485090` = 48°50,90' (F wechselt N/S, O/W) | dein Locator |
| Symbol | Kartensymbol: PERSON, HOUSE, CAR, BIKE, BOAT, PORTABLE | PERSON |
| APRS audio | ON: Pakete im Lautsprecher hören | OFF |
| Clear mailbox | alle Nachrichten löschen (mit Rückfrage) | |

Die unterste Zeile zeigt Messwerte für Tests: gehörte (RX) und gesendete (TX)
Pakete, Decoder-Last (DSP), Audiopegel (LV).

## 5. Tasten

| Bildschirm | Tasten |
|---|---|
| Postfach | OBEN/UNTEN wählen · MENU lesen · `*` neu · `1` Chat mit dieser Station/diesem Channel · `2` gehörte Stationen · F Setup · EXIT zurück |
| Lesen | OBEN/UNTEN blättern · MENU antworten (eigene Nachricht ohne Bestätigung: erneut senden) · F privat auf eine Gruppennachricht antworten · `0` löschen |
| Schreiben | `0`-`9` Mehrfachtippen (`2` = a b c 2) · `*` Abc/abc/ABC/123 · OBEN/UNTEN Cursor · EXIT löschen, lang: abbrechen · MENU weiter / senden |
| Empfänger | ein Rufzeichen, oder **F**: dein Channel → `#ALL` (öffentliches Bulletin) → `CQ` (Lobby) → `ANSRVR` (IS net, Text beginnt mit "CQ Gruppe ") |

## 6. Was die Anzeige sagt

- Statuszeile **`APRS`**: Der Messenger hört gerade auf VFO B. Briefumschlag:
  ungelesene Nachrichten.
- Markierungen: `*` neu · `>` wird gesendet (Wiederholung nach 30, 60, 120,
  240 s) · `+` zugestellt · `!` keine Bestätigung oder abgelehnt (MENU sendet
  erneut) · `#` Gruppe oder Lobby.
- Warnungen unten im Postfach:

| Warnung | Abhilfe |
|---|---|
| `NO VFO ON 144.800` | einen VFO auf *Freq* stellen |
| `SELECT APRS VFO OR DUAL WATCH` | Dual Watch ist aus (oder das UKW-Radio läuft) und VFO B ist nicht Haupt-VFO |
| `TX BLOCKED: CHECK FM/SIMPLEX/LOCK` | Mode, TxODir oder TX-Sperre von VFO B prüfen |
| `TX BLOCKED: BATTERY` | Akku laden |
| `FLASH IN USE: CLEAR MAILBOX` | fremde Daten im Flash-Bereich des Messengers; *Clear mailbox* übernimmt ihn |

## 7. Gut zu wissen

- **Ein Empfänger:** Während du auf VFO A sprichst oder hörst, oder während
  eines Suchlaufs, ist APRS taub. Verpasste Nachrichten kommen meist mit der
  nächsten Wiederholung des Absenders.
- **APRS hold ALWAYS** hält den Empfänger auf VFO B: Du sendest weiter auf
  VFO A, hörst ihn aber nur etwa 4 s nach deiner eigenen Sendung. Mit
  10/30/60 s hörst du VFO A mit, dafür hat APRS Lücken. Während eines Pakets
  oder solange eine Bestätigung wartet, springt der Messenger nie weg.
- **UKW-Radio:** Die Firmware pausiert Dual Watch; vorher VFO B als Haupt-VFO
  wählen, sonst ist APRS taub.
- **Akku:** Der Stromsparmodus ist aus, solange ein VFO auf der APRS-Frequenz
  steht.
- **Automatische Sendungen:** Bestätigungen 0,7-3 s nach freiem Kanal, Baken,
  Wiederholungen; höchstens 6 Pakete am Stück, danach eines pro 30 s.
- **Grenzen:** 67 Zeichen pro Nachricht (40 verschlüsselt); Umlaute werden zu
  ae/oe/ue/ss; bis zu 480 Nachrichten, bei vollem Speicher gehen die ältesten
  32; keine Uhrzeiten (das Gerät hat keine Uhr).
- **Nachrichten aus dem Internet** (aprs.fi, APRSdroid über APRS-IS) brauchen
  ein iGate, das dich kürzlich gehört hat: Bake eingeschaltet lassen.
- **CHIRP:** den Treiber dieses Forks verwenden,
  `chirp/f4hwn.messenger.chirp.py`; der F4HWN-Treiber setzt die
  MESSAGES-Taste zurück.
- **Nicht in dieser Version:** Spektrumanalysator, RF Log, QR-Codes,
  Logo-Bildschirmschoner, Overlay-Apps (F4HWN Labs kann in einem
  Multiboot-Slot liegen).
