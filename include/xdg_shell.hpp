#ifndef TI_XDG_SHELL_HPP
#define TI_XDG_SHELL_HPP

#include "cursor.hpp"
#include "server.hpp"
#include "view.hpp"

namespace ti {
class xdg_view : public view {
public:
  struct wlr_xdg_surface *xdg_surface;

	struct wl_listener set_app_id;

  std::string get_title() override;
  xdg_view();
  ~xdg_view();
  // wlr_xdg_surface* get_impl_surface<wlr_xdg_surface *>() override;
  // void* set_impl_surface();
};
} // namespace ti

/** Note: this function only deals with keyboard focus. */
// void focus_view(ti::xdg_view *view, struct wlr_surface *surface);

/** This event is raised when wlr_xdg_shell receives a new xdg surface from a
 * client, either a toplevel (application window) or popup. */
void handle_new_xdg_surface(struct wl_listener *listener, void *data);

#endif