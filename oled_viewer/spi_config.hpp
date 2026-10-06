#pragma once
#include <cstdint>
#include <hardware/gpio.h>
#include <hardware/spi.h>

namespace spi_config {

inline constexpr uint32_t FREQ_HZ{1'000'000};
inline constexpr uint8_t SCK{2};
inline constexpr uint8_t MOSI{3};
inline constexpr uint8_t MISO{4};
inline constexpr uint8_t CS{15};

inline void init_spi() {
    spi_init(spi0, FREQ_HZ);
    gpio_set_function(SCK, GPIO_FUNC_SPI);
    gpio_set_function(MOSI, GPIO_FUNC_SPI);
    gpio_set_function(MISO, GPIO_FUNC_SPI);

    gpio_init(CS);
    gpio_set_dir(CS, GPIO_OUT);
    gpio_put(CS, 1);
}

} // namespace spi_config
