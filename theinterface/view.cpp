#include "output.hpp"
#include "xdg_shell.hpp"
#include "xwayland.hpp"

#include "output.hpp"

#include "view.hpp"

// mapped is false, so we only map it when the view is ready
ti::view::view(enum view_type t)
    : type(t), mapped(false), was_ever_mapped(false), decorated(false),
      alpha(1.0f), title("(nil)") {}
ti::view::view(enum view_type t, int32_t __x, int32_t __y)
    : type(t), mapped(false), was_ever_mapped(false), decorated(false),
      alpha(1.0f), title("(nil)") {
  box.x = __x;
  box.y = __y;
}
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
  case ti::XDG_SHELL_VIEW: {
    ti::xdg_view *v = dynamic_cast<ti::xdg_view *>(this);
    wlr_xdg_toplevel_set_activated(v->xdg_surface, true);
    surf = v->xdg_surface->surface;
    break;
  }
  case ti::XWAYLAND_VIEW: {
    ti::xwayland_view *v = dynamic_cast<ti::xwayland_view *>(this);
    wlr_xwayland_surface_activate(v->xwayland_surface, true);
    surf = v->xwayland_surface->surface;
    break;
  }
  }

  /*
   * Tell the seat to have the keyboard enter this surface. wlroots will keep
   * track of this and automatically send key events to the appropriate
   * clients without additional work on your part.
   */
  wlr_seat_keyboard_notify_enter(server->seat, surf, keyboard->keycodes,
                                 keyboard->num_keycodes, &keyboard->modifiers);
}

void ti::view::get_box(wlr_box &_box) {
  _box.x = box.x;
  _box.y = box.y;
  _box.width = box.width;
  _box.height = box.height;
}

void ti::view::get_deco_box(wlr_box &_box) {
  ti::view::get_box(_box);
  if (!decorated) {
    return;
  }

  _box.x -= border_width;
  _box.y -= (border_width + titlebar_height);
  _box.width += border_width * 2;
  _box.height += (border_width * 2 + titlebar_height);
}

void ti::view::for_each_surface(wlr_surface_iterator_func_t iterator,
                                void *user_data) {
  wlr_surface_for_each_surface(surface, iterator, user_data);
}

void ti::view::damage_whole() {
  output *output;
  wl_list_for_each(output, &this->server->outputs, link) {
    output_damage_whole_view(this, output);
  }
}

void ti::view::update_position(int32_t __x, int32_t __y) {
  if (box.x == __x && box.y == __y) {
    return;
  }

  ti::view::damage_whole();
  box.x = __x;
  box.y = __y;
  ti::view::damage_whole();
}