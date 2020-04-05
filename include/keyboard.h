#ifndef TI_KEYBOARD_H
#define TI_KEYBOARD_H

#include <wlr/types/wlr_keyboard.h>

struct ti_keyboard {
  struct wl_list link;
  struct ti_server *server;
  struct wlr_input_device *device;

  struct wl_listener modifiers;
  struct wl_listener key;
};

void server_new_keyboard(struct ti_server *server,
                         struct wlr_input_device *device);

#endif