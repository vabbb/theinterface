#ifndef TI_XDG_SHELL_HPP
#define TI_XDG_SHELL_HPP

#include "cursor.hpp"
#include "server.hpp"
#include "view.hpp"

namespace ti {
class xdg_view : public view {
public:
  std::string get_title() override;
  xdg_view();
  ~xdg_view();
  // wlr_xdg_surface* get_impl_surface<wlr_xdg_surface *>() override;
  // void* set_impl_surface();
};
} // namespace ti

void focus_view(ti::xdg_view *view, struct wlr_surface *surface);
void server_new_xdg_surface(struct wl_listener *listener, void *data);

#endif