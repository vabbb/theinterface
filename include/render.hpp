#ifndef TI_RENDER_HPP
#define TI_RENDER_HPP

#include "view.hpp"

/* Used to move all of the data necessary to render a surface from the top-level
 * frame handler to the per-surface render function. */
struct render_data {
  struct wlr_output *output;
  struct wlr_renderer *renderer;
  class ti::view *view;
  struct timespec *when;
};

void render_surface(struct wlr_surface *surface, int sx, int sy, void *data);

#endif