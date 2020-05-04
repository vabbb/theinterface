#ifndef TI_DESKTOP_HPP
#define TI_DESKTOP_HPP

extern "C" {
#include <wlr/config.h>
#include <wlr/types/wlr_foreign_toplevel_management_v1.h>
#include <wlr/types/wlr_presentation_time.h>
#include <wlr/types/wlr_xcursor_manager.h>
#include <wlr/types/wlr_xdg_shell.h>

#define namespace c_namespace
#include <wlr/types/wlr_layer_shell_v1.h>
#undef namespace
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

  struct wlr_layer_shell_v1 *layer_shell;
  struct wl_listener new_layer_shell_surface;

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

  /** This iterates over all of our surfaces and attempts to find one under the
   *  cursor. This relies on desktop->views being ordered from top-to-bottom. */
  ti::view *view_at(double lx, double ly, struct wlr_surface **surface,
                    double *sx, double *sy);

  desktop(ti::server *s);
  ~desktop();
};
} // namespace ti

#endif