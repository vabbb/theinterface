#ifndef TI_OUTPUT_HPP
#define TI_OUTPUT_HPP

extern "C" {
#include <wlr/types/wlr_output_damage.h>
}

#include "server.hpp"

namespace ti {
struct output {
  struct wl_list link;
  ti::server *server;
  struct wlr_output *wlr_output;
  struct wl_listener frame;

  struct wlr_output_damage *damage;

  void get_decoration_box(ti::view &view, struct wlr_box &box);
};
} // namespace ti

/* This event is rasied by the backend when a new output (aka a display or
 * monitor) becomes available. */
void handle_new_output(struct wl_listener *listener, void *data);
#endif