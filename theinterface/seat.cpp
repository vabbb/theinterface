#include <cstdlib>

#include "wlroots.hpp"

#include "seat.hpp"
#include "xdg_shell.hpp"

#include "server.hpp"

void seat_request_cursor(struct wl_listener *listener, void *data) {
  struct ti_server *server = wl_container_of(listener, server, request_cursor);
  /* This event is rasied by the seat when a client provides a cursor image */
  struct wlr_seat_pointer_request_set_cursor_event *event =
      (struct wlr_seat_pointer_request_set_cursor_event *)data;
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

bool view_at(struct ti_xdg_view *view, double lx, double ly,
             struct wlr_surface **surface, double *sx, double *sy) {
  /*
   * XDG toplevels may have nested surfaces, such as popup windows for context
   * menus or tooltips. This function tests if any of those are underneath the
   * coordinates lx and ly (in output Layout Coordinates). If so, it sets the
   * surface pointer to that wlr_surface and the sx and sy coordinates to the
   * coordinates relative to that surface's top-left corner.
   */
  double view_sx = lx - view->x;
  double view_sy = ly - view->y;

  struct wlr_surface_state *state = &view->xdg_surface->surface->current;

  double _sx, _sy;
  struct wlr_surface *_surface = NULL;
  _surface = wlr_xdg_surface_surface_at(view->xdg_surface, view_sx, view_sy,
                                        &_sx, &_sy);

  if (_surface != NULL) {
    *sx = _sx;
    *sy = _sy;
    *surface = _surface;
    return true;
  }

  return false;
}

struct ti_xdg_view *desktop_view_at(struct ti_server *server, double lx,
                                    double ly, struct wlr_surface **surface,
                                    double *sx, double *sy) {
  /* This iterates over all of our surfaces and attempts to find one under the
   * cursor. This relies on server->views being ordered from top-to-bottom. */
  struct ti_xdg_view *view;
  wl_list_for_each(view, &server->views, link) {
    if (view_at(view, lx, ly, surface, sx, sy)) {
      return view;
    }
  }
  return NULL;
}
