#include <cmath>

extern "C" {
#include <wlr/util/log.h>
#define static
#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_matrix.h>
#undef static
}

#include "output.hpp"
#include "render.hpp"
#include "server.hpp"
#include "xdg_shell.hpp"

void scissor_output(struct wlr_output *wlr_output, pixman_box32_t *rect) {
  struct wlr_renderer *renderer = wlr_backend_get_renderer(wlr_output->backend);
  // assert(renderer);

  struct wlr_box box = {
      .x = rect->x1,
      .y = rect->y1,
      .width = rect->x2 - rect->x1,
      .height = rect->y2 - rect->y1,
  };

  int ow, oh;
  wlr_output_transformed_resolution(wlr_output, &ow, &oh);

  enum wl_output_transform transform =
      wlr_output_transform_invert(wlr_output->transform);
  wlr_box_transform(&box, &box, transform, ow, oh);

  wlr_renderer_scissor(renderer, &box);
}

void ti::view::render_decorations(ti::output *output, ti::render_data *data) {
  float decoration_color[4] = {0.1, 0.1, 0.1, alpha};
  pixman_box32_t *rects;
  if (!decorated || surface == NULL) {
    return;
  }

  struct wlr_renderer *renderer =
      wlr_backend_get_renderer(output->wlr_output->backend);

  struct wlr_box box;
  output->get_decoration_box(*this, box);

  struct wlr_box rotated;
  wlr_box_rotated_bounds(&rotated, &box, rotation);

  pixman_region32_t damage;
  pixman_region32_init(&damage);
  pixman_region32_union_rect(&damage, &damage, rotated.x, rotated.y,
                             rotated.width, rotated.height);
  pixman_region32_intersect(&damage, &damage, data->damage);
  bool damaged = pixman_region32_not_empty(&damage);
  if (!damaged) {
    goto buffer_damage_finish;
  }

  float matrix[9];
  wlr_matrix_project_box(matrix, &box, WL_OUTPUT_TRANSFORM_NORMAL, rotation,
                         output->wlr_output->transform_matrix);

  int nrects;
  rects = pixman_region32_rectangles(&damage, &nrects);
  for (int i = 0; i < nrects; ++i) {
    scissor_output(output->wlr_output, &rects[i]);
    wlr_render_quad_with_matrix(renderer, decoration_color, matrix);
  }

buffer_damage_finish:
  pixman_region32_fini(&damage);
}

static void render_texture(struct wlr_output *wlr_output,
                           pixman_region32_t *output_damage,
                           struct wlr_texture *texture,
                           const struct wlr_box *box, const float matrix[9],
                           float rotation, float alpha) {
  pixman_box32_t *rects;
  struct wlr_renderer *renderer = wlr_backend_get_renderer(wlr_output->backend);

  struct wlr_box rotated;
  wlr_box_rotated_bounds(&rotated, box, rotation);

  pixman_region32_t damage;
  pixman_region32_init(&damage);
  pixman_region32_union_rect(&damage, &damage, rotated.x, rotated.y,
                             rotated.width, rotated.height);
  pixman_region32_intersect(&damage, &damage, output_damage);
  bool damaged = pixman_region32_not_empty(&damage);
  if (!damaged) {
    goto buffer_damage_finish;
  }

  int nrects;
  rects = pixman_region32_rectangles(&damage, &nrects);
  for (int i = 0; i < nrects; ++i) {
    scissor_output(wlr_output, &rects[i]);
    wlr_render_texture_with_matrix(renderer, texture, matrix, alpha);
  }

buffer_damage_finish:
  pixman_region32_fini(&damage);
}

void render_surface_iterator(ti::output *output, struct wlr_surface *surface,
                             struct wlr_box *_box, float rotation,
                             void *_data) {
  ti::render_data *data = (ti::render_data *)_data;
  struct wlr_output *wlr_output = output->wlr_output;
  pixman_region32_t *output_damage = data->damage;
  float alpha = data->alpha;

  /* We first obtain a wlr_texture, which is a GPU resource. wlroots
   * automatically handles negotiating these with the client. The underlying
   * resource could be an opaque handle passed from the client, or the client
   * could have sent a pixel buffer which we copied to the GPU, or a few other
   * means. You don't have to worry about this, wlroots takes care of it. */
  struct wlr_texture *texture = wlr_surface_get_texture(surface);
  if (!texture) {
    return;
  }

  struct wlr_box box = {_box->x, _box->y, _box->width, _box->height};
  scale_box(&box, wlr_output->scale);

  /*
   * Those familiar with OpenGL are also familiar with the role of matricies
   * in graphics programming. We need to prepare a matrix to render the view
   * with. wlr_matrix_project_box is a helper which takes a box with a desired
   * x, y coordinates, width and height, and an output geometry, then
   * prepares an orthographic projection and multiplies the necessary
   * transforms to produce a model-view-projection matrix.
   *
   * Naturally you can do this any way you like, for example to make a 3D
   * compositor.
   */
  float matrix[9];
  enum wl_output_transform transform =
      wlr_output_transform_invert(surface->current.transform);
  wlr_matrix_project_box(matrix, &box, transform, rotation,
                         wlr_output->transform_matrix);

  render_texture(wlr_output, output_damage, texture, &box, matrix, 0.0, alpha);

  wlr_presentation_surface_sampled_on_output(output->server->presentation,
                                             surface, wlr_output);
}
