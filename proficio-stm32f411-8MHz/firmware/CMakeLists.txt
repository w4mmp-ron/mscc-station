# Scaffold CMake — incomplete until Cube/CMSIS/startup are added (Phase 0).
cmake_minimum_required(VERSION 3.16)
project(proficio-stm32f411-8MHz C ASM)

set(CMAKE_C_STANDARD 11)

# Placeholder: add toolchain file, MCU flags, linker script, HAL sources.
message(STATUS "proficio-stm32f411-8MHz: scaffold only — add STM32 toolchain to build")

add_executable(proficio_fw
    src/main.c
    src/board/board.c
    src/app/app.c
)

target_include_directories(proficio_fw PRIVATE
    include
)

# target_compile_options / link options TBD
