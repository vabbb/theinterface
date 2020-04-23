extern "C" {
#include <wlr/types/wlr_xdg_shell.h>
}

#include "desktop.hpp"
#include "output.hpp"
#include "render.hpp"
#include "seat.hpp"
#include "server.hpp"
#include "xdg_shell.hpp"
#include "xwayland.hpp"

#include "view.hpp"

// mapped is false, so we only map it when the view is ready
ti::view::view(enum view_type t) : type(t) {}
ti::view::view(enum view_type t, int __x, int __y) : type(t) {
  box.x = __x;
  box.y = __y;
}
ti::view::~view() {}

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

void ti::view::update_position(int __x, int __y) {
  if (box.x == __x && box.y == __y) {
    return;
  }

  this->damage_whole();
  box.x = __x;
  box.y = __y;
  this->damage_whole();
}

void ti::view::begin_interactive(ti::cursor_mode mode, unsigned edges) {
  ti::seat *seat = this->desktop->seat;
  if (surface != seat->wlr_seat->pointer_state.focused_surface) {
    /* Deny move/resize requests from unfocused clients. */
    return;
  }
  seat->grabbed_view = this;
  seat->cursor_mode = mode;
  struct wlr_box geo_box;
  switch (type) {
  case ti::XDG_SHELL_VIEW: {
    auto *v = dynamic_cast<ti::xdg_view *>(this);
    wlr_xdg_surface_get_geometry(v->xdg_surface, &geo_box);
    break;
  }
  case ti::XWAYLAND_VIEW: {
    auto *v = dynamic_cast<ti::xwayland_view *>(this);
    geo_box.width = v->xwayland_surface->width;
    geo_box.height = v->xwayland_surface->height;
    break;
  }
  }

  seat->view_x = box.x;
  seat->view_y = box.y;

  if (mode == ti::CURSOR_MOVE) {
    seat->grab_x = seat->cursor->x - box.x;
    seat->grab_y = seat->cursor->y - box.y;
  } else {
    seat->grab_x = seat->cursor->x;
    seat->grab_y = seat->cursor->y;
  }
  seat->grab_width = geo_box.width;
  seat->grab_height = geo_box.height;
  seat->resize_edges = edges;
}

void ti::view::damage_partial() {
  ti::output *output;
  wl_list_for_each(output, &desktop->outputs, link) {
    output->damage_partial_view(this);
  }
}

void ti::view::damage_whole() {
  ti::output *output;
  wl_list_for_each(output, &desktop->outputs, link) {
    output->damage_whole_view(this);
  }
}

void ti::view::render(ti::output *output, ti::render_data *data) {
  // don't render unmapped views
  if (!this->mapped) {
    return;
  }

  this->render_decorations(output, data);
  output->view_for_each_surface(this, render_surface_iterator, data);
}

bool ti::view::at(double lx, double ly, struct wlr_surface **surface,
                  double *sx, double *sy) {
  double view_sx = lx - this->box.x;
  double view_sy = ly - this->box.y;

  double _sx, _sy;
  struct wlr_surface *_surface = nullptr;

  switch (this->type) {
  case ti::XDG_SHELL_VIEW: {
    auto *v = dynamic_cast<ti::xdg_view *>(this);
    // auto *state = &v->xdg_surface->surface->current;
    _surface = wlr_xdg_surface_surface_at(v->xdg_surface, view_sx, view_sy,
                                          &_sx, &_sy);
    break;
  }
  case ti::XWAYLAND_VIEW: {
    auto *v = dynamic_cast<ti::xwayland_view *>(this);
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
