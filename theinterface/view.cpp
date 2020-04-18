extern "C" {
#include <wlr/types/wlr_xdg_shell.h>
}

#include "output.hpp"
#include "xdg_shell.hpp"
#include "xwayland.hpp"

#include "output.hpp"
#include "render.hpp"
#include "server.hpp"

#include "view.hpp"

// mapped is false, so we only map it when the view is ready
ti::view::view(enum view_type t) : type(t) {}
ti::view::view(enum view_type t, int32_t __x, int32_t __y) : type(t) {
  box.x = __x;
  box.y = __y;
}
ti::view::~view() {}

void ti::view::focus() {
  ti::view *prev_focus = server->focused_view;
  if (prev_focus == this) {
    return;
  } else if (prev_focus) {
    prev_focus->deactivate();
  }

  /* Move the view to the front */
  if (was_ever_mapped) {
    wl_list_remove(&wem_link);
    wl_list_insert(&server->wem_views, &wem_link);
  }
  wl_list_remove(&link);
  wl_list_insert(&server->views, &link);

  this->damage_whole();

  this->activate();
  server->focused_view = this;

  struct wlr_keyboard *keyboard = wlr_seat_get_keyboard(server->seat);

  /*
   * Tell the seat to have the keyboard enter this surface. wlroots will keep
   * track of this and automatically send key events to the appropriate
   * clients without additional work on your part.
   */
  wlr_seat_keyboard_notify_enter(server->seat, this->surface,
                                 keyboard->keycodes, keyboard->num_keycodes,
                                 &keyboard->modifiers);
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

void ti::view::update_position(int32_t __x, int32_t __y) {
  if (box.x == __x && box.y == __y) {
    return;
  }

  this->damage_whole();
  box.x = __x;
  box.y = __y;
  this->damage_whole();
}

void ti::view::begin_interactive(ti::cursor_mode mode, uint32_t edges) {
  if (surface != server->seat->pointer_state.focused_surface) {
    /* Deny move/resize requests from unfocused clients. */
    return;
  }
  server->grabbed_view = this;
  server->cursor_mode = mode;
  struct wlr_box geo_box;
  switch (type) {
  case ti::XDG_SHELL_VIEW: {
    ti::xdg_view *v = dynamic_cast<ti::xdg_view *>(this);
    wlr_xdg_surface_get_geometry(v->xdg_surface, &geo_box);
    break;
  }
  case ti::XWAYLAND_VIEW: {
    ti::xwayland_view *v = dynamic_cast<ti::xwayland_view *>(this);
    geo_box.width = v->xwayland_surface->width;
    geo_box.height = v->xwayland_surface->height;
    break;
  }
  }

  server->view_x = box.x;
  server->view_y = box.y;

  if (mode == ti::CURSOR_MOVE) {
    server->grab_x = server->cursor->x - box.x;
    server->grab_y = server->cursor->y - box.y;
  } else {
    server->grab_x = server->cursor->x;
    server->grab_y = server->cursor->y;
  }
  server->grab_width = geo_box.width;
  server->grab_height = geo_box.height;
  server->resize_edges = edges;
}

void ti::view::damage_partial() {
  ti::output *output;
  wl_list_for_each(output, &server->outputs, link) {
    output->damage_partial_view(this);
  }
}

void ti::view::damage_whole() {
  ti::output *output;
  wl_list_for_each(output, &server->outputs, link) {
    output->damage_whole_view(this);
  }
}

void ti::view::render(ti::output *output, ti::render_data *data) {
  this->render_decorations(output, data);
  output->view_for_each_surface(this, render_surface_iterator, data);
}