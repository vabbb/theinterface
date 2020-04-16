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

/* This function is called every time an output is ready to display a frame,
 * generally at the output's refresh rate (e.g. 60Hz). */
static void output_frame(struct wl_listener *listener, void *data) {
  float color[4] = {0.3, 0.3, 0.3, 1.0};
  enum wl_output_transform transform = WL_OUTPUT_TRANSFORM_NORMAL;
  ti::output *output = wl_container_of(listener, output, frame);
  struct wlr_renderer *renderer = output->server->renderer;

  struct timespec now;
  clock_gettime(CLOCK_MONOTONIC, &now);
  wlr_output_damage_add_whole(output->damage);
  bool needs_frame;
  pixman_region32_t buffer_damage;
  pixman_region32_init(&buffer_damage);
  /* wlr_output_attach_render makes the OpenGL context current. */
  if (!wlr_output_damage_attach_render(output->damage, &needs_frame,
                                       &buffer_damage)) {
    return;
  }

  ti::render_data rdata;
  rdata.damage = &buffer_damage;
  rdata.alpha = 1.0;

  int nrects;
  /// TODO: this needs to be moved after the goto
  pixman_box32_t *rects = pixman_region32_rectangles(&buffer_damage, &nrects);

  if (!needs_frame) {
    // Output doesn't need swap and isn't damaged, skip rendering completely
    goto buffer_damage_finish;
  }

  /* The "effective" resolution can change if you rotate your outputs. */
  int width, height;
  wlr_output_effective_resolution(output->wlr_output, &width, &height);
  /* Begin the renderer (calls glViewport and some other GL sanity checks) */
  wlr_renderer_begin(renderer, width, height);

  for (int i = 0; i < nrects; ++i) {
    scissor_output(output->wlr_output, &rects[i]);
    wlr_renderer_clear(renderer, color);
  }

  /* Each subsequent window we render is rendered on top of the last. Because
   * our view list is ordered front-to-back, we iterate over it backwards. */
  ti::view *view;
  wl_list_for_each_reverse(view, &output->server->wem_views, wem_link) {
    if (!view->mapped) {
      /* An unmapped view should not be rendered. */
      continue;
    }
    rdata.output = output->wlr_output;
    rdata.renderer = renderer;
    rdata.view = view;
    rdata.when = &now;

    switch (view->type) {
    case ti::XDG_SHELL_VIEW: {
      /* This calls our render_surface function for each surface among the
       * xdg_surface's toplevel and popups. */
      ti::xdg_view *v = dynamic_cast<ti::xdg_view *>(view);
      wlr_xdg_surface_for_each_surface(v->xdg_surface, render_surface, &rdata);
      break;
    }
    case ti::XWAYLAND_VIEW: {
      // rendering the surface directly
      // render_surface(view->xwayland_surface->surface, 0, 0, &rdata);
      ti::xwayland_view *v = dynamic_cast<ti::xwayland_view *>(view);

      render_decorations(output, v, &rdata);
      wlr_surface_for_each_surface(v->surface, render_surface, &rdata);
      break;
    }
    }
  }

  /* Hardware cursors are rendered by the GPU on a separate plane, and can be
   * moved around without re-rendering what's beneath them - which is more
   * efficient. However, not all hardware supports hardware cursors. For this
   * reason, wlroots provides a software fallback, which we ask it to render
   * here. wlr_cursor handles configuring hardware vs software cursors for you,
   * and this function is a no-op when hardware cursors are in use. */
  wlr_output_render_software_cursors(output->wlr_output, NULL);
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

  fp3s_counter(now);
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
}

void ti::output::get_decoration_box(ti::view &view, struct wlr_box &box) {
  struct wlr_box deco_box;
  view.get_deco_box(deco_box);
  double sx = deco_box.x - view.box.x;
  double sy = deco_box.y - view.box.y;
  // rotate_child_position(&sx, &sy, deco_box.width, deco_box.height,
  //                       view->wlr_surface->current.width,
  //                       view->wlr_surface->current.height, view->rotation);
  double x = sx + view.box.x;
  double y = sy + view.box.y;

  wlr_output_layout_output_coords(server->output_layout, wlr_output, &x, &y);

  box.x = x * wlr_output->scale;
  box.y = y * wlr_output->scale;
  box.width = deco_box.width * wlr_output->scale;
  box.height = deco_box.height * wlr_output->scale;
}