#ifndef TI_KEYBOARD_HPP
#define TI_KEYBOARD_HPP

namespace ti {
class seat;
struct keyboard {
  struct wl_list link;
  ti::seat *seat;
  struct wlr_input_device *device;

  struct wl_listener modifiers;
  struct wl_listener key;
};
} // namespace ti

#endif