// Shows the board orientation as a rotating 3D box on an SSD1306 OLED.
// Roll and pitch come from the complementary filter, yaw is the integrated gyro Z (drifts).
#include "bus_pico/spi_bus.hpp"
#include "i2c_config.hpp"
#include "mpu6500/calibration.hpp"
#include "mpu6500/config.hpp"
#include "mpu6500/mpu6500.hpp"
#include "mpu6500/sample.hpp"
#include "orientation/orientation.hpp"
#include "spi_config.hpp"
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <numbers>
#include <pico/stdlib.h>

extern "C" {
#include "ssd1306.h"
}

namespace {

namespace cfg = mpu6500::config;
namespace cal = mpu6500::calibration;

constexpr uint8_t SAMPLE_DIVIDER = 4;    // 1000 / (1 + 4) = 200 Hz
constexpr uint32_t PERIOD_US = 5000;     // matches 200 Hz
constexpr int SAMPLES_PER_FRAME = 7;     // 200 Hz / 7 ~ 28 FPS
constexpr int CALIBRATION_ATTEMPTS = 3;
constexpr uint32_t CALIBRATION_RETRY_MS = 1000;

constexpr uint16_t SCREEN_W = 128;
constexpr uint16_t SCREEN_H = 64;
constexpr int32_t CENTER_X = SCREEN_W / 2;
constexpr int32_t CENTER_Y = SCREEN_H / 2 + 4; // leave room for the text line
constexpr float CAMERA_DISTANCE = 4.0f;
constexpr float FOCAL_PX = 80.0f;
constexpr float CAMERA_TILT_DEG = 25.0f;
constexpr float DEG_TO_RAD = std::numbers::pi_v<float> / 180.0f;

struct Point3 {
    float x, y, z;
};

struct Point2 {
    int32_t x, y;
};

// Flat box shaped like a PCB, plus an arrow (8 -> 9) marking the +X axis of the sensor.
constexpr std::array<Point3, 10> VERTICES{{
    {-1.0f, -0.6f, -0.1f},
    {1.0f, -0.6f, -0.1f},
    {1.0f, 0.6f, -0.1f},
    {-1.0f, 0.6f, -0.1f},
    {-1.0f, -0.6f, 0.1f},
    {1.0f, -0.6f, 0.1f},
    {1.0f, 0.6f, 0.1f},
    {-1.0f, 0.6f, 0.1f},
    {0.0f, 0.0f, 0.1f},
    {1.4f, 0.0f, 0.1f},
}};

constexpr std::array<std::array<uint8_t, 2>, 13> EDGES{{
    {0, 1},
    {1, 2},
    {2, 3},
    {3, 0},
    {4, 5},
    {5, 6},
    {6, 7},
    {7, 4},
    {0, 4},
    {1, 5},
    {2, 6},
    {3, 7},
    {8, 9},
}};

struct Rotation {
    float cos_roll, sin_roll;
    float cos_pitch, sin_pitch;
    float cos_yaw, sin_yaw;
    float cos_tilt, sin_tilt;
};

Rotation make_rotation(const orientation::Angles& angles, float yaw_deg) {
    const float roll = angles.roll_deg * DEG_TO_RAD;
    const float pitch = angles.pitch_deg * DEG_TO_RAD;
    const float yaw = yaw_deg * DEG_TO_RAD;
    const float tilt = CAMERA_TILT_DEG * DEG_TO_RAD;
    return {std::cos(roll),
            std::sin(roll),
            std::cos(pitch),
            std::sin(pitch),
            std::cos(yaw),
            std::sin(yaw),
            std::cos(tilt),
            std::sin(tilt)};
}

// Sensor rotation (X roll, then Y pitch, then Z yaw), then a fixed camera tilt around X
// so a board lying flat is seen from above instead of edge-on.
Point3 rotate(const Rotation& r, Point3 p) {
    p = Point3{p.x, p.y * r.cos_roll - p.z * r.sin_roll, p.y * r.sin_roll + p.z * r.cos_roll};
    p = Point3{p.x * r.cos_pitch + p.z * r.sin_pitch, p.y, -p.x * r.sin_pitch + p.z * r.cos_pitch};
    p = Point3{p.x * r.cos_yaw - p.y * r.sin_yaw, p.x * r.sin_yaw + p.y * r.cos_yaw, p.z};
    p = Point3{p.x, p.y * r.cos_tilt - p.z * r.sin_tilt, p.y * r.sin_tilt + p.z * r.cos_tilt};
    return p;
}

// Camera looks along +Y: screen X = world X, screen Y = world Z (flipped, screen Y grows down).
Point2 project(const Point3& p) {
    const float depth = p.y + CAMERA_DISTANCE;
    return {CENTER_X + static_cast<int32_t>(std::lround(FOCAL_PX * p.x / depth)),
            CENTER_Y - static_cast<int32_t>(std::lround(FOCAL_PX * p.z / depth))};
}

void draw_frame(ssd1306_t& display, const orientation::Angles& angles, float yaw_deg) {
    const Rotation rotation = make_rotation(angles, yaw_deg);

    std::array<Point2, VERTICES.size()> screen{};
    for (std::size_t i = 0; i < VERTICES.size(); ++i)
        screen[i] = project(rotate(rotation, VERTICES[i]));

    ssd1306_clear(&display);
    for (const auto& edge : EDGES) {
        const Point2& a = screen[edge[0]];
        const Point2& b = screen[edge[1]];
        ssd1306_draw_line(&display, a.x, a.y, b.x, b.y);
    }

    char text[32];
    std::snprintf(text,
                  sizeof(text),
                  "R%4ld P%4ld Y%4ld",
                  std::lround(angles.roll_deg),
                  std::lround(angles.pitch_deg),
                  std::lround(yaw_deg));
    ssd1306_draw_string(&display, 0, 0, 1, text);

    ssd1306_show(&display);
}

[[noreturn]] void fail_forever(const char* message) {
    for (;;) {
        std::printf("# %s\n", message);
        sleep_ms(1000);
    }
}

} // namespace

int main() {
    stdio_init_all();

    spi_config::init_spi();
    i2c_config::init_test_i2c();

    ssd1306_t display{};
    display.external_vcc = false;
    if (!ssd1306_init(&display, SCREEN_W, SCREEN_H, 0x3C, i2c0))
        fail_forever("display init failed (out of memory)");

    ssd1306_clear(&display);
    ssd1306_draw_string(&display, 0, 0, 1, "Calibrating...");
    ssd1306_draw_string(&display, 0, 16, 1, "Keep still");
    ssd1306_show(&display);

    bus::pico::SPIBus spi_bus{spi0, 15};

    cfg::Config config{};
    config.use_i2c = false; // SPI: turn the chip's I2C interface off
    config.measurement.gyro.filter = cfg::GyroFilter::Hz92;
    config.measurement.accel.filter = cfg::AccelFilter::Hz92;
    config.measurement.sample_divider = SAMPLE_DIVIDER;

    mpu6500::Mpu6500 imu{spi_bus, sleep_ms, config};
    if (imu.init() != bus::Status::OK) {
        ssd1306_clear(&display);
        ssd1306_draw_string(&display, 0, 0, 1, "IMU init failed");
        ssd1306_show(&display);
        fail_forever("init failed, check wiring");
    }

    bus::Status cal_status = bus::Status::ERROR;
    for (int i = 0; i < CALIBRATION_ATTEMPTS; ++i) {
        cal_status = cal::calibrate_gyro(imu);
        if (cal_status == bus::Status::OK)
            break;
        std::printf("# calibration attempt %d failed, retrying\n", i + 1);
        sleep_ms(CALIBRATION_RETRY_MS);
    }
    std::printf("# gyro calibration: %s\n", cal_status == bus::Status::OK ? "ok" : "failed");

    orientation::ComplementaryFilter filter{};
    float yaw_deg = 0.0f;
    int samples_since_frame = 0;
    uint64_t last_us = time_us_64();
    absolute_time_t next = get_absolute_time();

    for (;;) {
        next = delayed_by_us(next, PERIOD_US);
        sleep_until(next);

        mpu6500::Sample s{};
        if (imu.read_all(s) != bus::Status::OK)
            continue;

        const uint64_t now_us = time_us_64();
        const float dt_s = static_cast<float>(now_us - last_us) * 1e-6f;
        last_us = now_us;

        const orientation::Angles angles = filter.update(s.accel_g, s.gyro_dps, dt_s);
        yaw_deg = std::remainder(yaw_deg + s.gyro_dps.z * dt_s, 360.0f);

        if (++samples_since_frame < SAMPLES_PER_FRAME)
            continue;
        samples_since_frame = 0;

        draw_frame(display, angles, yaw_deg);
        next = get_absolute_time(); // ssd1306_show blocks ~25 ms, do not try to catch up
    }
}
