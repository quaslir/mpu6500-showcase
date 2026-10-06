add_executable(web_viewer_firmware
    main.cpp
)

target_include_directories(web_viewer_firmware PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(web_viewer_firmware PRIVATE
    mpu6500
    bus_pico
    pico_stdlib
    hardware_spi
)

target_compile_options(web_viewer_firmware PRIVATE -Wall -Wextra)

pico_enable_stdio_usb(web_viewer_firmware 1)
pico_enable_stdio_uart(web_viewer_firmware 0)
pico_add_extra_outputs(web_viewer_firmware)
