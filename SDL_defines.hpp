#pragma once

#include <SDL.h>
#include <SDL_blendmode.h>
#include <SDL_keycode.h>
#include <SDL_mixer.h>
#include <SDL_pixels.h>
#include <SDL_render.h>
#include <SDL_timer.h>
#include <SDL_ttf.h>
#include <SDL_video.h>

#include <string_view>

#include "config.hpp"

struct Context {
  SDL_Window *window{};
  SDL_Renderer *renderer{};
  SDL_Texture *chip8_screen{};
  uint32_t *pixels{};
  SDL_PixelFormat *pixel_format{};
  Mix_Chunk *beep{};
  TTF_Font *font;
  unsigned width;
  unsigned height;

  Context(std::string_view window_name, auto window_width, auto window_height) {
    width = window_width;
    height = window_height;

    auto audio_rate = 44100;
    auto audio_format = static_cast<uint16_t>(AUDIO_S32);
    auto audio_channels = 1;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO) < 0) {
      SDL_Log("Couldn't initialize SDL: %s\n", SDL_GetError());
      exit(1);
    }

    if (TTF_Init() < 0) {
      SDL_Log("Couldn't initialize SDL_ttf: %s\n", SDL_GetError());
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

    window = SDL_CreateWindow(window_name.data(), SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED, window_width,
                              window_height, SDL_WINDOW_SHOWN);

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "best");
    SDL_RenderSetLogicalSize(renderer, window_width, window_height);

    // full rgb because why not
    chip8_screen = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                                     SDL_TEXTUREACCESS_STREAMING,
                                     Emulator::WIDTH, Emulator::HEIGHT);
    pixel_format = SDL_AllocFormat(SDL_PIXELFORMAT_RGBA8888);

    SDL_SetTextureScaleMode(chip8_screen, SDL_ScaleModeNearest);
    SDL_SetTextureBlendMode(chip8_screen, SDL_BLENDMODE_BLEND);

    std::string_view filename = "beep.wav";
    beep = Mix_LoadWAV(filename.data());

    if (beep == nullptr) {
      SDL_Log("Couldn't load %s: %s\n", filename.data(), SDL_GetError());
      exit(1);
    }

    const char *font_name = "/usr/share/fonts/TTF/DejaVuSans.ttf";
    font = TTF_OpenFont(font_name, 24);

    if (!font) {
      SDL_Log("error: font '%s' not found\n", font_name);
      exit(EXIT_FAILURE);
    }
  }

  Context(Context &) = delete;
  Context(Context &&) = delete;

  ~Context() {
    SDL_DestroyRenderer(renderer);
    SDL_DestroyTexture(chip8_screen);
    SDL_DestroyWindow(window);
    Mix_FreeChunk(beep);
    TTF_CloseFont(font);
  }
};

enum class Key { None, Exit, Pause };