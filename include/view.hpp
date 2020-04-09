#ifndef TI_VIEW_HPP
#define TI_VIEW_HPP

extern "C" {
#include <wayland-util.h>

#include <wlr/config.h>
#include <wlr/types/wlr_surface.h>
#include <wlr/types/wlr_xdg_shell.h>
#if WLR_HAS_WAYLAND
#include <wlr/xwayland.h>
#endif
}

#include "server.hpp"

enum ti_view_type {
  TI_XDG_SHELL_VIEW,
#if WLR_HAS_WAYLAND
  TI_XWAYLAND_VIEW,
#endif
};

struct ti_view {
  ti_server *server;
  struct wl_list link;     // server::views
  struct wl_list children; // ti_view_child::link
  struct wlr_surface *wlr_surface;

  int x, y;
  pid_t pid;
  bool mapped;

  enum ti_view_type type;
  const struct ti_view_impl *impl;

  struct wl_listener map;
  struct wl_listener unmap;
  struct wl_listener destroy;
  struct wl_listener request_move;
  struct wl_listener request_resize;
  struct wl_listener new_subsurface;
};

struct ti_view_impl {
  char *(*get_title)(struct ti_view *view);
  void (*get_geometry)(struct ti_view *view, int *width_out, int *height_out);
  bool (*is_primary)(struct ti_view *view);
  bool (*is_transient_for)(struct ti_view *child, struct ti_view *parent);
  void (*activate)(struct ti_view *view, bool activate);
  void (*maximize)(struct ti_view *view, int output_width, int output_height);
  void (*destroy)(struct ti_view *view);
  void (*for_each_surface)(struct ti_view *view,
                           wlr_surface_iterator_func_t iterator, void *data);
  void (*for_each_popup)(struct ti_view *view,
                         wlr_surface_iterator_func_t iterator, void *data);
  struct wlr_surface *(*wlr_surface_at)(struct ti_view *view, double sx,
                                        double sy, double *sub_x,
                                        double *sub_y);
};

#endif