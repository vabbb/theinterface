#include "view.hpp"
#include "output.hpp"

// mapped is false, so we only map it when the view is ready
ti::view::view(enum view_type t)
    : x(0), y(0), mapped(false), was_ever_mapped(false), type(t) {}
ti::view::~view() {}

void ti::view::focus(struct wlr_surface *surface) {
  struct wlr_surface *prev_surface =
      server->seat->keyboard_state.focused_surface;
  if (prev_surface == surface) {
    /* Don't re-focus an already focused surface. */
    return;
  }
  if (prev_surface) {
    /*
     * Deactivate the previously focused surface. This lets the client know
     * it no longer has focus and the client will repaint accordingly, e.g.
     * stop displaying a caret.
     */
    if (wlr_surface_is_xdg_surface(prev_surface)) {
      struct wlr_xdg_surface *previous = wlr_xdg_surface_from_wlr_surface(
          server->seat->keyboard_state.focused_surface);
      wlr_xdg_toplevel_set_activated(previous, false);
    } else if (wlr_surface_is_xwayland_surface(prev_surface)) {
      struct wlr_xwayland_surface *previous =
          wlr_xwayland_surface_from_wlr_surface(
              server->seat->keyboard_state.focused_surface);
      wlr_xwayland_surface_activate(previous, false);
    }
  }
  struct wlr_keyboard *keyboard = wlr_seat_get_keyboard(server->seat);

  /* Move the view to the front */
  if (was_ever_mapped) {
    wl_list_remove(&wem_link);
    wl_list_insert(&server->wem_views, &wem_link);
  }
  wlr_surface *surf;
  /* Activate the new surface */
  switch (type) {
  case ti::XDG_SHELL_VIEW:
    wlr_xdg_toplevel_set_activated(xdg_surface, true);
    surf = xdg_surface->surface;
    break;
  case ti::XWAYLAND_VIEW:
    wlr_xwayland_surface_activate(xwayland_surface, true);
    surf = xwayland_surface->surface;
    break;
  }
  /*
   * Tell the seat to have the keyboard enter this surface. wlroots will keep
   * track of this and automatically send key events to the appropriate
   * clients without additional work on your part.
   */

  wlr_seat_keyboard_notify_enter(server->seat, surf, keyboard->keycodes,
                                 keyboard->num_keycodes, &keyboard->modifiers);
}
