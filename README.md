# <img alt="logo" src="Logo/bambutagger.png" height="36" /> BambuTagger-AMS-C

Multi-spool NFC tag reader for Bambu Lab printers. Reads 4 Bambu Lab filament spool tags via RC522, displays live printer AMS tray data over MQTT, and sends RFID tag data to the printer/BMCU. Fully configurable via web interface with automatic AP fallback and OTA firmware updates.

[![ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/G8M220JASY)

<p align="center">
<img src="Pics/printer.png" width="400px" />
<img src="Pics/status.png" width="400px" />
<img src="Pics/tft01.jpg" width="400px" />
<img src="Pics/pcb-c.png" width="400px"/>
</p>

---

## Features

| Category           | Details                                                                                                                  |
| ------------------ | ------------------------------------------------------------------------------------------------------------------------ |
| **RFID**           | 4x RC522 on shared SPI bus; MIFARE Classic 1K (Bambu Lab) + NTAG (SpoolEase/TigerTag/OpenTag3D/OpenSpool) auto-detection |
| **Key derivation** | HKDF-SHA256 with Bambu Lab salt — no hardcoded keys                                                                      |
| **Tag parsers**    | TigerTag v2.1 binary, OpenTag3D MIME binary, OpenSpool JSON, SpoolEase NDEF URI, nested NDEF recursion                   |
| **Live AMS sync**  | Reads tray data (material, color, type) from printer over MQTT                                                           |
| **BMCU support**   | Sends `ams_filament_setting` with correct `tray_type`, `tray_color`, `nozzle_temp_min/max`                               |
| **TFT display**    | 240×240 1.3" ST7789VW with boot splash, live AMS tray data, OTA progress, BME280 temp/humidity                           |
| **LEDs**           | 4x WS2812 addressable LEDs with per-slot color from printer AMS tray data                                                |
| **Web UI**         | 3-tab SPA: Status (merged slots + color swatches), Printer Config, WiFi Config                                           |
| **MQTT bridge**    | Subscribes to printer status, publishes `ams_filament_setting` commands                                                  |
| **WiFi**           | Auto-STA on boot; AP fallback `192.168.4.1` with captive portal                                                          |
| **OTA updates**    | One-click firmware update from GitHub Releases with TFT progress bar                                                     |
| **CI/CD**          | GitHub Actions: build on commit, release on version tags                                                                 |

---

## Hardware

### Bill of Materials

| Component                      | Notes                                      | Buy                                                                                          |
| ------------------------------ | ------------------------------------------ | -------------------------------------------------------------------------------------------- |
| **ESP32** Dev Module           | Base board                                 | https://de.aliexpress.com/item/1005006589341221.html                                         |
| **4x RC522** RFID/NFC Readers  | SPI interface, shared bus                  | https://de.aliexpress.com/item/1005006233005745.html                                         |
| **4x WS2812** Addressable LEDs | Daisy-chained, single data pin             | https://de.aliexpress.com/item/32560280169.html                                              |
| **240×240 1.3" TFT**           | ST7789VW SPI Display                       | https://www.aliexpress.com/item/1005007094147766.html                                        |
| **BME280** Sensor              | Temperature/Humidity, I2C                  | https://de.aliexpress.com/item/1005006824236173.html                                         |
| Custom **PCB**                 | DIY PCB from JLPCB (use PCB v1.2 or newer) | https://oshwlab.com/bambutagger/project_hdkkdlsn                                             |
| 3D-Printed **Case**            |                                            | For [AMS](./3DPrints/BambuTagger-AMS.3mf) or [AMS 2 Pro](./3DPrints/BambuTagger-AMS2Pro.3mf) |

> [!NOTE]
> The 40-pin ESP32-S3-DevKitC-1 is a compatible alternative for development; however, the PCB only works with the 30-pin ESP-WROOM-32.

### Wiring / Pin Assignments

If you are not using the custom PCB

Connect the ESP32 to a 5V power input, either via the USB port or via the VCC/Vin pin.

The WS2812 LED must also be powered by 5V. If the ESP32 is powered via the USB port, the VCC/Vin pin becomes a 5V output, and can be used to power the LEDs. On some boards, you may need to short two pads (usually labeled VIN->VOUT) via soldering.

All other components will be powered by the 3.3V pin on the ESP32 board.

| Function                    | ESP32-WROOM-32 | ESP32-S3-DevKitC-1 |
| --------------------------- | -------------- | ------------------ |
| **RC522 SPI MOSI (Shared)** | 23             | 11                 |
| **RC522 SPI MISO (Shared)** | 19             | 13                 |
| **RC522 SPI SCK (Shared)**  | 18             | 12                 |
| **RC522 #1 SS / RST**       | 13 / 26        | 10 / 1             |
| **RC522 #2 SS / RST**       | 12 / 25        | 9 / 2              |
| **RC522 #3 SS / RST**       | 14 / 33        | 8 / 4              |
| **RC522 #4 SS / RST**       | 27 / 32        | 7 / 35             |
| **TFT MOSI / SCK**          | 17 / 16        | 21 / 18            |
| **TFT DC / RES / BLK**      | 4 / 5 / 2      | 16 / 17 / 15       |
| **WS2812 Data**             | 15             | 14                 |
| **BME280 SDA / SCL**        | 22 / 21        | 6 / 5              |

---

## Firmware

### Uploading from Web Interface

The easiest way to flash the board firmware is to use the [web flash utility](https://www.bambutagger.de/downloads/firmware/flash-bambutagger-ams-c).

> [!NOTE]
> The ESP32-S3-DevKitC-1 cannot be flashed via the web utility (yet). It must be flashed by building from source.

### Building from Source

To build from source, you may either use Visual Studio Code or run commands in the terminal. The following sections will cover setup for running commands in the terminal.

#### Initial Setup

To setup the local development environment, you can either use Visual Studio Code, or run the `./setup.sh` command. The command will create a Python virtual environment, using the virtualenv manager if one is available, and install dependencies.

> [!NOTE]
> To reactivate the virtualenv at a later time (if you don't have a virtualenv manager), simply run:
>
> ```sh
> source .venv/bin/activate
> ```

#### Build and Upload

```sh
# ESP32-WROOM / ESP32 Dev Module
pio run -e esp32-ams-c
pio run -e esp32-ams-c -t upload

# ESP32-S3-DevKitC-1
pio run -e esp32-s3-devkitc-1
pio run -e esp32-s3-devkitc-1 -t upload
```

---

## WiFi & Connectivity

### AP Mode

| Scenario                     | Behavior                                |
| ---------------------------- | --------------------------------------- |
| No WiFi configured           | Opens AP immediately                    |
| WiFi connection fails        | Opens AP after 15 seconds               |
| AP active, credentials exist | Retries STA connection every 30 seconds |
| STA connects while AP active | Closes AP, switches to normal mode      |

**AP Details:**

- **SSID**: Device name (default: `BambuTagger-AMS`)
- **Security**: Open (no password)
- **IP**: `192.168.4.1`
- **Captive portal**: DNS redirects all domains to the config page

### SSID Mode

When the ESP32 module is connected to an existing network, the IP address will display on the TFT screen. Alternatively, you may navigate to `[Device Name].local` (default: `BambuTagger-AMS.local`).

---

## Web Interface

Open a browser to the ESP32's IP (shown on TFT), `[Device Name].local`, or `http://192.168.4.1` (in AP mode).

| Tab                | Description                                                                                        |
| ------------------ | -------------------------------------------------------------------------------------------------- |
| **Status**         | Merged slot status with AMS data + scanned tag data, color swatches, printer AMS cards, OTA button |
| **Printer Config** | Printer IP, Port (8883), Access Code, Serial Number, AMS Unit selector (A/B/C/D), MQTT settings    |
| **WiFi Config**    | SSID, Password, Device Name                                                                        |

---

## TFT Display (240×240 1.3" ST7789VW)

The TFT display has two modes:

```
┌─────────────────────────────────┐
│ Device Name               WiFi  │  ← status bar (white bg)
├─────────────────────────────────┤
│    1       2                    │
│ ┌─────┐ ┌─────┐ ┌─────┐ ┌─────┐ │  ← color swatch + percentage
│ │  P  │ │     │ │     │ │     │ │
│ │  L  │ │     │ │     │ │     │ │
│ │  A  │ │     │ │     │ │     │ │
│ │     │ │     │ │     │ │     │ │
│ │     │ │     │ │     │ │     │ │
│ └─────┘ └─────┘ └─────┘ └─────┘ │
│                                 │
│  100%                           │
├─────────────────────────────────┤
│ 22C           45%               │  ← temperature, humidity
│ MQTT:OK                  PTR:OK │  ← status line
└─────────────────────────────────┘
```

```
┌─────────────────────────────────┐
│ Device Name               WiFi  │  ← status bar (white bg)
├─────────────────────────────────┤
│ 1: PLA     [■] #C0C0C0FF  100%  │  ← color swatch + percentage
│ 2: empty                        │
│ 3: empty                        │
│ 4: empty                        │
├─────────────────────────────────┤
│ 22C           45%               │  ← temperature, humidity
│ MQTT:OK                  PTR:OK │  ← status line
└─────────────────────────────────┘
```

OTA progress shown on TFT with header/footer preserved:  
"OTA Update" → "Downloading..." → "Flashing... 45%" → auto-reboot

---

## Printer Communication

### Subscribe

- **Topic**: `device/<serial>/report`
- **Data**: `push_status` (periodic, ~3KB), `get_version` responses

### Publish

- **Topic**: `device/<serial>/request`

**`ams_filament_setting`** structure:

```json
{
  "print": {
    "sequence_id": "0",
    "command": "ams_filament_setting",
    "ams_id": 0,
    "tray_id": 0,
    "tray_info_idx": "GFA00",
    "tray_color": "RRGGBBFF",
    "nozzle_temp_min": 190,
    "nozzle_temp_max": 230,
    "tray_type": "PLA"
  }
}
```

- `tray_type` derived from filament index prefix (GFA→PLA, GFG→PETG, etc.)
- `tray_color` forced to RRGGBBFF format
- `nozzle_temp_min/max` from tag block 6

---

## Tag Format & Reading

### Supported Tag Types

| Tag Type                  | Format            | Example                                              |
| ------------------------- | ----------------- | ---------------------------------------------------- |
| **Bambu Lab**             | MIFARE Classic 1K | `Bambu - PLA · C12E1FFF · 1000g/1000g`               |
| **SpoolEase**             | NTAG, NDEF URI    | `SpoolEase - PLA · 000000FF · 1000g/1036g`           |
| **TigerTag**              | raw binary v2.1   | `TigerTag - ASA-AF · F078B4FF · 1000g/1000g`         |
| **OpenSpool**             | NTAG, NDEF JSON   | `OpenSpool - ASA-AF · F078B4FF · 1000g/1000g`        |
| **OpenTag3D** (v1 and v2) | NTAG, MIME binary | `OpenTag3D v2.003 - ASA-AF · F078B4FF · 1000g/1000g` |

### Bambu Lab (MIFARE Classic 1K)

| Block | Content                                              |
| ----- | ---------------------------------------------------- |
| 0     | UID (4 bytes)                                        |
| 1     | Variant ID + Material index (e.g. "GFA00")           |
| 2     | Filament type short name                             |
| 4     | Detailed type string (e.g. "PLA Basic")              |
| 5     | RGBA color (bytes 0-3) + spool weight LE (bytes 4-5) |
| 6     | Nozzle temps (bytes 8-11 LE)                         |

### SpoolEase (NTAG, NDEF URI)

URL format: `https://tag.spoolease.io/S1/?TG=...&M=PLA&CC=000000FF&SC=GFL99&WL=1000&WE=179&WF=1215&NN=190&NX=240`

| Param | Field            | Description                         |
| ----- | ---------------- | ----------------------------------- |
| `M=`  | display type     | e.g. "PLA", "PETG"                  |
| `SC=` | `materialType`   | Bambu index for MQTT (e.g. "GFL99") |
| `CC=` | `colorHex`       | RGBA hex (e.g. "000000FF")          |
| `B=`  | `manufacturer`   | Brand name (e.g. "Jayo")            |
| `WL=` | `remainingGrams` | Remaining filament weight           |
| `WE=` | empty spool      | Empty spool weight                  |
| `WF=` | full spool       | `totalGrams = WF - WE`              |
| `NN=` | `nozzleTempMin`  | Min nozzle temp °C                  |
| `NX=` | `nozzleTempMax`  | Max nozzle temp °C                  |

### TigerTag (raw binary v2.1)

| Offset | Size | Field                                                |
| ------ | ---- | ---------------------------------------------------- |
| +0     | 4    | ID TigerTag magic (0x5BF59264/0xBC0FCB97/0x6C41A2E1) |
| +4     | 4    | Product ID                                           |
| +8     | 2    | Material ID → lookup table (PLA/PETG/ABS/TPU...)     |
| +14    | 2    | Brand ID                                             |
| +16    | 4    | Color 1 RGBA                                         |
| +20    | 3    | Measure (u24 BE)                                     |
| +24    | 2    | Nozzle Temp Min                                      |
| +26    | 2    | Nozzle Temp Max                                      |
| +76    | 3    | Measure Available                                    |

Known material IDs: PLA=38219, PETG=38256, ABS=20562, etc.

### OpenTag3D (NTAG, MIME binary)

The OpenTag3D spec can be read on [their website](https://opentag3d.info/spec).

### Authentication & Reading

- **Tag auto-detect**: SAK-based type detection (MIFARE 1K vs NTAG)
- **HKDF-SHA256** derives 16 per-sector Key A/B from 4-byte UID
- Bambu KDF salt/info vectors from reverse-engineered firmware
- Falls back to default key `0xFF...FF` for blank sectors
- Failed auth re-wakes tag via antenna power-cycle
- Dead readers auto-skipped (version register 0x92/0x91/0xB2 check)
- SPI: 1 MHz via `MFRC522_SPI`
- NTAG: page-level reads (page+=4, 4 pages per MIFARE_Read)

### Filament Type Mapping

| Prefix       | Type |
| ------------ | ---- |
| GFA-GFE, GFL | PLA  |
| GFG          | PETG |
| GFH, GFI     | ABS  |
| GFJ          | ASA  |
| GFK          | TPU  |

---

## OTA Updates

- **Button**: "Update Firmware" on Status page (shows "Update to vX.Y.Z" or "up to date")
- **Overlay**: full-screen progress overlay with spinner, status text, progress bar
- **Endpoint**: `POST /api/ota` triggers update, `GET /api/ota-check` checks for newer version
- Downloads latest `.ino.bin` from GitHub Releases, flashes via `Update.h`
- 3 retry attempts with 5s stall detection, fresh HTTP client per attempt
- TFT shows "Checking version..." → "Downloading..." → "Flashing..." with percentage
- Device auto-reboots after successful flash, web UI auto-reloads

---

## CI / CD

Workflow at `GHActions/release.yml`:

- **On push/PR**: compiles sketch, uploads artifacts
- **On release tag**: creates merged flash binary + OTA binary, attaches to GitHub Release
- Arduino cache for fast rebuilds, pinned esp32:esp32@3.0.7 core

---

## Configuration Defaults

| Setting              | Default         |
| -------------------- | --------------- |
| WiFi SSID            | (empty)         |
| WiFi Password        | (empty)         |
| Device Name          | BambuTagger-AMS |
| Printer IP           | 192.168.1.100   |
| Printer Port         | 8883            |
| Access Code          | (empty)         |
| Printer Serial       | (empty)         |
| AMS Unit             | A (0)           |
| MQTT Enabled         | Yes             |
| MQTT TLS             | No              |
| MQTT Update Interval | 3000 ms         |
| RFID Poll Interval   | 100 ms          |
| Firmware Version     | 1.0.8           |

---

## Credits & References

- [RFID-Tag-Guide](https://github.com/Bambu-Research-Group/RFID-Tag-Guide)
- Display library: [Adafruit ST7735/ST7789](https://github.com/adafruit/Adafruit-ST7735-Library)
- RFID library: [MFRC522-spi-i2c-uart-async](https://github.com/miguelbalboa/rfid)
- OpenTag3D spec: [opentag3d.info](https://opentag3d.info)

---

## License

This project is provided as-is for personal and educational use.  
Bambu Lab trademarks and spool tag data formats are the property of Bambu Lab.
