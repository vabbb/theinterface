#ifndef TI_RENDER_HPP
#define TI_RENDER_HPP

#include "view.hpp"

namespace ti {
/* Used to move all of the data necessary to render a surface from the top-level
 * frame handler to the per-surface render function. */
class render_data {
public:
  struct wlr_output *output;
  struct wlr_renderer *renderer;
  class ti::view *view;
  struct timespec *when;
  pixman_region32_t *damage;
  float alpha;

  render_data();
};
} // namespace ti

void render_surface(struct wlr_surface *surface, int sx, int sy, void *data);
void scissor_output(struct wlr_output *wlr_output, pixman_box32_t *rect);
void render_decorations(ti::output *output, ti::view *view,
                        ti::render_data *rdata);

#endif