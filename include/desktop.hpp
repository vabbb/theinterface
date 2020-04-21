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
class seat;
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
#endif

  struct wl_list views;

  /// were-ever-mapped views
  /// All the views that have been mapped at least once. This is used to make
  /// Alt+Tab work
  struct wl_list wem_views;
  struct wlr_foreign_toplevel_manager_v1 *foreign_toplevel_manager_v1;

  struct wl_listener new_input;

  class ti::seat *seat;

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