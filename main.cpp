#include <SDL.h>
#include <SDL_keycode.h>
#include <SDL_mixer.h>
#include <SDL_render.h>
#include <SDL_timer.h>
#include <SDL_video.h>
#include <chrono>
#include <format>
#include <iostream>
#include <string_view>

#include "CHIP8.hpp"
#include "config.hpp"

auto init_sdl(std::string_view window_name, auto width, auto height) {
  auto audio_rate = 44100;
  auto audio_format = static_cast<uint16_t>(AUDIO_S32);
  auto audio_channels = 1;

  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO) < 0) {
    SDL_Log("Couldn't initialize SDL: %s\n", SDL_GetError());
    exit(1);
  }

  if (Mix_OpenAudio(audio_rate, audio_format, audio_channels, 4096) < 0) {
    SDL_Log("Couldn't open audio: %s\n", SDL_GetError());
    exit(1);
  } else {
    Mix_QuerySpec(&audio_rate, &audio_format, &audio_channels);
    SDL_Log("Opened audio at %d Hz %d bit%s %s", audio_rate,
            (audio_format & 0xFF),
            (SDL_AUDIO_ISFLOAT(audio_format) ? " (float)" : ""),
            (audio_channels > 2)   ? "surround"
            : (audio_channels > 1) ? "stereo"
                                   : "mono");
  }

  auto *window = SDL_CreateWindow(window_name.data(), SDL_WINDOWPOS_UNDEFINED,
                                  SDL_WINDOWPOS_UNDEFINED, width, height,
                                  SDL_WINDOW_SHOWN);

  // full rgb because why not
  constexpr auto bpp = 32;
  auto *chip8_screen = SDL_CreateRGBSurface(0, Emulator::WIDTH,
                                            Emulator::HEIGHT, bpp, 0, 0, 0, 0);

  auto *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "best");
  SDL_RenderSetLogicalSize(renderer, width, height);

  std::string_view filename = "beep.wav";
  auto *beep = Mix_LoadWAV(filename.data());

  if (beep == nullptr) {
    SDL_Log("Couldn't load %s: %s\n", filename.data(), SDL_GetError());
    exit(1);
  }

  return std::tuple(window, renderer, chip8_screen, beep);
}

enum class Key { None, Exit, Pause };

// handles input for both the emulator and window events
static Key handle_input(auto &emulator, auto &instruction_timer) {
  using Emulator::Keymap;
  using namespace std::chrono_literals;

  SDL_Event event;
  auto keydown = [&emulator](Keymap key) {
    emulator.set_last_key(static_cast<uint8_t>(key));
  };

  // todo: find a smarter way to handle mapping key events
  while (SDL_PollEvent(&event) != 0) {
    switch (event.type) {
    case SDL_QUIT:
      return Key::Exit;
    case SDL_KEYDOWN:
      switch (event.key.keysym.sym) {
      case SDLK_ESCAPE:
        return Key::Exit;
      case SDLK_EQUALS:
        instruction_timer.interval -= 0.1ms;
        break;
      case SDLK_MINUS:
        instruction_timer.interval += 0.1ms;
        break;
      case SDLK_u:
        instruction_timer.interval = 1000ms / Emulator::PROCESSOR_SPEED;
        break;
      case SDLK_1:
        keydown(Keymap::one);
        break;
      case SDLK_2:
        keydown(Keymap::two);
        break;
      case SDLK_3:
        keydown(Keymap::three);
        break;
      case SDLK_4:
        keydown(Keymap::four);
        break;
      case SDLK_q:
        keydown(Keymap::q);
        break;
      case SDLK_w:
        keydown(Keymap::w);
        break;
      case SDLK_e:
        keydown(Keymap::e);
        break;
      case SDLK_r:
        keydown(Keymap::r);
        break;
      case SDLK_a:
        keydown(Keymap::a);
        break;
      case SDLK_s:
        keydown(Keymap::s);
        break;
      case SDLK_d:
        keydown(Keymap::d);
        break;
      case SDLK_f:
        keydown(Keymap::f);
        break;
      case SDLK_z:
        keydown(Keymap::z);
        break;
      case SDLK_x:
        keydown(Keymap::x);
        break;
      case SDLK_c:
        keydown(Keymap::c);
        break;
      case SDLK_v:
        keydown(Keymap::v);
        break;
      case SDLK_p:
        return Key::Pause;
      default:
        break;
      }
      break;
    default:
      break;
    }
  }

  return Key::None;
}

static void render_frame(auto *window, auto *surface, auto *renderer,
                         auto &emulator) {
  SDL_RenderClear(renderer);

  auto *window_surface = SDL_GetWindowSurface(window);

  SDL_FillRect(surface, nullptr, SDL_MapRGBA(surface->format, 0, 0, 0, 255));

  auto *pixels = static_cast<uint32_t *>(surface->pixels);
  const auto color = SDL_MapRGBA(surface->format, 255, 255, 255, 255);

  if (!emulator.need_clear_screen()) {
    SDL_LockSurface(surface);
    for (int y = 0; y < Emulator::HEIGHT; ++y) {
      for (int x = 0; x < Emulator::WIDTH; ++x) {
        const auto coordinate = y * surface->w + x;
        pixels[coordinate] = emulator.get_pixel(coordinate) > 0 ? color : 0;
      }
    }
    SDL_UnlockSurface(surface);
  }

  emulator.set_need_clear_screen(false);

  // maintain chip8 aspect ratio
  const auto target_height = WINDOW_WIDTH / Emulator::ASPECT_RATIO;
  SDL_Rect window_rect = {.x = 0,
                          .y = (WINDOW_HEIGHT - target_height) / 2,
                          .w = WINDOW_WIDTH,
                          .h = target_height};

  SDL_BlitScaled(surface, nullptr, window_surface, &window_rect);

  SDL_UpdateWindowSurface(window);
}

struct Timer {
  Timer(const auto interval) : interval(interval) {}

  bool exec(const auto at) {
    if (at - last_exec_time >= interval) {
      last_exec_time = at;
      return true;
    }
    return false;
  }

  std::chrono::duration<double, std::milli> interval;
  std::chrono::time_point<std::chrono::system_clock> last_exec_time{};
};

auto init_timers() {
  using namespace std::chrono_literals;

  constexpr auto FPS = 60;
  constexpr auto TIMER_TICKRATE = 60;

  const auto frame_timer = Timer(1000.0ms / FPS);
  const auto timer_timer = Timer(1000.0ms / TIMER_TICKRATE);
  const auto instruction_timer = Timer(1000.0ms / Emulator::PROCESSOR_SPEED);

  return std::array{frame_timer, timer_timer, instruction_timer};
}

int main() {
  std::string_view game_name = "particles.ch8";

  auto [window, renderer, chip8_screen, beep] =
      init_sdl(game_name, WINDOW_WIDTH, WINDOW_HEIGHT);

  Emulator::CHIP8 emulator;

  if (!emulator.load_rom(game_name)) {
    std::cout << std::format("Could not open ROM: {}\n", game_name);
    return 1;
  }

  std::cout << std::format("ROM loaded: {}\n", game_name);

  auto [frame_timer, timer_timer, instruction_timer] = init_timers();

  bool paused = false;
  for (Key key{}; key != Key::Exit;) {

    key = handle_input(emulator, instruction_timer);

    if (key == Key::Pause) {
      paused = !paused;
    }

    const auto now = std::chrono::system_clock::now();

    if (frame_timer.exec(now)) {
      render_frame(window, chip8_screen, renderer, emulator);
    }

    if (!paused) {
      if (timer_timer.exec(now)) {
        if (emulator.sound_playing()) {
          Mix_PlayChannel(0, beep, 0);
        } else {
          Mix_HaltChannel(0);
        }

        emulator.timer_tick();
      }

      if (instruction_timer.exec(now)) {
        if (!emulator.single_step()) {
          std::cout << "Emulator terminated execution\n";
          break;
        }
      }
    }
  }

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindowSurface(window);
  SDL_DestroyWindow(window);
  Mix_FreeChunk(beep);
  Mix_CloseAudio();
  SDL_Quit();

  return 0;
}
