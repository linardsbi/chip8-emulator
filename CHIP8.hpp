#include "config.hpp"
#include <SDL.h>
#include <array>
#include <bit>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <format>
#include <fstream>
#include <iostream>
#include <vector>
#include <optional>

#ifndef DEBUG_EMULATOR
#define DEBUG_EMULATOR 0
#endif

namespace Emulator {
struct Instruction {
  constexpr Instruction(const auto &memory, const auto address) {
    const auto * mem_bytes = std::next(memory.begin(), address);
    value = static_cast<uint16_t>((mem_bytes[0] << 8) | mem_bytes[1]);
  }
  auto opcode() const { return static_cast<uint8_t>(value >> 12); }
  auto X() const { return static_cast<uint8_t>((value >> 8) & 0x0F); }
  auto Y() const { return static_cast<uint8_t>((value >> 4) & 0x0F); }
  auto N() const { return static_cast<uint8_t>(value & 0x0F); }
  auto NN() const { return static_cast<uint8_t>(value & 0xFF); }
  auto NNN() const { return static_cast<uint16_t>(value & 0xFFF); }

  uint16_t value;
};

template<typename VALUE_T>
class Stack {
public:
  void push(VALUE_T value) {
    m_stack.at(m_pointer) = value;
    ++m_pointer;
  }

  auto pop() {
    const auto value = m_stack.at(m_pointer - 1);
    --m_pointer;
    return value;
  }

private:
  std::array<VALUE_T, STACK_SIZE> m_stack{};
  std::size_t m_pointer{};
};

struct State {
  std::array<uint8_t, MEMORY_SIZE> memory;
  Stack<uint32_t> stack;
  std::array<uint8_t, REG_COUNT> registers;
  std::size_t index_register;
  uint32_t program_counter;
  uint16_t stack_counter;
  uint8_t sound_timer;
  uint8_t delay_timer;
};

class CHIP8 {
public:
  constexpr CHIP8() {
    m_state.program_counter = PROGMEM_START;
    for (std::size_t i = 0; i < font.size(); ++i) {
      m_state.memory[i] = font[i];
    }
  }
  bool load_rom(std::string_view filename);

  bool single_step();

  void timer_tick() {
    if (m_state.delay_timer > 0) {
      --m_state.delay_timer;
    }
    if (m_state.sound_timer > 0) {
      --m_state.sound_timer;
    }
  }

  bool sound_playing() const {
    return m_state.sound_timer > 0;
  }

  uint8_t get_pixel(const auto coordinate) const {
    return m_pixels.at(coordinate);
  }

  bool need_clear_screen() const {
    return m_need_repaint;
  }

  void set_need_clear_screen(const bool value) {
    m_need_repaint = value;
  }

  void set_last_key(const uint8_t key) {
    m_last_key = key;
  }

private:
  uint32_t instruction_count() const {
    return (m_program_end_address - PROGMEM_START) / 2;
  }

  State m_state{};
  std::array<uint8_t, WIDTH * HEIGHT> m_pixels;
  std::vector<Instruction> m_instructions;
  std::optional<uint8_t> m_last_key;
  uint16_t m_program_end_address;
  bool m_need_repaint{false};
  bool m_waiting_for_keypress{false};
};
} // namespace Emulator