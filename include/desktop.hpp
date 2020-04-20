#ifndef TI_DESKTOP_HPP
#define TI_DESKTOP_HPP

extern "C" {
#include <wlr/config.h>
#include <wlr/types/wlr_foreign_toplevel_management_v1.h>
#include <wlr/types/wlr_presentation_time.h>
#include <wlr/types/wlr_xcursor_manager.h>
#include <wlr/types/wlr_xdg_shell.h>
}

#ifdef WLR_HAS_XWAYLAND
#include "xwayland.hpp"
#endif

namespace ti {
class server;
enum cursor_mode;

class desktop {
public:
  class ti::server *server;
  struct wlr_compositor *compositor;

  struct wlr_xdg_shell *xdg_shell;
  struct wl_listener new_xdg_surface;

#ifdef WLR_HAS_XWAYLAND
  struct wlr_xwayland *xwayland;
  struct wl_listener new_xwayland_surface;
  struct wlr_xcursor *xcursor;
  struct wlr_xcursor_manager *xwayland_xcursor_manager;
#endif

  struct wl_list views;

  /// were-ever-mapped views
  /// All the views that have been mapped at least once. This is used to make
  /// Alt+Tab work
  struct wl_list wem_views;
  struct wlr_foreign_toplevel_manager_v1 *foreign_toplevel_manager_v1;

  struct wl_listener new_input;
  struct wlr_cursor *cursor;
  struct wlr_xcursor_manager *cursor_mgr;
  struct wl_listener cursor_motion;
  struct wl_listener cursor_motion_absolute;
  struct wl_listener cursor_button;
  struct wl_listener cursor_axis;
  struct wl_listener cursor_frame;
  int view_x = 0, view_y = 0;

  struct wlr_seat *seat;
  struct wl_listener request_cursor;
  struct wl_list keyboards;
  enum ti::cursor_mode cursor_mode;

  class view *focused_view = nullptr;

  class view *grabbed_view = nullptr;
  double grab_x = 0, grab_y = 0;
  int grab_width, grab_height;
  uint32_t resize_edges;

  struct wlr_output_layout *output_layout;
  struct wl_list outputs;
  struct wl_listener new_output;
  struct wlr_presentation *presentation;

  void new_keyboard(struct wlr_input_device *device);

  /** We don't do anything special with pointers. All of our pointer handling
   * is proxied through wlr_cursor. On another compositor, you might take this
   * opportunity to do libinput configuration on the device to set
   * acceleration, etc. */
  void new_pointer(struct wlr_input_device *device);

  desktop(ti::server *s);
  ~desktop();
};
} // namespace ti

#endif