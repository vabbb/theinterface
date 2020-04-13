#ifndef TI_KEYBOARD_HPP
#define TI_KEYBOARD_HPP

#include "server.hpp"

namespace ti {
struct keyboard {
  struct wl_list link;
  ti::server *server;
  struct wlr_input_device *device;

  struct wl_listener modifiers;
  struct wl_listener key;
};
} // namespace ti

#endif