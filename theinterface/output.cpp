#include <cmath>
#include <ctime>

extern "C" {
#include <wlr/types/wlr_output_damage.h>
#include <wlr/util/log.h>
#include <wlr/util/region.h>
#define static
#include <wlr/render/wlr_renderer.h>
#undef static
}

#include "output.hpp"
#include "render.hpp"
#include "util.hpp"
#include "view.hpp"
#include "xdg_shell.hpp"

#include "server.hpp"

/**
 * Rotate a child's position relative to a parent. The parent size is (pw, ph),
 * the child position is (*sx, *sy) and its size is (sw, sh).
 */
void rotate_child_position(double *sx, double *sy, double sw, double sh,
                           double pw, double ph, float rotation) {
  if (rotation == 0.0) {
    return;
  }

  // Coordinates relative to the center of the subsurface
  double cx = *sx - pw / 2 + sw / 2, cy = *sy - ph / 2 + sh / 2;
  // Rotated coordinates
  double rx = cos(rotation) * cx - sin(rotation) * cy,
         ry = cos(rotation) * cy + sin(rotation) * cx;
  *sx = rx + pw / 2 - sw / 2;
  *sy = ry + ph / 2 - sh / 2;
}

static bool get_surface_box(struct surface_iterator_data *data,
                            struct wlr_surface *surface, int sx, int sy,
                            struct wlr_box *surface_box) {
  ti::output *output = data->output;

  if (!wlr_surface_has_buffer(surface)) {
    return false;
  }

  int sw = surface->current.width;
  int sh = surface->current.height;

  double _sx = (double)(sx + surface->sx);
  double _sy = (double)(sy + surface->sy);
  rotate_child_position(&_sx, &_sy, sw, sh, data->width, data->height,
                        data->rotation);

  struct wlr_box box = {
      .x = (int)(data->ox + _sx),
      .y = (int)(data->oy + _sy),
      .width = sw,
      .height = sh,
  };
  if (surface_box != NULL) {
    *surface_box = box;
  }

  struct wlr_box rotated_box;
  wlr_box_rotated_bounds(&rotated_box, &box, data->rotation);

  // "= {}" initializes every member of the struct with the "default"
  // constructor, which will set every variable to "0 of the appropriate type".
  // We need this because the C version assumes .x and .y to be 0, whereas C++
  // can't make any assumptions
  struct wlr_box output_box = {};
  wlr_output_effective_resolution(output->wlr_output, &output_box.width,
                                  &output_box.height);

  struct wlr_box intersection;
  return wlr_box_intersection(&intersection, &output_box, &rotated_box);
}

static void output_for_each_surface_iterator(struct wlr_surface *surface,
                                             int sx, int sy, void *_data) {
  struct surface_iterator_data *data = (struct surface_iterator_data *)_data;

  struct wlr_box box;
  bool intersects = get_surface_box(data, surface, sx, sy, &box);
  if (!intersects) {
    return;
  }

  data->user_iterator(data->output, surface, &box, data->rotation,
                      data->user_data);
}

void ti::output::view_for_each_surface(ti::view *view,
                                       ti_surface_iterator_func_t iterator,
                                       void *user_data) {
  struct wlr_box *output_box =
      wlr_output_layout_get_box(server->output_layout, wlr_output);
  if (!output_box) {
    return;
  }

  struct surface_iterator_data data = {
      .user_iterator = iterator,
      .user_data = user_data,
      .output = this,
      .ox = (double)(view->box.x - output_box->x),
      .oy = (double)(view->box.y - output_box->y),
      .width = view->box.width,
      .height = view->box.height,
      .rotation = view->rotation,
  };

  view->for_each_surface(output_for_each_surface_iterator, &data);
}

inline int scale_length(int length, int offset, float scale) {
  return round((offset + length) * scale) - round(offset * scale);
}

void scale_box(struct wlr_box *box, float scale) {
  box->width = scale_length(box->width, box->x, scale);
  box->height = scale_length(box->height, box->y, scale);
  box->x = round(box->x * scale);
  box->y = round(box->y * scale);
}

static void damage_surface_iterator(ti::output *output,
                                    struct wlr_surface *surface,
                                    struct wlr_box *_box, float rotation,
                                    void *data) {
  bool *whole = (bool *)data;

  struct wlr_box box = *_box;
  scale_box(&box, output->wlr_output->scale);

  int center_x = box.x + box.width / 2;
  int center_y = box.y + box.height / 2;

  if (pixman_region32_not_empty(&surface->buffer_damage)) {
    pixman_region32_t damage;
    pixman_region32_init(&damage);
    wlr_surface_get_effective_damage(surface, &damage);
    wlr_region_scale(&damage, &damage, output->wlr_output->scale);
    if (std::ceil(output->wlr_output->scale) > surface->current.scale) {
      // When scaling up a surface, it'll become blurry so we need to
      // expand the damage region
      wlr_region_expand(&damage, &damage,
                        std::ceil(output->wlr_output->scale) -
                            surface->current.scale);
    }
    pixman_region32_translate(&damage, box.x, box.y);
    wlr_region_rotated_bounds(&damage, &damage, rotation, center_x, center_y);
    wlr_output_damage_add(output->damage, &damage);
    pixman_region32_fini(&damage);
  }

  if (*whole) {
    wlr_box_rotated_bounds(&box, &box, rotation);
    wlr_output_damage_add_box(output->damage, &box);
  }

  wlr_output_schedule_frame(output->wlr_output);
}

static void damage_whole_decoration(ti::view *view, ti::output *output) {
  if (!view->decorated || view->surface == NULL) {
    return;
  }

  struct wlr_box box;
  output->get_decoration_box(*view, box);

  wlr_box_rotated_bounds(&box, &box, view->rotation);

  wlr_output_damage_add_box(output->damage, &box);
}

static void surface_send_frame_done_iterator(ti::output *output,
                                             struct wlr_surface *surface,
                                             struct wlr_box *box,
                                             float rotation, void *data) {
  struct timespec *when = reinterpret_cast<struct timespec *>(data);
  wlr_surface_send_frame_done(surface, when);
}

/* This function is called every time an output is ready to display a frame,
 * generally at the output's refresh rate (e.g. 60Hz). */
static void output_frame(struct wl_listener *listener, void *data) {
  int nrects;
  pixman_box32_t *rects = nullptr;
  int width, height;
  const float color[] = {0.4, 0.4, 0.4, 1.0};

  enum wl_output_transform transform;
  ti::output *output = wl_container_of(listener, output, frame);
  struct wlr_renderer *renderer = output->server->renderer;

  struct timespec now;
  clock_gettime(CLOCK_MONOTONIC, &now);

  /// use this for debugging rendering functions in case nothing else works
  // wlr_output_damage_add_whole(output->damage);

  bool needs_frame;
  pixman_region32_t buffer_damage;
  pixman_region32_init(&buffer_damage);
  // fps_counter(now);
  /* wlr_output_attach_render makes the OpenGL context current. */
  if (!wlr_output_damage_attach_render(output->damage, &needs_frame,
                                       &buffer_damage)) {
    return;
  }

  ti::render_data rdata = {
      .damage = &buffer_damage,
      .alpha = 1.0,
  };

  if (!needs_frame) {
    // Output doesn't need swap and isn't damaged, skip rendering completely
    goto buffer_damage_finish;
  }

  // /* The "effective" resolution can change if you rotate your outputs. */
  // wlr_output_effective_resolution(output->wlr_output, &width, &height);
  /* Begin the renderer (calls glViewport and some other GL sanity checks) */
  wlr_renderer_begin(renderer, output->wlr_output->width,
                     output->wlr_output->height);

  if (!pixman_region32_not_empty(&buffer_damage)) {
    // Output isn't damaged but needs buffer swap
    goto renderer_end;
  }

  rects = pixman_region32_rectangles(&buffer_damage, &nrects);
  for (int i = 0; i < nrects; ++i) {
    scissor_output(output->wlr_output, &rects[i]);
    wlr_renderer_clear(renderer, color);
  }

  /* Each subsequent window we render is rendered on top of the last. Because
   * our view list is ordered front-to-back, we iterate over it backwards. */
  ti::view *view;
  wl_list_for_each_reverse(view, &output->server->wem_views, wem_link) {
    view->render(output, &rdata);
  }

renderer_end:
  /* Hardware cursors are rendered by the GPU on a separate plane, and can be
   * moved around without re-rendering what's beneath them - which is more
   * efficient. However, not all hardware supports hardware cursors. For this
   * reason, wlroots provides a software fallback, which we ask it to render
   * here. wlr_cursor handles configuring hardware vs software cursors for you,
   * and this function is a no-op when hardware cursors are in use. */
  wlr_output_render_software_cursors(output->wlr_output, &buffer_damage);
  wlr_renderer_scissor(renderer, NULL);

  /* Conclude rendering and swap the buffers, showing the final frame
   * on-screen. */
  wlr_renderer_end(renderer);

  wlr_output_transformed_resolution(output->wlr_output, &width, &height);

  pixman_region32_t frame_damage;
  pixman_region32_init(&frame_damage);

  transform = wlr_output_transform_invert(output->wlr_output->transform);
  wlr_region_transform(&frame_damage, &output->damage->current, transform,
                       width, height);

  wlr_output_set_damage(output->wlr_output, &frame_damage);
  pixman_region32_fini(&frame_damage);

  wlr_output_commit(output->wlr_output);

buffer_damage_finish:
  pixman_region32_fini(&buffer_damage);

  // Send frame done events to all surfaces
  output->for_each_surface(surface_send_frame_done_iterator, &now);
}

void handle_new_output(struct wl_listener *listener, void *data) {
  ti::server *server = wl_container_of(listener, server, new_output);
  struct wlr_output *wlr_output = reinterpret_cast<struct wlr_output *>(data);

  /* Some backends don't have modes. DRM+KMS does, and we need to set a mode
   * before we can use the output. The mode is a tuple of (width, height,
   * refresh rate), and each monitor supports only a specific set of modes. We
   * just pick the monitor's preferred mode, a more sophisticated compositor
   * would let the user configure it. */
  if (!wl_list_empty(&wlr_output->modes)) {

    /// EVIL: force modeset
    // struct wlr_output_mode *m;
    // wl_list_for_each(m, &wlr_output->modes, link) {
    //   if (m->width == 1440 && m->height == 900) {
    //     break;
    //   }
    // };
    struct wlr_output_mode *mode = wlr_output_preferred_mode(wlr_output); // m;

    wlr_output_set_mode(wlr_output, mode);
    wlr_output_enable(wlr_output, true);
    if (!wlr_output_commit(wlr_output)) {
      return;
    }
  }

  /* Allocates and configures our state for this output */
  ti::output *output = new ti::output;
  output->wlr_output = wlr_output;
  output->server = server;
  output->damage = wlr_output_damage_create(wlr_output);

  /* Sets up a listener for the frame notify event. */
  output->frame.notify = output_frame;
  wl_signal_add(&wlr_output->events.frame, &output->frame);
  wl_list_insert(&server->outputs, &output->link);

  /* Adds this to the output layout. The add_auto function arranges outputs
   * from left-to-right in the order they appear. A more sophisticated
   * compositor would let the user configure the arrangement of outputs in the
   * layout.
   * The output layout utility automatically adds a wl_output global to the
   * display, which Wayland clients can see to find out information about the
   * output (such as DPI, scale factor, manufacturer, etc).
   */
  wlr_output_layout_add_auto(server->output_layout, wlr_output);

  wlr_output_damage_add_whole(output->damage);
}

void ti::output::get_decoration_box(ti::view &view, struct wlr_box &box) {
  struct wlr_box deco_box;
  view.get_deco_box(deco_box);
  double sx = deco_box.x - view.box.x;
  double sy = deco_box.y - view.box.y;
  rotate_child_position(&sx, &sy, deco_box.width, deco_box.height,
                        view.surface->current.width,
                        view.surface->current.height, view.rotation);
  double x = sx + view.box.x;
  double y = sy + view.box.y;

  wlr_output_layout_output_coords(server->output_layout, wlr_output, &x, &y);

  box.x = x * wlr_output->scale;
  box.y = y * wlr_output->scale;
  box.width = deco_box.width * wlr_output->scale;
  box.height = deco_box.height * wlr_output->scale;
}

void ti::output::damage_partial_view(ti::view *view) {
  /// TODO: incomplete
  bool whole = false;
  this->view_for_each_surface(view, damage_surface_iterator, &whole);
}

void ti::output::damage_whole_view(ti::view *view) {

  damage_whole_decoration(view, this);

  bool whole = true;
  this->view_for_each_surface(view, damage_surface_iterator, &whole);
}

void ti::output::for_each_surface(ti_surface_iterator_func_t iterator,
                                  void *user_data) {
  /// TODO: re-add fullscreen, drag icons, layers
  ti::view *view;
  wl_list_for_each_reverse(view, &server->wem_views, wem_link) {
    this->view_for_each_surface(view, iterator, user_data);
  }
}
