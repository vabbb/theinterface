#ifndef TI_VIEW_HPP
#define TI_VIEW_HPP

#include <string>

extern "C" {
#include <wayland-util.h>

#include <wlr/config.h>
#include <wlr/types/wlr_surface.h>
#include <wlr/types/wlr_xdg_shell.h>
#if WLR_HAS_WAYLAND
#include <wlr/xwayland.h>
#endif
}

namespace ti {
enum view_type {
  TI_XDG_SHELL_VIEW,
#if WLR_HAS_WAYLAND
  TI_XWAYLAND_VIEW,
#endif
};

class view {
public:
  int x, y;
  pid_t pid;
  bool mapped;
  enum view_type type;

  class server *server;
  struct wl_list link;     // ti::server::views
  struct wl_list children; // ti::view_child::link
  struct wlr_surface *surface;

  struct wl_listener map;
  struct wl_listener unmap;
  struct wl_listener destroy;
  struct wl_listener request_move;
  struct wl_listener request_resize;
  struct wl_listener new_subsurface;

  view();
  // virtual ~view() = 0;
  // virtual std::string getTitle() = 0;
  // virtual void getGeometry(int *width_out, int *height_out) = 0;
  // virtual bool isPrimary() = 0;
  // virtual bool isTransient_for(view *child, view *parent) = 0;
  // virtual void activate(bool activate) = 0;
  // virtual void maximize(int output_width, int output_height) = 0;
  // virtual void forEachSurface(wlr_surface_iterator_func_t iterator,
  //                             void *data) = 0;
  // virtual void forEachPopup(wlr_surface_iterator_func_t iterator,
  //                           void *data) = 0;
  // virtual struct wlr_surface *wlr_surface_at(double sx, double sy,
  //                                            double *sub_x, double *sub_y) =
  //                                            0;
};
} // namespace ti

#endif