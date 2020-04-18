#ifndef TI_OUTPUT_HPP
#define TI_OUTPUT_HPP

extern "C" {
#include <wlr/types/wlr_output_damage.h>
}

namespace ti {
class server;
class view;
class output;
} // namespace ti

typedef void (*ti_surface_iterator_func_t)(ti::output *output,
                                           struct wlr_surface *surface,
                                           struct wlr_box *box, float rotation,
                                           void *user_data);

struct surface_iterator_data {
  ti_surface_iterator_func_t user_iterator;
  void *user_data;
  ti::output *output;
  double ox, oy;
  int width, height;
  float rotation;
};

namespace ti {
struct output {
  struct wl_list link;
  ti::server *server;
  struct wlr_output *wlr_output;
  struct wl_listener frame;

  struct wlr_output_damage *damage;

  void get_decoration_box(ti::view &view, struct wlr_box &box);
  void damage_partial_view(ti::view *view);
  void for_each_surface(ti_surface_iterator_func_t iterator, void *user_data);
  void view_for_each_surface(ti::view *view,
                             ti_surface_iterator_func_t iterator,
                             void *user_data);
  void damage_whole_view(ti::view *view);
};
} // namespace ti

/* This event is rasied by the backend when a new output (aka a display or
 * monitor) becomes available. */
void handle_new_output(struct wl_listener *listener, void *data);

void output_damage_whole_view(ti::view *view, ti::output *output);

void scale_box(struct wlr_box *box, float scale);

#endif