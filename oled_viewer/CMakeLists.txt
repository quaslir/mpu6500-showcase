add_executable(oled_orientation
    main.cpp
)

target_link_libraries(oled_orientation PRIVATE
    demo_common
    mpu6500
    bus_pico
    orientation
    pico_ssd1306
    pico_stdlib
    pico_multicore
    hardware_spi
    hardware_i2c
)

target_compile_options(oled_orientation PRIVATE -Wall -Wextra)

pico_enable_stdio_usb(oled_orientation 1)
pico_enable_stdio_uart(oled_orientation 0)
pico_add_extra_outputs(oled_orientation)
