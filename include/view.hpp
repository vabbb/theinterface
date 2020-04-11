#ifndef TI_VIEW_HPP
#define TI_VIEW_HPP

#include <string>

extern "C" {
#include <wayland-util.h>

#include <wlr/config.h>
#include <wlr/types/wlr_surface.h>
#include <wlr/types/wlr_xdg_shell.h>
}

extern "C" {
#define static
#define class c_class
#include <wlr/xwayland.h>
#undef class
#undef static
}

namespace ti {
enum view_type {
  XDG_SHELL_VIEW,
#ifdef WLR_HAS_XWAYLAND
  XWAYLAND_VIEW,
#endif
};

/// view interface
class view {
public:
  int x, y;
  pid_t pid;
  bool mapped;

  class server *server;
  struct wl_list link;     // ti::server::views
  struct wl_list children; // ti::view_child::link
  // struct wlr_surface *surface;

  struct wl_listener map;
  struct wl_listener unmap;
  struct wl_listener destroy;
  struct wl_listener request_move;
  struct wl_listener request_resize;
  struct wl_listener new_subsurface;

  const enum view_type type;
  union {
    struct wlr_xwayland_surface *xwayland_surface;
    struct wlr_xdg_surface *xdg_surface;
  };

  view(enum view_type t);
  virtual ~view() = 0;
  virtual std::string get_title() = 0;
};
} // namespace ti

#endif