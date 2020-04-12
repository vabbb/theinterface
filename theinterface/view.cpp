#include "view.hpp"
#include "output.hpp"

// mapped is false, so we only map it when the view is ready
ti::view::view(enum view_type t) : x(0), y(0), mapped(false), type(t) {}
ti::view::~view() {}

void ti::view::focus(struct wlr_surface *surface) {
  struct wlr_surface *prev_surface =
      server->seat->keyboard_state.focused_surface;
  if (prev_surface == surface) {
    /* Don't re-focus an already focused surface. */
    return;
  }

  if (prev_surface && wlr_surface_is_xdg_surface(prev_surface)) {
    /*
     * Deactivate the previously focused surface. This lets the client know
     * it no longer has focus and the client will repaint accordingly, e.g.
     * stop displaying a caret.
     */
    struct wlr_xdg_surface *previous = wlr_xdg_surface_from_wlr_surface(
        server->seat->keyboard_state.focused_surface);
    wlr_xdg_toplevel_set_activated(previous, false);
  }
  struct wlr_keyboard *keyboard = wlr_seat_get_keyboard(server->seat);
  /* Move the view to the front */
  wl_list_remove(&link);
  wl_list_insert(&server->views, &link);

  switch (type) {
  case ti::XDG_SHELL_VIEW:
    /* Activate the new surface */
    wlr_xdg_toplevel_set_activated(xdg_surface, true);
    break;
  case ti::XWAYLAND_VIEW:
    wlr_xwayland_surface_activate(xwayland_surface, true);
    break;
  }
  /*
   * Tell the seat to have the keyboard enter this surface. wlroots will keep
   * track of this and automatically send key events to the appropriate
   * clients without additional work on your part.
   */
  wlr_seat_keyboard_notify_enter(server->seat, xdg_surface->surface,
                                 keyboard->keycodes, keyboard->num_keycodes,
                                 &keyboard->modifiers);
}
