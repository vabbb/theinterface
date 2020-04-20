#include <cstdlib>

#include "cursor.hpp"
#include "server.hpp"
#include "xdg_shell.hpp"
#include "xwayland.hpp"

#include "seat.hpp"

void seat_request_cursor(struct wl_listener *listener, void *data) {
  ti::desktop *desktop = wl_container_of(listener, desktop, request_cursor);
  struct wlr_seat_pointer_request_set_cursor_event *event =
      reinterpret_cast<wlr_seat_pointer_request_set_cursor_event *>(data);
  struct wlr_seat_client *focused_client =
      desktop->seat->pointer_state.focused_client;
  /* This can be sent by any client, so we check to make sure this one is
   * actually has pointer focus first. */
  if (focused_client == event->seat_client) {
    /* Once we've vetted the client, we can tell the cursor to use the
     * provided surface as the cursor image. It will set the hardware cursor
     * on the output that it's currently on and continue to do so as the
     * cursor moves between outputs. */
    wlr_cursor_set_surface(desktop->cursor, event->surface, event->hotspot_x,
                           event->hotspot_y);
  }
}

bool view_at(ti::view *view, double lx, double ly, struct wlr_surface **surface,
             double *sx, double *sy) {
  double view_sx = lx - view->box.x;
  double view_sy = ly - view->box.y;

  double _sx, _sy;
  struct wlr_surface *_surface = nullptr;

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
    if (v->surface) {
      _surface =
          wlr_surface_surface_at(v->surface, view_sx, view_sy, &_sx, &_sy);
    }
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

ti::view *desktop_view_at(ti::desktop *desktop, double lx, double ly,
                          struct wlr_surface **surface, double *sx,
                          double *sy) {
  ti::view *view;
  wl_list_for_each(view, &desktop->wem_views, wem_link) {
    if (view_at(view, lx, ly, surface, sx, sy)) {
      return view;
    }
  }
  return NULL;
}
