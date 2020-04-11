#ifndef TI_OUTPUT_HPP
#define TI_OUTPUT_HPP

#include "server.hpp"

namespace ti {
struct output {
  struct wl_list link;
  ti::server *server;
  struct wlr_output *wlr_output;
  struct wl_listener frame;
};
} // namespace ti

void server_new_output(struct wl_listener *listener, void *data);

#endif