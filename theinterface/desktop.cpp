#include "cursor.hpp"
#include "output.hpp"
#include "seat.hpp"
#include "server.hpp"
#include "xdg_shell.hpp"
#include "xwayland.hpp"

#include "desktop.hpp"

ti::desktop::desktop(ti::server *s) {
  this->server = s;

  /* Set up our list of views and the xdg-shell. The xdg-shell is a Wayland
   * protocol which is used for application windows. For more detail on
   * shells, refer to my article:
   *
   * https://drewdevault.com/2018/07/29/Wayland-shells.html
   */
  wl_list_init(&this->views);

  // these are all the views with was_ever_mapped set to true. We need this for
  // alt+tab to work
  wl_list_init(&this->wem_views);

  /* Configure a listener to be notified when new outputs are available on the
   * backend. */
  wl_list_init(&this->outputs);
  this->new_output.notify = handle_new_output;
  wl_signal_add(&server->backend->events.new_output, &this->new_output);

  /* Creates an output layout, which a wlroots utility for working with an
   * arrangement of screens in a physical layout. */
  this->output_layout = wlr_output_layout_create();

  /* This creates some hands-off wlroots interfaces. The compositor is
   * necessary for clients to allocate surfaces and the data device manager
   * handles the clipboard. Each of these wlroots interfaces has room for you
   * to dig your fingers in and play with their behavior if you want. */
  this->compositor = wlr_compositor_create(server->display, server->renderer);

  this->xdg_shell = wlr_xdg_shell_create(server->display);
  this->new_xdg_surface.notify = handle_new_xdg_surface;
  wl_signal_add(&this->xdg_shell->events.new_surface, &this->new_xdg_surface);

  /*
   * Creates a cursor, which is a wlroots utility for tracking the cursor
   * image shown on screen.
   */
  this->cursor = wlr_cursor_create();
  wlr_cursor_attach_output_layout(this->cursor, this->output_layout);

  /* Creates an xcursor manager, another wlroots utility which loads up
   * Xcursor themes to source cursor images from and makes sure that cursor
   * images are available at all scale factors on the screen (necessary for
   * HiDPI support). We add a cursor theme at scale factor 1 to begin with. */
  this->cursor_mgr = wlr_xcursor_manager_create(nullptr, 24);
  wlr_xcursor_manager_load(this->cursor_mgr, 1);

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
  this->cursor_motion.notify = handle_cursor_motion;
  wl_signal_add(&this->cursor->events.motion, &this->cursor_motion);
  this->cursor_motion_absolute.notify = handle_cursor_motion_absolute;
  wl_signal_add(&this->cursor->events.motion_absolute,
                &this->cursor_motion_absolute);
  this->cursor_button.notify = handle_cursor_button;
  wl_signal_add(&this->cursor->events.button, &this->cursor_button);
  this->cursor_axis.notify = handle_cursor_axis;
  wl_signal_add(&this->cursor->events.axis, &this->cursor_axis);
  this->cursor_frame.notify = handle_cursor_frame;
  wl_signal_add(&this->cursor->events.frame, &this->cursor_frame);

  this->new_input.notify = handle_new_input;
  wl_signal_add(&this->server->backend->events.new_input, &this->new_input);

  this->foreign_toplevel_manager_v1 =
      wlr_foreign_toplevel_manager_v1_create(server->display);

  /*
   * Configures a seat, which is a single "seat" at which a user sits and
   * operates the computer. This conceptually includes up to one keyboard,
   * pointer, touch, and drawing tablet device. We also rig up a listener to
   * let us know when new input devices are available on the backend.
   */
  wl_list_init(&this->keyboards);

  this->seat = wlr_seat_create(server->display, "seat0");
  this->request_cursor.notify = seat_request_cursor;
  wl_signal_add(&this->seat->events.request_set_cursor, &this->request_cursor);

#ifdef WLR_HAS_XWAYLAND
  this->xwayland =
      wlr_xwayland_create(server->display, this->compositor, false);
  this->new_xwayland_surface.notify = handle_new_xwayland_surface;
  wl_signal_add(&this->xwayland->events.new_surface,
                &this->new_xwayland_surface);
  setenv("DISPLAY", this->xwayland->display_name, true);

  this->xwayland_xcursor_manager = wlr_xcursor_manager_create(nullptr, 24);
  wlr_xcursor_manager_load(this->xwayland_xcursor_manager, 1);
  this->xcursor = wlr_xcursor_manager_get_xcursor(
      this->xwayland_xcursor_manager, "left_ptr", 1);
  if (this->xcursor != NULL) {
    struct wlr_xcursor_image *image = this->xcursor->images[0];
    wlr_xwayland_set_cursor(this->xwayland, image->buffer, image->width * 4,
                            image->width, image->height, image->hotspot_x,
                            image->hotspot_y);
  }
  wlr_xwayland_set_seat(this->xwayland, this->seat);
#endif

  this->presentation =
      wlr_presentation_create(server->display, server->backend);
}

ti::desktop::~desktop() {
#ifdef WLR_HAS_XWAYLAND
  wlr_xwayland_destroy(this->xwayland);
#endif
}
