#ifndef TI_SEAT_HPP
#define TI_SEAT_HPP

#include "desktop.hpp"

namespace ti {
class desktop;
class view;
class seat {
public:
  class ti::desktop *desktop;
  struct wlr_seat *wlr_seat;
  struct wl_list keyboards;

  struct wlr_cursor *cursor;
  struct wlr_xcursor_manager *cursor_mgr;
#ifdef WLR_HAS_XWAYLAND
  struct wlr_xcursor *xcursor;
  struct wlr_xcursor_manager *xwayland_xcursor_manager;
#endif
  enum ti::cursor_mode cursor_mode;

  struct wl_listener request_cursor;

  struct wl_listener cursor_motion;
  struct wl_listener cursor_motion_absolute;
  struct wl_listener cursor_button;
  struct wl_listener cursor_axis;
  struct wl_listener cursor_frame;

  class ti::view *focused_view = nullptr;

  int view_x = 0, view_y = 0;

  class view *grabbed_view = nullptr;
  double grab_x = 0, grab_y = 0;
  int grab_width, grab_height;
  int resize_edges;

  void setup_xwayland_cursor(wlr_xwayland *xwayland);

  /** NOTE: this function only deals with keyboard focus. */
  void focus(ti::view *v);

  void new_keyboard(struct wlr_input_device *device);

  /** We don't do anything special with pointers. All of our pointer handling
   * is proxied through wlr_cursor. On another compositor, you might take this
   * opportunity to do libinput configuration on the device to set
   * acceleration, etc. */
  void new_pointer(struct wlr_input_device *device);

  seat(ti::desktop *d);
  ~seat();
};
} // namespace ti

/* This event is rasied by the seat when a client provides a cursor image */
void seat_request_cursor(struct wl_listener *listener, void *data);

/*
 * XDG toplevels may have nested surfaces, such as popup windows for context
 * menus or tooltips. This function tests if any of those are underneath the
 * coordinates lx and ly (in output Layout Coordinates). If so, it sets the
 * surface pointer to that wlr_surface and the sx and sy coordinates to the
 * coordinates relative to that surface's top-left corner.
 */
bool view_at(ti::view *view, double lx, double ly, struct wlr_surface **surface,
             double *sx, double *sy);

/** This iterates over all of our surfaces and attempts to find one under the
 *  cursor. This relies on desktop->views being ordered from top-to-bottom. */
ti::view *desktop_view_at(ti::desktop *desktop, double lx, double ly,
                          struct wlr_surface **surface, double *sx, double *sy);

#endif