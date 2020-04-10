#ifndef TI_XDG_SHELL_HPP
#define TI_XDG_SHELL_HPP

#include "cursor.hpp"
#include "server.hpp"
#include "view.hpp"

namespace ti {

// xdg_view is an implementation of the view interface
class xdg_view : public view {
public:
  struct wlr_xdg_surface *xdg_surface;

  xdg_view();
};
} // namespace ti

void focus_view(ti::xdg_view *view, struct wlr_surface *surface);
void server_new_xdg_surface(struct wl_listener *listener, void *data);

#endif