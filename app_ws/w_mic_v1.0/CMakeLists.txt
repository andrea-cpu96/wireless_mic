#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(app)

target_sources(app PRIVATE
  src/main.c
  src/pwm_drv/pwm_drv.c
  src/adc_drv/adc_drv.c
  src/i2s_drv/i2s_drv.c
  src/audio/audio.c
  src/adc_drv/vendor/nordic/nrf_adc_drv.c
)

include_directories(${CMAKE_SOURCE_DIR}/src)

zephyr_library_include_directories(
  ./
  src/audio/
  src/pwm_drv/
  src/adc_drv/
  src/i2s_drv/
  src/adc_drv/vendor/nordic/
)
