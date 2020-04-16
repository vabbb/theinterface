#ifndef TI_VIEW_HPP
#define TI_VIEW_HPP

#include <string>

extern "C" {
#include <wayland-util.h>

#include <wlr/config.h>
#include <wlr/types/wlr_box.h>
#include <wlr/types/wlr_foreign_toplevel_management_v1.h>
#include <wlr/types/wlr_surface.h>
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
  bool mapped, was_ever_mapped;
  uint32_t border_width, titlebar_height;
  bool decorated;

  struct wlr_box box;
  float rotation;
  float alpha;

  bool maximized;
  struct output *fullscreen_output;

  std::string title;
  pid_t pid;
  uid_t uid;
  gid_t gid;

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
  view(enum view_type t, int32_t __x, int32_t __y);
  virtual ~view() = 0;
  void get_box(wlr_box &box);
  void get_deco_box(wlr_box &box);
  virtual std::string get_title() = 0;
  void focus(struct wlr_surface *surface);
  void for_each_surface(wlr_surface_iterator_func_t iterator, void *user_data);
  void damage_whole();
  void update_position(int32_t __x, int32_t __y);
};
} // namespace ti

#endif