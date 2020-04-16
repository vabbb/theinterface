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

void render_decorations(ti::output *output, ti::view *view,
                        ti::render_data *data) {
  float color[4] = {0.2, 0.2, 0.2, 1.0}; // view->alpha};
  pixman_box32_t *rects;
  if (!view->decorated || view->surface == NULL) {
    return;
  }

  struct wlr_renderer *renderer =
      wlr_backend_get_renderer(output->wlr_output->backend);

  struct wlr_box box;
  output->get_decoration_box(*view, box);

  struct wlr_box rotated;
  wlr_box_rotated_bounds(&rotated, &box, view->rotation);

  pixman_region32_t damage;
  pixman_region32_init(&damage);
  pixman_region32_union_rect(&damage, &damage, rotated.x, rotated.y,
                             rotated.width, rotated.height);
  pixman_region32_intersect(&damage, &damage, &damage); // data->damage);
  bool damaged = pixman_region32_not_empty(&damage);
  if (!damaged) {
    goto buffer_damage_finish;
  }

  float matrix[9];
  wlr_matrix_project_box(matrix, &box, WL_OUTPUT_TRANSFORM_NORMAL,
                         view->rotation, output->wlr_output->transform_matrix);

  int nrects;
  rects = pixman_region32_rectangles(&damage, &nrects);
  for (int i = 0; i < nrects; ++i) {
    scissor_output(output->wlr_output, &rects[i]);
    wlr_render_quad_with_matrix(renderer, color, matrix);
  }

buffer_damage_finish:
  pixman_region32_fini(&damage);
}

/** This function is called for every surface that needs to be rendered.
 * Assumes surface is not NULL
 */
void render_surface(struct wlr_surface *surface, int sx, int sy, void *data) {
  ti::render_data *rdata = reinterpret_cast<ti::render_data *>(data);
  ti::view *view = rdata->view;
  struct wlr_output *output = rdata->output;

  /* We first obtain a wlr_texture, which is a GPU resource. wlroots
   * automatically handles negotiating these with the client. The underlying
   * resource could be an opaque handle passed from the client, or the client
   * could have sent a pixel buffer which we copied to the GPU, or a few other
   * means. You don't have to worry about this, wlroots takes care of it. */
  struct wlr_texture *texture = wlr_surface_get_texture(surface);
  if (texture == NULL) {
    return;
  }

  /* The view has a position in layout coordinates. If you have two displays,
   * one next to the other, both 1080p, a view on the rightmost display might
   * have layout coordinates of 2000,100. We need to translate that to
   * output-local coordinates, or (2000 - 1920). */
  double ox = 0, oy = 0;
  wlr_output_layout_output_coords(view->server->output_layout, output, &ox,
                                  &oy);
  ox += view->x + sx, oy += view->y + sy;

  /* We also have to apply the scale factor for HiDPI outputs. This is only
   * part of the puzzle, TinyWL does not fully support HiDPI. */
  struct wlr_box box = {
      .x = (int)(ox * output->scale),
      .y = (int)(oy * output->scale),
      .width = (int)(surface->current.width * output->scale),
      .height = (int)(surface->current.height * output->scale),
  };

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
  wlr_matrix_project_box(matrix, &box, transform, 0, output->transform_matrix);

  /* This takes our matrix, the texture, and an alpha, and performs the actual
   * rendering on the GPU. */
  wlr_render_texture_with_matrix(rdata->renderer, texture, matrix, 1);

  /* This lets the client know that we've displayed that frame and it can
   * prepare another one now if it likes. */
  wlr_surface_send_frame_done(surface, rdata->when);
}

ti::render_data::render_data() {}