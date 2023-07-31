cmake_minimum_required(VERSION 3.1...3.27.1)

project(
  CHIP8
  VERSION 1.0
  LANGUAGES CXX)

find_package(SDL2 REQUIRED)
find_package(SDL2_mixer REQUIRED)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

add_compile_options("-O0" "-g" "-Wall" "-Wextra" "-Wpedantic" "-Wconversion" "-fsanitize=address,leak,undefined" "-DDEBUG_EMULATOR=1")
set(CMAKE_EXE_LINKER_FLAGS "-fsanitize=address,leak,undefined -lSDL2_mixer")

include_directories(. ${SDL2_INCLUDE_DIR} ${SDL2_MIXER_INCLUDE_DIRS})

add_executable(CHIP8 main.cpp CHIP8.cpp)

target_link_libraries(CHIP8 SDL2::SDL2)
