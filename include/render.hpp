#ifndef TI_RENDER_HPP
#define TI_RENDER_HPP

struct wlr_output;

namespace ti {

class view;

/* Used to move all of the data necessary to render a surface from the top-level
 * frame handler to the per-surface render function. */
struct render_data {
  pixman_region32_t *damage;
  float alpha;
};
} // namespace ti

void render_surface(struct wlr_surface *surface, int sx, int sy, void *data);
void scissor_output(struct wlr_output *wlr_output, pixman_box32_t *rect);
void render_surface_iterator(ti::output *output, struct wlr_surface *surface,
                             struct wlr_box *_box, float rotation, void *_data);

#endif