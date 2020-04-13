#include <cmath>
#include <ctime>

extern "C" {
#include <wlr/util/log.h>
#define static
#include <wlr/render/wlr_renderer.h>
#undef static
}

#include "output.hpp"
#include "render.hpp"
#include "xdg_shell.hpp"

#include "server.hpp"

static void output_frame(struct wl_listener *listener, void *data) {
  /* This function is called every time an output is ready to display a frame,
   * generally at the output's refresh rate (e.g. 60Hz). */
  ti::output *output = wl_container_of(listener, output, frame);
  struct wlr_renderer *renderer = output->server->renderer;

  struct timespec now;
  clock_gettime(CLOCK_MONOTONIC, &now);

  /* wlr_output_attach_render makes the OpenGL context current. */
  if (!wlr_output_attach_render(output->wlr_output, NULL)) {
    return;
  }
  /* The "effective" resolution can change if you rotate your outputs. */
  int width, height;
  wlr_output_effective_resolution(output->wlr_output, &width, &height);
  /* Begin the renderer (calls glViewport and some other GL sanity checks) */
  wlr_renderer_begin(renderer, width, height);

  float color[4] = {0.3, 0.3, 0.3, 1.0};
  wlr_renderer_clear(renderer, color);

  /* Each subsequent window we render is rendered on top of the last. Because
   * our view list is ordered front-to-back, we iterate over it backwards. */
  ti::view *view;
  wl_list_for_each_reverse(view, &output->server->wem_views, wem_link) {
    if (!view->mapped) {
      /* An unmapped view should not be rendered. */
      continue;
    }
    struct render_data rdata = {
        .output = output->wlr_output,
        .renderer = renderer,
        .view = view,
        .when = &now,
    };
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

  /* Conclude rendering and swap the buffers, showing the final frame
   * on-screen. */
  wlr_renderer_end(renderer);
  wlr_output_commit(output->wlr_output);
}

void handle_new_output(struct wl_listener *listener, void *data) {
  /* This event is rasied by the backend when a new output (aka a display or
   * monitor) becomes available. */
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
  /* Sets up a listener for the frame notify event. */
  output->frame.notify = output_frame;
  wl_signal_add(&wlr_output->events.frame, &output->frame);
  wl_list_insert(&server->outputs, &output->link);

  /* Adds this to the output layout. The add_auto function arranges outputs
   * from left-to-right in the order they appear. A more sophisticated
   * compositor would let the user configure the arrangement of outputs in the
   * layout. */
  wlr_output_layout_add_auto(server->output_layout, wlr_output);

  /* Creating the global adds a wl_output global to the display, which Wayland
   * clients can see to find out information about the output (such as
   * DPI, scale factor, manufacturer, etc). */
  wlr_output_create_global(wlr_output);
}
