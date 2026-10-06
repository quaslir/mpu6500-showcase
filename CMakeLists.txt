cmake_minimum_required(VERSION 3.20)

set(PICO_BOARD pico CACHE STRING "Board type")
include(pico_sdk_import.cmake)

project(mpu6500_demos C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

pico_sdk_init()

if (POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

set(MPU6500_GIT_URL "https://github.com/quaslir/mpu6500-rp2040.git"
    CACHE STRING "Git repository of the MPU6500 driver")
set(MPU6500_GIT_TAG "v1.0.0"
    CACHE STRING "Git tag or branch of the MPU6500 driver")
set(SSD1306_URL "https://github.com/daschr/pico-ssd1306/archive/8467f5b06ebd7bfa7b53e4ebc83dfdc6f396e4eb.tar.gz"
    CACHE STRING "Archive of the pico-ssd1306 library")

include(FetchContent)
FetchContent_Declare(
    mpu6500
    GIT_REPOSITORY ${MPU6500_GIT_URL}
    GIT_TAG        ${MPU6500_GIT_TAG}
    GIT_SHALLOW    TRUE
)
FetchContent_Declare(
    pico_ssd1306
    URL ${SSD1306_URL}
)
FetchContent_MakeAvailable(mpu6500 pico_ssd1306)

add_library(pico_ssd1306 STATIC
    ${pico_ssd1306_SOURCE_DIR}/ssd1306.c
)

target_include_directories(pico_ssd1306 SYSTEM PUBLIC
    ${pico_ssd1306_SOURCE_DIR}
)

target_link_libraries(pico_ssd1306 PUBLIC
    pico_stdlib
    hardware_i2c
)

add_library(demo_common INTERFACE)

target_include_directories(demo_common INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/common
)

add_subdirectory(web_viewer_firmware)
add_subdirectory(oled_viewer)
