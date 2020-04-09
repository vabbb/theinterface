#ifndef TI_OUTPUT_HPP
#define TI_OUTPUT_HPP

#include "server.hpp"

struct ti_output {
  struct wl_list link;
  ti_server *server;
  struct wlr_output *wlr_output;
  struct wl_listener frame;
};

void server_new_output(struct wl_listener *listener, void *data);

#endif