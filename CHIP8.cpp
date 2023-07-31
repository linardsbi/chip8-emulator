#include <SDL.h>
#include <array>
#include <bit>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <format>
#include <fstream>
#include <iostream>
#include <iterator>
#include <ranges>
#include <vector>

#include "CHIP8.hpp"
#include "config.hpp"

namespace Emulator {
bool CHIP8::load_rom(std::string_view filename) {
  std::ifstream istrm(filename.data(), std::ios::binary);
  if (!istrm.is_open()) {
    return false;
  }

  int bytes_read = 0;
  for (; !istrm.eof(); bytes_read += 2) {
    // todo: read whole program at once
    auto *mem_bytes =
        std::next(m_state.memory.begin(), PROGMEM_START + bytes_read);
    istrm.read(std::bit_cast<char *>(mem_bytes), 2);

    if constexpr (DEBUG_EMULATOR) {
      // big endian -> little endian
      const auto bytes =
          static_cast<uint16_t>((mem_bytes[0] << 8) | mem_bytes[1]);
      std::cout << std::format("Read instruction #{}:  0x{:04x}\n",
                               bytes_read / 2, bytes);
    }
  }

  m_program_end_address = static_cast<uint16_t>(PROGMEM_START + bytes_read);

  if constexpr (DEBUG_EMULATOR) {
    std::cout << std::format("Total instructions: {}\n", bytes_read / 2);
  }

  return true;
}

bool CHIP8::single_step() {
  const auto instruction_address = m_state.program_counter;

  if (instruction_address > m_program_end_address) {
    if constexpr (DEBUG_EMULATOR) {
      std::cout << std::format(
          "Tried to access out-of-range instruction at 0x{:x}. "
          "Total instructions: {}\n",
          instruction_address, instruction_count());
    }
    return false;
  }

  const auto instruction = Instruction(m_state.memory, instruction_address);

  if constexpr (DEBUG_EMULATOR) {
    std::cout << std::format("Execute instruction #{} (0x{:04x})\n",
                             (instruction_address - PROGMEM_START) / 2,
                             instruction.value);
  }

  switch (instruction.opcode()) {
  case 0x0:
    if (instruction.Y() == 0x0) {
      m_state.program_counter -= 2;
    } else if (instruction.NN() == 0xE0) {
      m_need_repaint = true;
    } else if (instruction.NN() == 0xEE) {
      m_state.program_counter = m_state.stack.pop();
      // return true;
    } else if (instruction.NNN() == 0x00) {
      return false;
    }
    break;
  case 0x1: {
    m_state.program_counter = instruction.NNN();
    return true;
  }
  case 0x2: {
    m_state.stack.push(m_state.program_counter);
    m_state.program_counter = instruction.NNN();
    return true;
  }
  case 0x3: {
    if (m_state.registers[instruction.X()] == instruction.NN()) {
      m_state.program_counter += 2;
    }

    break;
  }
  case 0x4: {
    if (m_state.registers[instruction.X()] != instruction.NN()) {
      m_state.program_counter += 2;
    }

    break;
  }
  case 0x5: {
    if (m_state.registers[instruction.X()] ==
        m_state.registers[instruction.Y()]) {
      m_state.program_counter += 2;
    }

    break;
  }
  case 0x6: {
    m_state.registers[instruction.X()] = instruction.NN();
    break;
  }
  case 0x7: {
    m_state.registers[instruction.X()] += instruction.NN();
    break;
  }
  case 0x8: {
    switch (instruction.N()) {
    case 0x0:
      m_state.registers[instruction.X()] = m_state.registers[instruction.Y()];
      break;
    case 0x1:
      m_state.registers[instruction.X()] |= m_state.registers[instruction.Y()];
      break;
    case 0x2:
      m_state.registers[instruction.X()] &= m_state.registers[instruction.Y()];
      break;
    case 0x3:
      m_state.registers[instruction.X()] ^= m_state.registers[instruction.Y()];
      break;
    case 0x4: {
      const uint8_t old_x = m_state.registers[instruction.X()];
      m_state.registers[instruction.X()] += m_state.registers[instruction.Y()];

      // did it overflow?
      m_state.registers[0xF] =
          old_x > m_state.registers[instruction.X()] ? 1 : 0;

      break;
    }
    case 0x5: {
      const uint8_t old_x = m_state.registers[instruction.X()];
      m_state.registers[instruction.X()] -= m_state.registers[instruction.Y()];
      // did it overflow?
      m_state.registers[0xF] =
          old_x < m_state.registers[instruction.X()] ? 0 : 1;
      break;
    }

    case 0x6:
      m_state.registers[0xF] = (m_state.registers[instruction.X()] & 1);
      m_state.registers[instruction.X()] >>= 1;
      break;
    case 0x7: {
      const uint8_t y_sub_x = m_state.registers[instruction.Y()] -
                              m_state.registers[instruction.X()];
      m_state.registers[instruction.X()] = y_sub_x;

      // did it overflow?
      m_state.registers[0xF] =
          m_state.registers[instruction.Y()] < y_sub_x ? 0 : 1;
      break;
    }
    case 0xE:
      m_state.registers[0xF] = (m_state.registers[instruction.X()] >> 7);
      m_state.registers[instruction.X()] =
          static_cast<uint8_t>(m_state.registers[instruction.X()] << 1);
      break;
    }
    break;
  }
  case 0x9: {
    if (m_state.registers[instruction.X()] !=
        m_state.registers[instruction.Y()]) {
      m_state.program_counter += 2;
    }

    break;
  }
  case 0xA: {
    m_state.index_register = instruction.NNN();
    break;
  }
  case 0xB: {
    m_state.program_counter = m_state.registers[0] + instruction.NNN();
    return true;
  }
  case 0xC: {
    // fixme:
    const auto fake_random =
        static_cast<uint8_t>(m_state.program_counter ^ 0xFF);
    m_state.registers[instruction.X()] = fake_random & instruction.NN();
    break;
  }

  case 0xD: {
    m_state.registers[0xF] = 0;

    // wrap coordinates outside of screen
    const std::size_t x_coord = m_state.registers[instruction.X()] & 63;
    const std::size_t y_coord = m_state.registers[instruction.Y()] & 31;
    const std::size_t height = instruction.N();

    constexpr auto sprite_width = 8;

    for (std::size_t y = 0; y < height; ++y) {
      if (y + y_coord >= HEIGHT) {
        break;
      }

      // each address contains values for 8 pixels
      const auto value_at_index = m_state.memory[m_state.index_register + y];

      for (std::size_t x = 0; x < sprite_width; ++x) {
        const auto pixel_index = (y_coord + y) * WIDTH + x + x_coord;

        if (x + x_coord >= WIDTH) {
          break;
        }

        // read pixels left to right
        const uint8_t index_pixel_bit_value =
            (value_at_index >> (sprite_width - 1 - x)) & 1;

        // check if a pixel will get unset (both memory and screen pixel have 1)
        m_state.registers[0xF] |= static_cast<uint8_t>(
            (index_pixel_bit_value == 1 && m_pixels[pixel_index] == 1));

        m_pixels[pixel_index] ^= index_pixel_bit_value;
      }
    }

    break;
  }
  case 0xE:
    if (instruction.NN() == 0x9E) {
      if (m_last_key.has_value() &&
          m_last_key.value() == m_state.registers[instruction.X()]) {
        m_last_key.reset();
        m_state.program_counter += 2;
      }
    } else if (instruction.NN() == 0xA1) {
      if (m_last_key.has_value() &&
          m_last_key.value() != m_state.registers[instruction.X()]) {
        m_last_key.reset();
        m_state.program_counter += 2;
      }
    }
    break;
  case 0xF: {
    switch (instruction.NN()) {
    case 0x07:
      m_state.registers[instruction.X()] = m_state.delay_timer;
      break;
    case 0x0A:
      if (!m_last_key.has_value()) {
        m_waiting_for_keypress = true;
        return true; // simulated blocking
      }
      m_state.registers[instruction.X()] = m_last_key.value();
      m_last_key.reset();
      break;
    case 0x15:
      m_state.delay_timer = m_state.registers[instruction.X()];
      break;
    case 0x18:
      m_state.sound_timer = m_state.registers[instruction.X()];
      break;
    case 0x1E:
      m_state.index_register += m_state.registers[instruction.X()];
      break;
    case 0x29:
      m_state.index_register = m_state.registers[instruction.X()] * 5;
      break;
    case 0x33:
      m_state.memory[m_state.index_register + 2] =
          m_state.registers[instruction.X()] % 10;
      m_state.memory[m_state.index_register + 1] =
          (m_state.registers[instruction.X()] / 10) % 10;
      m_state.memory[m_state.index_register] =
          m_state.registers[instruction.X()] / 100;
      break;
    case 0x55:
      for (std::size_t i = m_state.index_register;
           const auto reg :
           m_state.registers | std::ranges::views::take(instruction.X() + 1)) {
        m_state.memory[i++] = reg;
      }
      break;

    case 0x65:
      for (std::size_t i = m_state.index_register;
           auto &reg :
           m_state.registers | std::ranges::views::take(instruction.X() + 1)) {
        reg = m_state.memory[i++];
      }
      break;
    }
    break;
  }
  default:
    std::cout << std::format("Unknown opcode: {:x} (0x{:x})\n",
                             instruction.opcode(), instruction.value);
    return false;
  }
  m_state.program_counter += 2;

  return true;
}
} // namespace Emulator