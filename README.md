# ESP32-S3 I2C Clock (DS1307 + SSD1306)

A small learning project that reads the current time and date from a **DS1307**
real-time clock (RTC) over I2C and displays it on a **128×64 SSD1306** OLED.
Both devices share a single I2C bus. Written in C for **ESP-IDF** and built with
**PlatformIO**.

The display shows:

```
   15:33:59          <- time, large font (HH:MM:SS)
 Mon 29.06.2026      <- weekday + date (EEE DD.MM.YYYY)
```

The screen refreshes once per second, and the same line is mirrored to the
serial monitor.

## Hardware

- **Board:** Freenove ESP32-S3-WROOM
- **RTC:** DS1307 (I2C address `0x68`)
- **Display:** SSD1306 128×64 OLED (I2C address `0x3C`)

### Wiring

Both devices sit on the same I2C bus (shared SDA/SCL and common ground).

| Signal | ESP32-S3 | DS1307  | SSD1306 |
| ------ | -------- | ------- | ------- |
| SCL    | GPIO 9   | SCL     | SCL     |
| SDA    | GPIO 8   | SDA     | SDA     |
| VCC    | —        | **3V3** | **3V3** |
| GND    | GND      | GND     | GND     |

## Project structure

```
include/
  ds1307.h     # RTC public API + rtc_time_t struct
  ssd1306.h    # Display public API + geometry
  font5x7.h    # 5x7 bitmap font table
src/
  ds1307.c     # DS1307 driver (read/write, BCD conversion)
  ssd1306.c    # SSD1306 driver (framebuffer, init, text rendering)
  main.c       # App logic: I2C init, display_clock, app_main
  CMakeLists.txt
platformio.ini
```

The code is organized in three layers:

- **`main.c`** — application logic. Initializes the bus, then loops:
  read time → render to display, once per second.
- **`ds1307.c` / `ssd1306.c`** — self-contained device drivers. Internal
  details (register addresses, framebuffer, low-level I2C transactions) are
  `static` and hidden behind the headers.
- **`driver/i2c.h`** — the ESP-IDF I2C peripheral driver underneath.

## Building and flashing

Requires [PlatformIO](https://platformio.org/).

```bash
pio run                 # build
pio run -t upload       # build + flash
pio device monitor      # open serial monitor (115200 baud)
pio run -t upload -t monitor   # all-in-one
```

## Setting the time

The DS1307 always powers up at `01.01.2000 00:00:00` until you write a real
time to it once. In `app_main()` (in `src/main.c`) there is a one-shot call:

```c
ds1307_set(12, 0, 0, /*Mon*/2, 29, 6, 26);
// hours, minutes, seconds, day_of_week (1=Sun..7=Sat), date, month, year(20XX)
```

To set the clock:

1. **Uncomment** the `ds1307_set(...)` line.
2. Edit it to your current time (account for the few seconds it takes to
   compile and flash — set it slightly ahead).
3. Build and flash.
4. **Re-comment** the line and flash again — otherwise the time resets to those
   values on every reboot.

With a backup battery installed, the DS1307 keeps time across power cycles, so
you only need to set it once.

## Configuration

Key settings live at the top of the source files:

| Setting         | File        | Default |
| --------------- | ----------- | ------- |
| I2C SCL pin     | `main.c`    | GPIO 9  |
| I2C SDA pin     | `main.c`    | GPIO 8  |
| I2C frequency   | `main.c`    | 100 kHz |
| DS1307 address  | `ds1307.c`  | `0x68`  |
| SSD1306 address | `ssd1306.c` | `0x3C`  |
| Display size    | `ssd1306.h` | 128×64  |

## Notes

- The DS1307 stores only a two-digit year (`00–99`); the century (`20xx`) is
  added in software, so dates are valid for 2000–2099.
- The embedded font covers ASCII `0x20`–`0x7A` (digits, punctuation, and the
  letters needed for weekday names). Out-of-range characters render as `?`.
- The weekday register (`0x03`) is a user-defined 1–7 counter. This project
  uses the convention `1 = Sunday … 7 = Saturday`.
