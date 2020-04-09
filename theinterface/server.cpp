extern "C" {
#include <wlr/types/wlr_data_device.h>
#include <wlr/util/log.h>
}

#include "output.hpp"
#include "seat.hpp"
#include "server.hpp"
#include "xdg_shell.hpp"
#include "xwayland.hpp"

ti_server::ti_server() {
  /* The Wayland display is managed by libwayland. It handles accepting
   * clients from the Unix socket, manging Wayland globals, and so on. */
  wl_display = wl_display_create();
  /* The backend is a wlroots feature which abstracts the underlying input and
   * output hardware. The autocreate option will choose the most suitable
   * backend based on the current environment, such as opening an X11 window
   * if an X11 server is running. The NULL argument here optionally allows you
   * to pass in a custom renderer if wlr_renderer doesn't meet your needs. The
   * backend uses the renderer, for example, to fall back to software cursors
   * if the backend does not support hardware cursors (some older GPUs
   * don't). */
  backend = wlr_backend_autocreate(wl_display, NULL);

  /* If we don't provide a renderer, autocreate makes a GLES2 renderer for us.
   * The renderer is responsible for defining the various pixel formats it
   * supports for shared memory, this configures that for clients. */
  renderer = wlr_backend_get_renderer(backend);
  wlr_renderer_init_wl_display(renderer, wl_display);

  /* This creates some hands-off wlroots interfaces. The compositor is
   * necessary for clients to allocate surfaces and the data device manager
   * handles the clipboard. Each of these wlroots interfaces has room for you
   * to dig your fingers in and play with their behavior if you want. */
  compositor = wlr_compositor_create(wl_display, renderer);
  wlr_data_device_manager_create(wl_display);

  /* Creates an output layout, which a wlroots utility for working with an
   * arrangement of screens in a physical layout. */
  output_layout = wlr_output_layout_create();

  /* Configure a listener to be notified when new outputs are available on the
   * backend. */
  wl_list_init(&outputs);
  new_output.notify = server_new_output;
  wl_signal_add(&backend->events.new_output, &new_output);

  /* Set up our list of views and the xdg-shell. The xdg-shell is a Wayland
   * protocol which is used for application windows. For more detail on
   * shells, refer to my article:
   *
   * https://drewdevault.com/2018/07/29/Wayland-shells.html
   */
  wl_list_init(&views);
  xdg_shell = wlr_xdg_shell_create(wl_display);
  new_xdg_surface.notify = server_new_xdg_surface;
  wl_signal_add(&xdg_shell->events.new_surface, &new_xdg_surface);

  /*
   * Creates a cursor, which is a wlroots utility for tracking the cursor
   * image shown on screen.
   */
  cursor = wlr_cursor_create();
  wlr_cursor_attach_output_layout(cursor, output_layout);

  /* Creates an xcursor manager, another wlroots utility which loads up
   * Xcursor themes to source cursor images from and makes sure that cursor
   * images are available at all scale factors on the screen (necessary for
   * HiDPI support). We add a cursor theme at scale factor 1 to begin with. */
  cursor_mgr = wlr_xcursor_manager_create(nullptr, 24);
  wlr_xcursor_manager_load(cursor_mgr, 1);

  /*
   * wlr_cursor *only* displays an image on screen. It does not move around
   * when the pointer moves. However, we can attach input devices to it, and
   * it will generate aggregate events for all of them. In these events, we
   * can choose how we want to process them, forwarding them to clients and
   * moving the cursor around. More detail on this process is described in my
   * input handling blog post:
   *
   * https://drewdevault.com/2018/07/17/Input-handling-in-wlroots.html
   *
   * And more comments are sprinkled throughout the notify functions above.
   */
  cursor_motion.notify = server_cursor_motion;
  wl_signal_add(&cursor->events.motion, &cursor_motion);
  cursor_motion_absolute.notify = server_cursor_motion_absolute;
  wl_signal_add(&cursor->events.motion_absolute,
                &cursor_motion_absolute);
  cursor_button.notify = server_cursor_button;
  wl_signal_add(&cursor->events.button, &cursor_button);
  cursor_axis.notify = server_cursor_axis;
  wl_signal_add(&cursor->events.axis, &cursor_axis);
  cursor_frame.notify = server_cursor_frame;
  wl_signal_add(&cursor->events.frame, &cursor_frame);

  /*
   * Configures a seat, which is a single "seat" at which a user sits and
   * operates the computer. This conceptually includes up to one keyboard,
   * pointer, touch, and drawing tablet device. We also rig up a listener to
   * let us know when new input devices are available on the backend.
   */
  wl_list_init(&keyboards);
  new_input.notify = server_new_input;
  wl_signal_add(&backend->events.new_input, &new_input);
  seat = wlr_seat_create(wl_display, "seat0");
  request_cursor.notify = seat_request_cursor;
  wl_signal_add(&seat->events.request_set_cursor, &request_cursor);

  /* Add a Unix socket to the Wayland display. */
  const char *socket = wl_display_add_socket_auto(wl_display);
  if (!socket) {
    wlr_backend_destroy(backend);
    exit(EXIT_FAILURE);
  }

  /* Start the backend. This will enumerate outputs and inputs, become the DRM
   * master, etc */
  if (!wlr_backend_start(backend)) {
    wlr_backend_destroy(backend);
    wl_display_destroy(wl_display);
    exit(EXIT_FAILURE);
  }

#ifdef WLR_HAS_XWAYLAND
  xwayland =
      wlr_xwayland_create(wl_display, compositor, false);
  setenv("DISPLAY", xwayland->display_name, true);
#endif

  /* Set the WAYLAND_DISPLAY environment variable to our socket and run the
   * startup command if requested. */
  setenv("WAYLAND_DISPLAY", socket, true);
  wlr_log(WLR_INFO, "Running TheInterface on WAYLAND_DISPLAY=%s", socket);
}

/// automatically run when the program is about to exit
ti_server::~ti_server() {
  wlr_log(WLR_INFO, "Deallocating ti_server resources");
#ifdef WLR_HAS_XWAYLAND
  wlr_xwayland_destroy(xwayland);
#endif
  /* Once wl_display_run returns, we shut down the server. */
  wl_display_destroy_clients(wl_display);
  wl_display_destroy(wl_display);
}