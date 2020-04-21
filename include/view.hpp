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

#include "cursor.hpp"

namespace ti {
enum view_type {
  XDG_SHELL_VIEW,
#ifdef WLR_HAS_XWAYLAND
  XWAYLAND_VIEW,
#endif
};

struct render_data;
class desktop;

/// view interface
class view {
public:
  const enum view_type type;
  bool mapped = false, was_ever_mapped = false;
  uint32_t border_width, titlebar_height;
  bool decorated = false;

  struct wlr_box box {};
  float rotation = 0.0;
  float alpha = 1.0;

  bool maximized = false;
  struct output *fullscreen_output = nullptr;

  std::string title = "(nil)";
  pid_t pid;
  uid_t uid;
  gid_t gid;

  ti::desktop *desktop;
  struct wl_list link;     // ti::desktop::views
  struct wl_list wem_link; // ti::desktop::wem_views
  struct wl_list children; // ti::view_child::link
  struct wlr_surface *surface = nullptr;

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

  /** NOTE: this function only deals with keyboard focus. */
  void focus();

  virtual void for_each_surface(wlr_surface_iterator_func_t iterator,
                                void *user_data) = 0;
  void damage_whole();
  void damage_partial();
  void update_position(int32_t __x, int32_t __y);
  void render_decorations(ti::output *output, ti::render_data *rdata);
  void render(ti::output *output, ti::render_data *data);
  virtual void activate() = 0;
  virtual void deactivate() = 0;

  /** This function sets up an interactive move or resize operation, where the
   * compositor stops propegating pointer events to clients and instead consumes
   * them itself, to move or resize windows. */
  void begin_interactive(ti::cursor_mode mode, uint32_t edges);
};
} // namespace ti

#endif