#ifndef TI_XWAYLAND_HPP
#define TI_XWAYLAND_HPP

// these need to be imported before <wlr/xwayland.h> otherwise nothing works
#include <cmath>
#include <xcb/xcb.h>

extern "C" {
#define static
// for some reason wlr/xwayland.h is using "class" as a variable name 🤷
#define class c_class
#include <wlr/xwayland.h>
#undef class
#undef static
}

#include "view.hpp"

namespace ti {
class xwayland_view : public view {
public:
  struct wlr_xwayland_surface *xwayland_surface;

  struct wl_listener commit;
  struct wl_listener request_configure;

  std::string get_title() override;

  xwayland_view();
  ~xwayland_view();
};
} // namespace ti

/** Called when the surface is destroyed and should never be shown again. */
void handle_xwayland_surface_destroy(struct wl_listener *listener, void *data);

/** This event is raised when wlr_xwayland receives a new xwayland surface */
void handle_new_xwayland_surface(struct wl_listener *listener, void *data);

#endif