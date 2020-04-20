#ifndef TI_KEYBOARD_HPP
#define TI_KEYBOARD_HPP

namespace ti {
class desktop;
struct keyboard {
  struct wl_list link;
  ti::desktop *desktop;
  struct wlr_input_device *device;

  struct wl_listener modifiers;
  struct wl_listener key;
};
} // namespace ti

#endif