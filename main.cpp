#include <chrono>
#include <format>
#include <iostream>
#include <memory>
#include <string_view>

#include "CHIP8.hpp"
#include "UI.hpp"
#include "config.hpp"
#include "SDL_defines.hpp"

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

static void render_frame(auto &context, auto &emulator, auto& user_interface) {
  SDL_SetRenderDrawColor(context.renderer, 0, 0, 0, 255);
  SDL_RenderClear(context.renderer);

  int pitch;
  SDL_LockTexture(context.chip8_screen, NULL, (void **)&context.pixels, &pitch);

  const auto color = SDL_MapRGBA(context.pixel_format, 255, 255, 255, 255);
  
  if (!emulator.need_clear_screen()) {
    SDL_UnlockTexture(context.chip8_screen);
    for (int y = 0; y < Emulator::HEIGHT; ++y) {
      for (int x = 0; x < Emulator::WIDTH; ++x) {
        const auto coordinate = y * Emulator::WIDTH + x;
        context.pixels[coordinate] = emulator.get_pixel(coordinate) > 0 ? color : 0;
      }
    }
    SDL_UnlockTexture(context.chip8_screen);
  }

  emulator.set_need_clear_screen(false);

  // maintain chip8 aspect ratio
  const auto target_height = WINDOW_WIDTH / Emulator::ASPECT_RATIO;
  SDL_Rect window_rect = {.x = 0,
                          .y = (WINDOW_HEIGHT - target_height) / 2,
                          .w = WINDOW_WIDTH,
                          .h = target_height};

  if (SDL_RenderCopy(context.renderer, context.chip8_screen, nullptr,
                     &window_rect) < 0) {
    SDL_Log("Couldn't load %s\n", SDL_GetError());
    exit(1);
  }

  user_interface.render(context);

  SDL_RenderPresent(context.renderer);
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

  Context context(game_name, WINDOW_WIDTH, WINDOW_HEIGHT);
  Emulator::CHIP8 emulator;
  UI user_interface;

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
      if (user_interface.container_start()) {
        // user_interface.textbox("Tab 1");
        // user_interface.textbox("Tab 2");
        // user_interface.textbox("Tab 3");
        // user_interface.button("Click me");

        user_interface.container_end();
      }
      
      render_frame(context, emulator, user_interface);
    }

    if (!paused) {
      if (timer_timer.exec(now)) {
        if (emulator.sound_playing()) {
          Mix_PlayChannel(0, context.beep, 0);
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

  Mix_CloseAudio();
  SDL_Quit();

  return 0;
}
