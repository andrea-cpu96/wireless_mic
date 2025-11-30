cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(wmic)

target_sources(app PRIVATE 
    src/main.c
    src/i2s_txrx.c
    src/ble_drv.c
)

zephyr_library_include_directories(
  src/
  src/audio
  src/bluetooth
  src/utils
  src/utils/macros
)

add_subdirectory(src/audio)
#add_subdirectory(src/bluetooth)
#add_subdirectory(src/utils)
