// Streams timestamped accel + gyro samples over USB serial for imu_viewer.html.
// Line format: t_us,ax,ay,az,gx,gy,gz   (µs, g, °/s)
// Lines starting with '#' are comments; the viewer ignores them.
// Orientation is computed in the browser (Madgwick), the Pico only sends raw data.
#include "bus_pico/spi_bus.hpp"
#include "mpu6500/calibration.hpp"
#include "mpu6500/config.hpp"
#include "mpu6500/mpu6500.hpp"
#include "mpu6500/sample.hpp"
#include "spi_config.hpp"
#include <cstdint>
#include <cstdio>
#include <pico/stdlib.h>

namespace {

namespace cfg = mpu6500::config;
namespace cal = mpu6500::calibration;

constexpr uint32_t STARTUP_DELAY_MS = 3000; // time to open the viewer or a terminal
constexpr uint8_t SAMPLE_DIVIDER = 4;       // 1000 / (1 + 4) = 200 Hz
constexpr uint32_t PERIOD_US = 5000;        // matches 200 Hz
constexpr int CALIBRATION_ATTEMPTS = 3;
constexpr uint32_t CALIBRATION_RETRY_MS = 1000;

} // namespace

int main() {
    stdio_init_all();
    std::printf("# MPU6500 stream: t_us,ax,ay,az,gx,gy,gz\n");

    spi_config::init_spi();
    bus::pico::SPIBus spi_bus{spi0, spi_config::CS};

    cfg::Config config{};
    config.use_i2c = false; // SPI: turn the chip's I2C interface off
    config.measurement.gyro.filter = cfg::GyroFilter::Hz92;
    config.measurement.accel.filter = cfg::AccelFilter::Hz92;
    config.measurement.sample_divider = SAMPLE_DIVIDER;

    mpu6500::Mpu6500 imu{spi_bus, sleep_ms, config};
    if (imu.init() != bus::Status::OK) {
        for (;;) {
            std::printf("# init failed, check wiring\n");
            sleep_ms(1000);
        }
    }

    std::printf("# keep the board still: calibrating gyro\n");
    bus::Status cal_status = bus::Status::ERROR;
    for (int i = 0; i < CALIBRATION_ATTEMPTS; ++i) {
        cal_status = cal::calibrate_gyro(imu);
        if (cal_status == bus::Status::OK)
            break;
        std::printf("# calibration attempt %d failed, retrying\n", i + 1);
        sleep_ms(CALIBRATION_RETRY_MS);
    }
    std::printf("# gyro calibration: %s\n", cal_status == bus::Status::OK ? "ok" : "failed");

    absolute_time_t next = get_absolute_time();
    for (;;) {
        next = delayed_by_us(next, PERIOD_US);
        sleep_until(next);

        mpu6500::Sample s{};
        if (imu.read_all(s) != bus::Status::OK)
            continue;

        std::printf("%llu,%.4f,%.4f,%.4f,%.3f,%.3f,%.3f\n",
                    static_cast<unsigned long long>(time_us_64()),
                    s.accel_g.x,
                    s.accel_g.y,
                    s.accel_g.z,
                    s.gyro_dps.x,
                    s.gyro_dps.y,
                    s.gyro_dps.z);
    }
}
