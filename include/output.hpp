#ifndef TI_OUTPUT_H
#define TI_OUTPUT_H

#include "wlroots.hpp"

struct ti_output {
  struct wl_list link;
  struct ti_server *server;
  struct wlr_output *wlr_output;
  struct wl_listener frame;
};

void server_new_output(struct wl_listener *listener, void *data);

#endif