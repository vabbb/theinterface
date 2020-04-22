extern "C" {
#include <wlr/backend.h>
#include <wlr/types/wlr_foreign_toplevel_management_v1.h>
#include <wlr/util/log.h>
#define static
#include <wlr/render/wlr_renderer.h>
#undef static
}

#include "seat.hpp"

#include "server.hpp"

ti::server::server() {
  /* The Wayland display is managed by libwayland. It handles accepting
   * clients from the Unix socket, manging Wayland globals, and so on. */
  this->display = wl_display_create();
  if (!this->display) {
    wlr_log_errno(WLR_ERROR, "Unable to create a wayland display");
  }

  /* The backend is a wlroots feature which abstracts the underlying input and
   * output hardware. The autocreate option will choose the most suitable
   * backend based on the current environment, such as opening an X11 window
   * if an X11 server is running. The NULL argument here optionally allows you
   * to pass in a custom renderer if wlr_renderer doesn't meet your needs. The
   * backend uses the renderer, for example, to fall back to software cursors
   * if the backend does not support hardware cursors (some older GPUs
   * don't). */
  this->backend = wlr_backend_autocreate(this->display, NULL);
  if (!this->backend) {
    wlr_log_errno(WLR_ERROR, "Unable to start backend");
  }

  /* If we don't provide a renderer, autocreate makes a GLES2 renderer for us.
   * The renderer is responsible for defining the various pixel formats it
   * supports for shared memory, this configures that for clients. */
  this->renderer = wlr_backend_get_renderer(this->backend);
  if (!this->renderer) {
    wlr_log_errno(WLR_ERROR, "Unable to find a renderer");
  }
  this->data_device_manager = wlr_data_device_manager_create(this->display);
  wlr_renderer_init_wl_display(this->renderer, this->display);

  this->desktop = new ti::desktop(this);

  /* Add a Unix socket to the Wayland display. */
  const char *socket = wl_display_add_socket_auto(this->display);
  if (!socket) {
    wlr_backend_destroy(this->backend);
    exit(EXIT_FAILURE);
  }

  /* Set the WAYLAND_DISPLAY environment variable to our socket and run the
   * startup command if requested. */
  wlr_log(WLR_INFO, "Running TheInterface on WAYLAND_DISPLAY=%s", socket);
  setenv("WAYLAND_DISPLAY", socket, true);

  /* Start the backend. This will enumerate outputs and inputs, become the DRM
   * master, etc */
  if (!wlr_backend_start(this->backend)) {
    wlr_backend_destroy(this->backend);
    wl_display_destroy(this->display);
    exit(EXIT_FAILURE);
  }
}

/// automatically ran when the program is about to exit
ti::server::~server() {
  wlr_log(WLR_INFO, "Deallocating server resources");
  delete desktop;
  /* Once wl_display_run returns, we shut down the server. */
  wl_display_destroy_clients(display);
  wl_display_destroy(display);
}
