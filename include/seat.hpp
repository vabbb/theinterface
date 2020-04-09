#ifndef TI_SEAT_HPP
#define TI_SEAT_HPP

#include "server.hpp"
#include "xdg_shell.hpp"

void seat_request_cursor(struct wl_listener *listener, void *data);
bool view_at(struct ti_xdg_view *view, double lx, double ly,
             struct wlr_surface **surface, double *sx, double *sy);
struct ti_xdg_view *desktop_view_at(ti_server *server, double lx, double ly,
                                    struct wlr_surface **surface, double *sx,
                                    double *sy);

#endif