cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(wmic)

target_sources(app PRIVATE 
    src/main.c
    src/audio/audio.c
    src/filters/low_pass_filter.c
    src/filters/signals.c
    src/bt1036c/bt1036c_drv.c
)

zephyr_library_include_directories(
  src/
  src/bt1036c/
  src/filters/
  src/audio/
)

# Include the local CMSIS-DSP headers and compile only needed sources
zephyr_library_include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/../modules/CMSIS-DSP/Include
  ${CMAKE_CURRENT_SOURCE_DIR}/../modules/CMSIS-DSP/PrivateInclude
)

# Compile only CMSIS-DSP Filtering Functions (FIR) and Support Functions needed
file(GLOB CMSIS_DSP_FILTERING_SOURCES 
  ${CMAKE_CURRENT_SOURCE_DIR}/../modules/CMSIS-DSP/Source/FilteringFunctions/arm_fir*.c
)

file(GLOB CMSIS_DSP_SUPPORT_SOURCES 
  ${CMAKE_CURRENT_SOURCE_DIR}/../modules/CMSIS-DSP/Source/SupportFunctions/arm_*_f32.c
  ${CMAKE_CURRENT_SOURCE_DIR}/../modules/CMSIS-DSP/Source/SupportFunctions/arm_*_q15.c
)

target_sources(app PRIVATE ${CMSIS_DSP_FILTERING_SOURCES} ${CMSIS_DSP_SUPPORT_SOURCES})
