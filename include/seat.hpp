#ifndef TI_SEAT_HPP
#define TI_SEAT_HPP

#include "server.hpp"
#include "view.hpp"

void seat_request_cursor(struct wl_listener *listener, void *data);
bool view_at(ti::view *view, double lx, double ly, struct wlr_surface **surface,
             double *sx, double *sy);
ti::view *desktop_view_at(ti::server *server, double lx, double ly,
                          struct wlr_surface **surface, double *sx, double *sy);

#endif