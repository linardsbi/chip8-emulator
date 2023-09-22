#pragma once

#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

enum class ElementType { Textbox, InputBox, ContainerStart, ContainerEnd };

enum class DrawDirection { Horizontal, Vertical };

struct Attributes {
  virtual ~Attributes() {}
};

struct ContainerAttributes : Attributes {
  ContainerAttributes(DrawDirection dd) : direction(dd) {}

  DrawDirection direction;
};

struct RenderedProperties {
  uint32_t width;
  uint32_t height;
  uint32_t x;
  uint32_t y;
};

struct Element {
  using AttributePtr = std::shared_ptr<Attributes>;

  Element(AttributePtr attributes, std::string &&content, ElementType type)
      : attributes(attributes), content(content), type(type) {}

  RenderedProperties rendered_properties;
  AttributePtr attributes;
  std::string content;
  ElementType type;
};

struct Context;

class UI {
public:
  void textbox(std::string_view text) {
    m_elements.emplace_back(
        Element{{}, std::string{text}, ElementType::Textbox});
  }

  void inputbox(std::string_view text) {
    m_elements.emplace_back(
        Element{{}, std::string{text}, ElementType::Textbox});
  }

  bool container_start(ContainerAttributes attributes = {
                           DrawDirection::Horizontal}) {
    // will using attributes like this break everything because of automatic
    // storage duration?
    m_elements.emplace_back(
        Element{std::make_shared<ContainerAttributes>(attributes),
                {},
                ElementType::ContainerStart});
    return true;
  }

  bool container_end() {
    m_elements.emplace_back(Element{{}, {}, ElementType::ContainerEnd});
    return true;
  }

  bool button(std::string_view text) {
    // i need to know where the button is to check whether it is pressed
    // to do that, i need to save the rendered elements somewhere
    // but i also need to remove the old elements from the window

    // i could create an element lookup table with references to those elements 
  }

  void render(Context &);

private:
  std::vector<Element> m_elements;
};
