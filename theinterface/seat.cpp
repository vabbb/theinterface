#include <cstdlib>

#include "server.hpp"
#include "xdg_shell.hpp"
#include "xwayland.hpp"

#include "seat.hpp"

void seat_request_cursor(struct wl_listener *listener, void *data) {
  ti::server *server = wl_container_of(listener, server, request_cursor);
  struct wlr_seat_pointer_request_set_cursor_event *event =
      reinterpret_cast<wlr_seat_pointer_request_set_cursor_event *>(data);
  struct wlr_seat_client *focused_client =
      server->seat->pointer_state.focused_client;
  /* This can be sent by any client, so we check to make sure this one is
   * actually has pointer focus first. */
  if (focused_client == event->seat_client) {
    /* Once we've vetted the client, we can tell the cursor to use the
     * provided surface as the cursor image. It will set the hardware cursor
     * on the output that it's currently on and continue to do so as the
     * cursor moves between outputs. */
    wlr_cursor_set_surface(server->cursor, event->surface, event->hotspot_x,
                           event->hotspot_y);
  }
}

bool view_at(ti::view *view, double lx, double ly, struct wlr_surface **surface,
             double *sx, double *sy) {
  double view_sx = lx - view->x;
  double view_sy = ly - view->y;

  double _sx, _sy;
  struct wlr_surface *_surface = NULL;

  switch (view->type) {
  case ti::XDG_SHELL_VIEW: {
    ti::xdg_view *v = dynamic_cast<ti::xdg_view *>(view);
    // auto *state = &v->xdg_surface->surface->current;
    _surface = wlr_xdg_surface_surface_at(v->xdg_surface, view_sx, view_sy,
                                          &_sx, &_sy);
    break;
  }
  case ti::XWAYLAND_VIEW: {
    ti::xwayland_view *v = dynamic_cast<ti::xwayland_view *>(view);
    if (v->get_wlr_surface()) {
      _surface = wlr_surface_surface_at(v->get_wlr_surface(), view_sx, view_sy,
                                        &_sx, &_sy);
    }
    break;
  }
  default: {
    break;
  }
  }

  if (_surface != NULL) {
    *sx = _sx;
    *sy = _sy;
    *surface = _surface;
    return true;
  }
  return false;
}

ti::view *desktop_view_at(ti::server *server, double lx, double ly,
                          struct wlr_surface **surface, double *sx,
                          double *sy) {
  ti::view *view;
  wl_list_for_each(view, &server->wem_views, wem_link) {
    if (view_at(view, lx, ly, surface, sx, sy)) {
      return view;
    }
  }
  return NULL;
}
