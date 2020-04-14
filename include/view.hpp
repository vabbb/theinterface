#ifndef TI_VIEW_HPP
#define TI_VIEW_HPP

#include <string>
#include <variant>

extern "C" {
#include <wayland-util.h>

#include <wlr/config.h>
#include <wlr/types/wlr_foreign_toplevel_management_v1.h>
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
  const enum view_type type;
  uint32_t x, y;
  uint32_t border_width, titlebar_height;
  bool decorated = false;

  struct wlr_box box;
  float rotation;
  float alpha;

  bool maximized;
  struct ti::output *fullscreen_output;

  std::string title;
  pid_t pid;
  uid_t uid;
  gid_t gid;
  bool mapped, was_ever_mapped;

  class server *server;
  struct wl_list link;     // ti::server::views
  struct wl_list wem_link; // ti::server::wem_views
  struct wl_list children; // ti::view_child::link
  struct wlr_surface *surface;

  struct wl_listener set_title;

  struct wl_listener map;
  struct wl_listener unmap;
  struct wl_listener destroy;
  struct wl_listener request_move;
  struct wl_listener request_resize;
  struct wl_listener new_subsurface;

  struct wl_listener surface_commit;

  struct wlr_foreign_toplevel_handle_v1 *toplevel_handle;
  struct wl_listener toplevel_handle_request_maximize;
  struct wl_listener toplevel_handle_request_activate;
  struct wl_listener toplevel_handle_request_fullscreen;
  struct wl_listener toplevel_handle_request_close;

  view(enum view_type t);
  virtual ~view() = 0;
  virtual std::string get_title() = 0;
  void focus(struct wlr_surface *surface);
};
} // namespace ti

#endif