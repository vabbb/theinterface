#ifndef TI_XDG_SHELL_HPP
#define TI_XDG_SHELL_HPP

#include "cursor.hpp"

struct ti_xdg_view {
  struct wl_list link;
  struct ti_server *server;
  struct wlr_xdg_surface *xdg_surface;
  struct wl_listener map;
  struct wl_listener unmap;
  struct wl_listener destroy;
  struct wl_listener request_move;
  struct wl_listener request_resize;

  pid_t pid;
  bool mapped;
  int x, y;
};

void focus_view(struct ti_xdg_view *view, struct wlr_surface *surface);
void server_new_xdg_surface(struct wl_listener *listener, void *data);

#endif