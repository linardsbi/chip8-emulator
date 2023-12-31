#pragma once
#include <array>
#include <cstdint>

constexpr auto WINDOW_WIDTH = 800;
constexpr auto WINDOW_HEIGHT = 600;

namespace Emulator {
constexpr auto WIDTH = 64;
constexpr auto HEIGHT = 32;
constexpr auto ASPECT_RATIO = WIDTH / HEIGHT;
constexpr auto MEMORY_SIZE = 4096;
constexpr auto STACK_SIZE = 256;
constexpr auto REG_COUNT = 16;
constexpr auto KEYBOARD_SIZE = 4 * 4;
constexpr auto PROGMEM_START = 0x200;
constexpr auto PROCESSOR_SPEED = 400;

enum class Keymap: uint8_t {
  one = 0x1, two = 0x2, three = 0x3, four = 0xc,
  q = 0x4, w = 0x5, e = 0x6, r = 0xd,
  a = 0x7, s = 0x8, d = 0x9, f = 0xe,
  z = 0xa, x = 0x0, c = 0xb, v = 0xf,
};

constexpr std::array<uint8_t, 5 * 16> font = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};
} // namespace Emulator