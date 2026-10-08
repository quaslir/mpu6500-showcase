# mpu6500-demos

Demo firmware for the [mpu6500-rp2040](https://github.com/quaslir/mpu6500-rp2040) driver on the Raspberry Pi Pico (RP2040).

| Demo | What it does | Output |
|------|--------------|--------|
| `web_viewer_firmware` | Streams raw accel + gyro samples at 200 Hz over USB serial. A browser page computes the orientation (Madgwick) and draws it in 3D. | `web_viewer_firmware.uf2` |
| `oled_viewer` | Computes the orientation on the Pico (complementary filter + integrated yaw) and draws a rotating 3D board on an SSD1306 OLED. | `oled_viewer.uf2` |

The driver and the [pico-ssd1306](https://github.com/daschr/pico-ssd1306) library are downloaded automatically by CMake (`FetchContent`); nothing has to be installed by hand except the toolchain.

![Web viewer: the board orientation follows the sensor in real time](docs/media/mpu6500-web-view.gif)

## Requirements

- [Pico SDK](https://github.com/raspberrypi/pico-sdk) 2.0 or newer, with `PICO_SDK_PATH` set
- CMake 3.20+
- `arm-none-eabi-gcc` toolchain
- [picotool](https://github.com/raspberrypi/picotool) (optional, for flashing from the terminal)
- For the web viewer: a Chromium-based browser (Chrome, Edge), because the page uses the Web Serial API

## Hardware

- Raspberry Pi Pico
- MPU6500 module (SPI)
- SSD1306 128×64 I2C OLED (only for `oled_viewer`)

### Wiring

| Signal | Pico pin | Connected to |
|--------|----------|--------------|
| SPI0 SCK | GP2 | MPU6500 SCL/SCLK |
| SPI0 TX | GP3 | MPU6500 SDA/SDI |
| SPI0 RX | GP4 | MPU6500 AD0/SDO |
| CS | GP15 | MPU6500 NCS |
| I2C1 SDA | GP6 | SSD1306 SDA |
| I2C1 SCL | GP7 | SSD1306 SCL |
| 3V3 | 3V3(OUT) | VCC of both modules |
| GND | GND | GND of both modules |

The SSD1306 is expected at I2C address `0x3C`. Some modules use `0x3D`; change `DISPLAY_ADDRESS` in `common/i2c_config.hpp` if the firmware reports that the display is not found.

See [docs/wiring.md](docs/wiring.md) for photos and details.

## Build

```bash
git clone https://github.com/quaslir/mpu6500-demos.git
cd mpu6500-demos
mkdir build && cd build
cmake ..
make -j
```

The `.uf2` files end up in `build/<demo>/`.

To build against another version of the driver:

```bash
cmake .. -DMPU6500_GIT_TAG=main
```

## Flash

Either hold BOOTSEL while plugging in the Pico and copy the `.uf2` to the `RPI-RP2` drive, or use picotool:

```bash
picotool load -f -x build/oled_viewer/oled_viewer.uf2
```

`-f` reboots a running Pico into BOOTSEL, `-x` starts the firmware after loading.

## Usage

### Web viewer

1. Flash `web_viewer_firmware.uf2`.
2. Keep the board still for about a second after power-up: the gyro is calibrated at startup.
3. Open `viewer/imu_viewer.html` in Chrome or Edge and connect to the Pico's serial port.

Serial format, one sample per line at 200 Hz:

```
t_us,ax,ay,az,gx,gy,gz
```

`t_us` is microseconds since boot, acceleration is in g, angular rate in °/s. Lines starting with `#` are status messages and are ignored by the viewer. The port can be read by one program at a time, so close any serial monitor before connecting the viewer.

### OLED viewer

1. Flash `oled_viewer.uf2`.
2. The display shows "Calibrating... Keep still" while the gyro is calibrated.
3. Afterwards it shows the board as a 3D box with an arrow along the sensor's +X axis, plus roll / pitch / yaw in degrees.

Yaw is integrated from the gyro only (the MPU6500 has no magnetometer), so it slowly drifts.

## Project layout

```
mpu6500-demos/
├── CMakeLists.txt          fetches the driver and pico-ssd1306, adds the demos
├── common/                 pin and bus configuration shared by the demos
│   ├── i2c_config.hpp
│   └── spi_config.hpp
├── web_viewer_firmware/    USB serial streaming firmware
├── oled_viewer/            on-device orientation + SSD1306 rendering
├── viewer/
│   └── imu_viewer.html     browser 3D viewer
└── docs/
    ├── wiring.md
    └── media/
```

## Troubleshooting

- **`Device or resource busy` on `/dev/ttyACM0`**: another program has the port open (serial monitor, browser tab).
- **`Permission denied` on `/dev/ttyACM0`** (Arch Linux): add yourself to the `uucp` group and log in again: `sudo usermod -aG uucp $USER`.
- **Changed `MPU6500_GIT_TAG` but nothing changed**: the old value is cached. Delete `build/CMakeCache.txt` and `build/_deps`.
- **`init failed, check wiring`**: the MPU6500 did not answer over SPI. Check the wiring table above and that CS is on GP15.

## License

See [LICENSE](LICENSE).
