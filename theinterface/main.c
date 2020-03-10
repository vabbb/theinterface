#define _POSIX_C_SOURCE 200809L

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <wayland-server.h>
#include <wlr/backend.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_output.h>

struct ti_server {
  struct wl_display *wl_display;
  struct wl_event_loop *wl_event_loop;
  struct wlr_backend *backend;
  struct wl_listener new_output;
  struct wl_list outputs; // ti_output::link
};

struct ti_output {
  struct wlr_output *wlr_output;
  struct ti_server *server;
  struct timespec last_frame;
  float color[4];
  int dec;
  struct wl_listener destroy;
  struct wl_listener frame;
  struct wl_list link;
};

static void output_frame_notify(struct wl_listener *listener, void *data) {
  struct ti_output *output = wl_container_of(listener, output, frame);
  struct wlr_output *wlr_output = data;
  struct wlr_renderer *renderer = wlr_backend_get_renderer(wlr_output->backend);

  struct timespec now;
  clock_gettime(CLOCK_MONOTONIC, &now);

  // Calculate a color, just for pretty demo purposes
  long ms = (now.tv_sec - output->last_frame.tv_sec) * 1000 +
            (now.tv_nsec - output->last_frame.tv_nsec) / 1000000;
  int inc = (output->dec + 1) % 3;
  output->color[inc] += ms / 2000.0f;
  output->color[output->dec] -= ms / 2000.0f;
  if (output->color[output->dec] < 0.0f) {
    output->color[inc] = 1.0f;
    output->color[output->dec] = 0.0f;
    output->dec = inc;
  }
  // End pretty color calculation

  wlr_output_attach_render(wlr_output, NULL);

  // Type signature: void wlr_renderer_begin(struct wlr_renderer *r, int
  // width, int height);
  wlr_renderer_begin(renderer, wlr_output->width, wlr_output->height);

  wlr_renderer_clear(
      renderer, (const float *)(&output->color)); // compiler complains about
                                                  // this second argument

  wlr_output_commit(wlr_output);
  wlr_renderer_end(renderer);

  output->last_frame = now;
}

static void output_destroy_notify(struct wl_listener *listener, void *data) {
  struct ti_output *output = wl_container_of(listener, output, destroy);
  wl_list_remove(&output->link);
  wl_list_remove(&output->destroy.link);
  wl_list_remove(&output->frame.link);
  free(output);
}

static void new_output_notify(struct wl_listener *listener, void *data) {
  struct ti_server *server =
      wl_container_of(listener, server, new_output);
  struct wlr_output *wlr_output = data;

  if (wl_list_length(&wlr_output->modes) > 0) {
    struct wlr_output_mode *mode =
        wl_container_of((&wlr_output->modes)->prev, mode, link);
    wlr_output_set_mode(wlr_output, mode);
  }

  struct ti_output *output = calloc(1, sizeof(struct ti_output));
  clock_gettime(CLOCK_MONOTONIC, &output->last_frame);
  output->server = server;
  output->wlr_output = wlr_output;
  output->color[0] = 1.0;
  output->color[3] = 1.0;
  wl_list_insert(&server->outputs, &output->link);

  output->destroy.notify = output_destroy_notify;
  wl_signal_add(&wlr_output->events.destroy, &output->destroy);
  output->frame.notify = output_frame_notify;
  wl_signal_add(&wlr_output->events.frame, &output->frame);
}

int main(int argc, char **argv) {
  struct ti_server server;

  server.wl_display = wl_display_create();
  assert(server.wl_display);
  server.wl_event_loop = wl_display_get_event_loop(server.wl_display);
  assert(server.wl_event_loop);

  // calling API
  server.backend = wlr_backend_autocreate(
      server.wl_display, NULL); // yields "root privelege" error

  assert(server.backend);

  wl_list_init(&server.outputs);

  server.new_output.notify = new_output_notify;
  wl_signal_add(&server.backend->events.new_output, &server.new_output);

  if (!wlr_backend_start(server.backend)) {
    fprintf(stderr, "Failed to start backend\n");
    wl_display_destroy(server.wl_display);
    return 1;
  }

  wl_display_run(server.wl_display);
  wl_display_destroy(server.wl_display);
  return 0;
}