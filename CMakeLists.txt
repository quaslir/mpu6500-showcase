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


set(MPU6500_GIT_URL "https://github.com/quaslir/mpu6500-rp2040.git"
    CACHE STRING "Git repository of the MPU6500 driver")
set(MPU6500_GIT_TAG "v1.0.0"
    CACHE STRING "main")
set(MPU6500_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

include(FetchContent)
FetchContent_Declare(
    mpu6500
    GIT_REPOSITORY ${MPU6500_GIT_URL}
    GIT_TAG        ${MPU6500_GIT_TAG}
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(mpu6500)

add_subdirectory(web_viewer_firmware)
