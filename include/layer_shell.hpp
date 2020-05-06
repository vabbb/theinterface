#ifndef TI_LAYER_SHELL_HPP
#define TI_LAYER_SHELL_HPP

extern "C" {
#include <wlr/types/wlr_box.h>
#define namespace c_namespace
#include <wlr/types/wlr_layer_shell_v1.h>
#undef namespace
}

namespace ti {
class output;
class layer_view {
public:
  struct wlr_layer_surface_v1 *layer_surface; // *data points to wrapper class
  struct wl_list link;                        // ti::layer_view

  struct wl_listener destroy;
  struct wl_listener map;
  struct wl_listener unmap;
  struct wl_listener surface_commit;
  struct wl_listener output_destroy;
  struct wl_listener new_popup;

  struct wlr_box geo;
  enum zwlr_layer_shell_v1_layer layer;

  void update_cursor(ti::seat *seat);
  layer_view();
  ~layer_view();
};

class layer_popup {
public:
  class layer_view *parent;
  struct wlr_xdg_popup *wlr_popup;
  struct wl_listener map;
  struct wl_listener unmap;
  struct wl_listener destroy;
  struct wl_listener commit;

  layer_popup();
  ~layer_popup();
};
} // namespace ti

void arrange_layers(ti::output *output);

void handle_new_layer_shell_surface(struct wl_listener *listener, void *data);
#endif