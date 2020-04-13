#ifndef TI_XWAYLAND_HPP
#define TI_XWAYLAND_HPP

// these need to be imported before <wlr/xwayland.h> otherwise nothing works
#include <cmath>
#include <xcb/xcb.h>

extern "C" {
#define static
#define class c_class
#include <wlr/xwayland.h>
#undef class
#undef static
}

#include "view.hpp"

namespace ti {
class xwayland_view : public view {
public:
  struct wl_listener commit;

  std::string get_title() override;

  /// ti::view->xwayland_surface->surface;
  struct wlr_surface *get_wlr_surface();

  xwayland_view();
  ~xwayland_view();
};
} // namespace ti

/** Called when the surface is destroyed and should never be shown again. */
void handle_xwayland_surface_destroy(struct wl_listener *listener, void *data);

/** This event is raised when wlr_xwayland receives a new xwayland surface */
void handle_new_xwayland_surface(struct wl_listener *listener, void *data);

#endif