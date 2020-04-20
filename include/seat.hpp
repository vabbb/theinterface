#ifndef TI_SEAT_HPP
#define TI_SEAT_HPP

#include "desktop.hpp"
#include "view.hpp"

/* This event is rasied by the seat when a client provides a cursor image */
void seat_request_cursor(struct wl_listener *listener, void *data);

/*
 * XDG toplevels may have nested surfaces, such as popup windows for context
 * menus or tooltips. This function tests if any of those are underneath the
 * coordinates lx and ly (in output Layout Coordinates). If so, it sets the
 * surface pointer to that wlr_surface and the sx and sy coordinates to the
 * coordinates relative to that surface's top-left corner.
 */
bool view_at(ti::view *view, double lx, double ly, struct wlr_surface **surface,
             double *sx, double *sy);

/** This iterates over all of our surfaces and attempts to find one under the
 *  cursor. This relies on desktop->views being ordered from top-to-bottom. */
ti::view *desktop_view_at(ti::desktop *desktop, double lx, double ly,
                          struct wlr_surface **surface, double *sx, double *sy);

#endif