# Pocuter One – ESPHome

ESPHome config for the Pocuter One (ESP32-C3, 96×64 RGB OLED, board rev 5):
a clock on the OLED plus the LED, buttons, charger, battery, light sensor and
accelerometer in Home Assistant.

## Device

- Pocuter One: ESP32-C3-MINI-1 with 4 MB flash, no PSRAM. ESPHome with the
  ESP-IDF framework, board `esp32-c3-devkitm-1`.
- One config for every board: `name_add_mac_suffix` names each one
  `pocuter-xxxxxx` after the last three bytes of its MAC. That is also its
  hostname (`pocuter-xxxxxx.local`).
- API without encryption or password, OTA (`platform: esphome`) without
  password, fallback access point `Pocuter Setup` without password.
- Logs and USB flashing go over the ESP32-C3's native USB port (`303a:1001`,
  `/dev/ttyACM0` on Linux). The board has no USB serial chip; auto reset into
  the bootloader works, no button needed.
- Besides the three front buttons there are two small ones: reset next to the
  USB port, and BOOT near the header pins. Hold BOOT while pressing reset to
  force the bootloader.

## Files

- `pocuter.yaml` – the complete device config. Pin assignments sit next to
  each part; the expander pin map is the comment block under `aw9523b:`.
- `components/aw9523b/` – AW9523B expander driver, see [AW9523B](#aw9523b).
- `components/mxc4005/` – MXC4005XC accelerometer, X/Y/Z in m/s² plus the
  die temperature. Polled; its INT line is not used.
- `secrets.yaml` – `wifi_ssid` / `wifi_password` / `timezone` (git-ignored,
  copy from `secrets.yaml.example`). The timezone is only the fallback for a
  boot without Home Assistant, which pushes its own on every time sync.
- `docs/PCT-V1-REV5_schematics.pdf` – board schematic, see [License](#license).
- `mockups/` – clock face and font studies rendered at the panel's 96×64.

## Build / flash

```sh
python3 -m venv venv            # once
venv/bin/pip install esphome    # once
venv/bin/esphome config pocuter.yaml
venv/bin/esphome run pocuter.yaml --device /dev/ttyACM0            # USB
venv/bin/esphome run pocuter.yaml --device pocuter-xxxxxx.local    # OTA
venv/bin/esphome logs pocuter.yaml --device pocuter-xxxxxx.local
```

A board still on the original PocuterOS holds the only copy of it. Dump the
flash before the first ESPHome flash if you might want it back (`--no-stub`
is slower, but the stub flasher dropped out partway on one board):

```sh
venv/bin/esptool --port /dev/ttyACM0 --no-stub read-flash 0 0x400000 pocuter-original.bin
venv/bin/esptool --port /dev/ttyACM0 write-flash 0 pocuter-original.bin   # restore
```

A build that breaks WiFi needs USB again; the fallback access point is the
safety net.

## Adding to your ESPHome / Home Assistant

1. Put `pocuter.yaml` and `components/` into the ESPHome dashboard config dir
   (e.g. `/config/esphome/`). The dashboard `secrets.yaml` must define
   `wifi_ssid`, `wifi_password` and `timezone`.
2. The dashboard shows the device online via mDNS (`pocuter-xxxxxx.local`);
   install from there works via OTA.
3. Home Assistant: the ESPHome integration discovers it, or add it manually
   with host `pocuter-xxxxxx.local`, port `6053`. No encryption key is needed.

To enable API encryption, add `api: encryption: key: <base64 key>`, flash,
then enter the key in Home Assistant.

## Clock

- Four digits `HHMM` in [Jersey 10](https://fonts.google.com/specimen/Jersey+10),
  no colon. Once a second the number drops from 50 px to 40 px for 150 ms and
  back, a beat instead of a blinking colon. Both sizes are exact multiples of
  the font's 10 unit pixel grid, so they stay sharp at 1 bpp.
- 50 px is the largest such size where the widest time (23:39, 86 px) fits.
  The digits are proportional, so the number is centred as a whole and moves
  a little as the time changes.
- `----` until Home Assistant or SNTP answers. There is no RTC on the board.
- Bottom left: battery voltage, `+` while charging, only with a cell fitted.
- Dot bottom right: green with a Home Assistant client connected, amber on
  WiFi only, red offline.
- The display has no `update_interval`; an `interval` draws the two frames of
  each beat (about 33 ms each) and skips them while the Display switch is
  off. Fonts are Google fonts downloaded at compile time, so compiling needs
  internet.

## Entities

| Entity | Type | Notes |
|---|---|---|
| Display | switch | 12.5 V panel supply; off also stops drawing. The picture comes back as it was |
| LED | light (RGB) | AW9523B LED current mode, pulse effect |
| Button 1–3 | binary sensor | Pressed state, 20 ms debounce, no behavior attached |
| Charging | binary sensor | Charger STAT low for more than 1 s, see [Battery](#battery) |
| Battery Present | binary sensor (diagnostic) | See [Battery](#battery) |
| Battery Voltage / Battery | sensor (diagnostic) / sensor | VBAT via 1.2M/1.2M divider; % is linear 3.3–4.15 V, unknown without a cell |
| Light Sensor | sensor (V) | Photodiode + MCP6001, 0 dB attenuation. 4–8 mV in room light, about 60 mV under a phone torch; dark/bright, not lux |
| Acceleration X / Y / Z | sensor (m/s²) | Every 10 s. Z has a per-board zero-g offset, see [Accelerometer](#accelerometer) |
| Board Temperature | sensor (diagnostic) | The accelerometer's die, tracks the board rather than the room |
| SD Card | binary sensor (diagnostic) | Card detect only |
| WiFi Signal | sensor (diagnostic) | |

### Not exposed

- **Microphone**: needs a new component, see [Microphone](#microphone).
- **SD card storage**: ESPHome has no SPI SD card file component; only card detect is exposed.
- **Accelerometer interrupt** (shake / orientation, GPIO7): not used.
- **Header pins**: P0 (GPIO4) and P5 (GPIO3) are free native GPIOs, P1–P4 are expander pins left as inputs.
- **Deep sleep**: incompatible with an always-connected Home Assistant device.
- **Speaker / buzzer**: the board has none.

## Hardware notes

| Function | GPIO | Notes |
|---|---|---|
| I2C SDA / SCL | 8 / 9 | 400 kHz, 2.7k pullups. AW9523B at **0x5B**, MXC4005XC at 0x15. GPIO9 is also the BOOT strap: the small button near the header pins |
| SPI CLK / MOSI / MISO | 5 / 6 / 10 | OLED and micro SD. `ssd1331_spi` at 8 MHz |
| AW9523B INT | 20 | Open drain, 10k pullup. UART0 RX, hence logs over USB |
| SD CS | 21 | Not used. UART0 TX |
| Accelerometer INT | 7 | Not used |
| Microphone / light / VBAT÷2 | 0 / 1 / 2 | ADC1 channels 0 / 1 / 2 |
| Header P0 / P5 | 4 / 3 | Free |

| Expander pin | Port | Function |
|---|---|---|
| 0 / 10 / 11 | P0_0 / P1_2 / P1_3 | LED blue / red / green, LED current mode, 2.7k series resistors, 9.25 mA ceiling |
| 1 / 5 / 6 | P0_1 / P0_5 / P0_6 | Button 3 / 2 / 1, active low, 10k pullups |
| 2 | P0_2 | OLED 12.5 V boost enable |
| 3 | P0_3 | MCP73831 STAT |
| 4 | P0_4 | SD card detect, low = card present |
| 7 / 12 / 13 | P0_7 / P1_4 / P1_5 | OLED CS / reset / D/C |
| 8 / 9 / 14 / 15 | P1_0 / P1_1 / P1_6 / P1_7 | Header P2 / P1 / P4 / P3 |

The OLED is an SSD1331, 96×64 (the vendor library calls it "SSD1131").

### AW9523B

ESPHome has no AW9523 support as of 2026.9
([feature request #2263](https://github.com/esphome/feature-requests/issues/2263)).
`components/aw9523b/` is M5Stack's driver from
[m5stack/esphome-yaml](https://github.com/m5stack/esphome-yaml) at `cc708dd`
with two fixes, both in `aw9523b.h`:

- Upstream starts its LED mode shadow at `0x0000`, so the first LED output
  turns all 16 pins into LED sinks: dark display, every input reads 0. It now
  starts at the chip's reset value, all GPIO.
- Upstream starts its direction shadow at "all outputs", which drives every
  pin nobody configured low, header pins included. It now starts at all
  inputs.

Also needed on this board:

- `p0_drive_mode: PUSH_PULL`. The chip starts port 0 open drain, and nothing
  pulls up the OLED CS or the panel supply enable.
- The soft reset (`reset: true`, the default). The expander's reset line is on
  EN, which the USB reset does not toggle, so its state survives reflashing.
  With the broken driver above, it kept the wrong LED mode across reboots.

### Display

- The controller runs on 3.3 V and answers every command without the 12.5 V
  panel supply, so a missing supply enable looks like a dead display.
- CS and D/C are expander pins, so every SSD1331 command costs I2C writes, and
  ESPHome's `ssd1331_spi` adds a 1 ms delay per command. Measured with a
  free-running display: 30 fps at 8 MHz SPI, 42 fps at 40 MHz. About 21 ms per
  frame is that fixed overhead, and a free-running display owns the main loop.
- Brightness is 60 % to go easy on the OLED.

### Battery

| Entity | Source |
|---|---|
| Battery Voltage | ADC on GPIO2, VBAT through 1.2M/1.2M, 12 dB, sampled every 10 s, median of 6 published every 60 s. The first sample after idle reads low, the median hides it |
| Charging | MCP73831 STAT (expander P0_3) held low for more than 1 s |
| Battery Present | False if STAT blipped low in the last 5 minutes |
| Battery | Voltage mapped linearly 3.3–4.15 V; unknown without a cell |

There is no presence pin for the cell. With none fitted, the MCP73831 holds
VBAT at 4.20–4.22 V, which looks like a full cell, and keeps restarting into
the empty output: STAT drops for 3–20 ms about every 10 s. That blip is the
only usable signal. The expander only sees it if its read lands inside the
pulse, so some are missed, hence the 5 minute window. A newly fitted cell
takes up to 5 minutes to show as present.

### Accelerometer

On one board, face up reads about −16.5 m/s² on Z and face down about +1.7.
The spread, 18.2 m/s², is close to the expected 19.6 (2 g), so the scale is
right and Z sits about −7.4 m/s² off. It is per board, so it is not in
`pocuter.yaml`. Measure yours the same way and add `filters: [offset: 7.4]` to
`acceleration_z` if absolute values matter.

### Microphone

An analog MEMS mic and a one-transistor gain stage into GPIO0. No codec, no
I2S, no anti-alias filter. ESPHome has no way in: `i2s_audio` internal ADC
mode only ever supported the original ESP32 and is removed in 2026.9. It needs
a component that samples GPIO0 with the ESP-IDF continuous (DMA) ADC driver
and exposes it as a `microphone`; ESPHome's `sound_level` can then turn that
into relative dBFS sensors.

The catch is ADC1: the continuous driver holds the ADC1 lock from start to
stop, and Battery Voltage and Light Sensor are ADC1 one-shot reads.

| Option | How | Cost |
|---|---|---|
| Bursts | Sample about 100 ms every few seconds, release ADC1 in between | Least code, other sensors unchanged. Misses short sounds |
| One ADC1 owner | One continuous scan over all three channels | Continuous level. Most code, replaces both `adc` sensors |
| Audio stream | 16 kHz `microphone` for push-to-talk | Poor audio, no speaker for replies, no `micro_wake_word` on a single-core C3 without PSRAM |

The vendor Pocuter library runs the mic only between `startRecording()` and
`stopRecording()`: ADC1 channel 0 in continuous DMA mode at 0 dB, 8–48 kHz,
running DC removal, and a gate that zeroes quiet blocks. It does not handle
ADC1 sharing. Worth keeping: 0 dB attenuation and DC removal before any level
calculation.

## License

MIT, see [LICENSE](LICENSE), except:

- `components/aw9523b/` is M5Stack's driver, MIT, see
  [its LICENSE](components/aw9523b/LICENSE).
- `docs/PCT-V1-REV5_schematics.pdf` is by Pocuter GmbH, released under
  [CC BY-NC-ND 4.0](https://creativecommons.org/licenses/by-nc-nd/4.0/) and
  included unmodified, see [docs/README.md](docs/README.md).

Firmware built from this links ESPHome's C++ runtime, which is GPLv3. Tiles A
and B of `mockups/fonts-dots.png` show Nothing's Ndot typeface for comparison
only; the font is not part of this project.
