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
  xwayland_view();
  ~xwayland_view();
};
} // namespace ti

void handle_new_xwayland_surface(struct wl_listener *listener, void *data);

#endif