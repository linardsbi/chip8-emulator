#include <SDL.h>
#include <SDL_rect.h>
#include <cassert>
#include <format>
#include <iostream>
#include <memory>

#include "SDL_defines.hpp"
#include "UI.hpp"

auto draw_textbox(const auto &element, const SDL_Rect &prev_bb,
                  const auto draw_direction, const Context &context) {

  auto *text = TTF_RenderText_Solid(context.font, element.content.c_str(),
                                    SDL_Color{255, 255, 255, 255});
  auto *texture = SDL_CreateTextureFromSurface(context.renderer, text);

  auto bb_x = prev_bb.x + prev_bb.w;
  auto bb_y = prev_bb.y;

  if (draw_direction == DrawDirection::Vertical) {
    bb_x = prev_bb.x;
    bb_y = prev_bb.y + prev_bb.h;
  }

  SDL_Rect text_bounding_box = {.x = static_cast<int>(bb_x),
                                .y = static_cast<int>(bb_y),
                                .w = text->w,
                                .h = text->h};

  SDL_FreeSurface(text);
  SDL_RenderCopy(context.renderer, texture, nullptr, &text_bounding_box);

  return text_bounding_box;
}

void UI::render(Context &context) {
  SDL_Rect last_element_bb{};
  DrawDirection prev_draw_direction{};
  DrawDirection draw_direction{};

  for (const auto &element : m_elements) {
    switch (element.type) {
    case ElementType::ContainerStart: {
      const auto attributes =
          std::dynamic_pointer_cast<ContainerAttributes>(element.attributes)
              .get();
      prev_draw_direction = draw_direction;
      draw_direction = attributes->direction;
      break;
    }
    case ElementType::ContainerEnd:
      draw_direction = prev_draw_direction;
      break;
    case ElementType::Textbox:
      last_element_bb =
          draw_textbox(element, last_element_bb, draw_direction, context);
      break;
    default:
      // fixme: proper type names
      std::cout << std::format("Element type '{}' not implemented\n",
                               static_cast<uint32_t>(element.type));
      assert(false);
    }
  }

  m_elements.clear();
}