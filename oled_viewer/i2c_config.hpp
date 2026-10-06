#pragma once

#include <cstdint>
#include <hardware/gpio.h>
#include <hardware/i2c.h>
namespace i2c_config {
inline constexpr uint8_t SDA{8};
inline constexpr uint8_t SCL{9};
inline constexpr uint32_t FREQ{4 * 100 * 1000};
inline void init_test_i2c() {
    sleep_ms(3000);
    i2c_init(i2c0, FREQ);
    gpio_set_function(SDA, GPIO_FUNC_I2C);
    gpio_set_function(SCL, GPIO_FUNC_I2C);

    gpio_pull_up(SDA);
    gpio_pull_up(SCL);
}
} // namespace i2c_config
