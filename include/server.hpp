#ifndef TI_SERVER_HPP
#define TI_SERVER_HPP

extern "C" {
#include <wlr/backend.h>
#include <wlr/config.h>
#include <wlr/types/wlr_xcursor_manager.h>
#include <wlr/types/wlr_xdg_shell.h>
}

#ifdef WLR_HAS_XWAYLAND
#include "xwayland.hpp"
#endif

#include "cursor.hpp"
#include "view.hpp"

namespace ti {
class server {
public:
  struct wl_display *display;
  struct wlr_backend *backend;
  struct wlr_renderer *renderer;
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

  struct wlr_cursor *cursor;
  struct wlr_xcursor_manager *cursor_mgr;
  struct wl_listener cursor_motion;
  struct wl_listener cursor_motion_absolute;
  struct wl_listener cursor_button;
  struct wl_listener cursor_axis;
  struct wl_listener cursor_frame;

  struct wlr_seat *seat;
  struct wl_listener new_input;
  struct wl_listener request_cursor;
  struct wl_list keyboards;
  enum ti::cursor_mode cursor_mode;

  class view *grabbed_view;
  double grab_x, grab_y;
  int grab_width, grab_height;
  uint32_t resize_edges;

  struct wlr_output_layout *output_layout;
  struct wl_list outputs;
  struct wl_listener new_output;

  server();
  ~server();
  void new_keyboard(struct wlr_input_device *device);
  void new_pointer(struct wlr_input_device *device);
};
} // namespace ti

#endif