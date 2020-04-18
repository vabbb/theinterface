#ifndef TI_XDG_SHELL_HPP
#define TI_XDG_SHELL_HPP

#include "view.hpp"

namespace ti {
class xdg_view : public view {
public:
  struct wlr_xdg_surface *xdg_surface;

  struct wl_listener set_app_id;

  std::string get_title() override;
  void for_each_surface(wlr_surface_iterator_func_t iterator,
                        void *user_data) override;
  void activate() override;
  void deactivate() override;

  xdg_view();
  ~xdg_view();
};
} // namespace ti

/** This event is raised when wlr_xdg_shell receives a new xdg surface from a
 * client, either a toplevel (application window) or popup. */
void handle_new_xdg_surface(struct wl_listener *listener, void *data);

#endif