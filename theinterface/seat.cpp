#include "cursor.hpp"
#include "server.hpp"
#include "xdg_shell.hpp"
#include "xwayland.hpp"

#include "seat.hpp"

void seat_request_cursor(struct wl_listener *listener, void *data) {
  ti::seat *seat = wl_container_of(listener, seat, request_cursor);
  auto *event =
      reinterpret_cast<wlr_seat_pointer_request_set_cursor_event *>(data);
  struct wlr_seat_client *focused_client =
      seat->wlr_seat->pointer_state.focused_client;
  /* This can be sent by any client, so we check to make sure this one is
   * actually has pointer focus first. */
  if (focused_client == event->seat_client) {
    /* Once we've vetted the client, we can tell the cursor to use the
     * provided surface as the cursor image. It will set the hardware cursor
     * on the output that it's currently on and continue to do so as the
     * cursor moves between outputs. */
    wlr_cursor_set_surface(seat->cursor, event->surface, event->hotspot_x,
                           event->hotspot_y);
  }
}

void ti::seat::set_focus_layer(struct wlr_layer_surface_v1 *layer) {
  if (!layer) {
    if (this->focused_layer) {
      this->focused_layer = NULL;
      if (!wl_list_empty(&this->desktop->wem_views)) {
        // Focus first view
        ti::view *first_seat_view = wl_container_of(
            this->desktop->wem_views.next, first_seat_view, link);
        this->focus(first_seat_view);
      } else {
        this->focus(NULL);
      }
    }
    return;
  }
  struct wlr_keyboard *keyboard = wlr_seat_get_keyboard(this->wlr_seat);
  // if (!roots_seat_allow_input(seat, layer->resource)) {
  //   return;
  // }
  // if (this->has_focus) {
  ti::view *prev_focus = this->focused_view;
  wlr_seat_keyboard_clear_focus(this->wlr_seat);
  prev_focus->deactivate();
  // }
  // this->has_focus = false;
  if (layer->current.layer >= ZWLR_LAYER_SHELL_V1_LAYER_TOP) {
    this->focused_layer = reinterpret_cast<ti::layer_view *>(layer->data);
  }
  if (keyboard != NULL) {
    wlr_seat_keyboard_notify_enter(this->wlr_seat, layer->surface,
                                   keyboard->keycodes, keyboard->num_keycodes,
                                   &keyboard->modifiers);
  } else {
    wlr_seat_keyboard_notify_enter(this->wlr_seat, layer->surface, NULL, 0,
                                   NULL);
  }

  // if (this->cursor) {
  //   roots_cursor_update_focus(this->cursor);
  // }

  // roots_input_method_relay_set_focus(&this->im_relay, layer->surface);
}

void ti::seat::focus(ti::view *v) {
  ti::view *prev_focus = this->focused_view;
  if (prev_focus == v) {
    return;
  } else if (prev_focus) {
    prev_focus->deactivate();
  }

  /* Move the view to the front */
  if (v->was_ever_mapped) {
    wl_list_remove(&v->wem_link);
    wl_list_insert(&v->desktop->wem_views, &v->wem_link);
  }
  wl_list_remove(&v->link);
  wl_list_insert(&v->desktop->views, &v->link);

  v->damage_whole();

  v->activate();
  this->focused_view = v;

  struct wlr_keyboard *keyboard = wlr_seat_get_keyboard(this->wlr_seat);

  /*
   * Tell the seat to have the keyboard enter this surface. wlroots will keep
   * track of this and automatically send key events to the appropriate
   * clients without additional work on your part.
   */
  wlr_seat_keyboard_notify_enter(this->wlr_seat, v->surface, keyboard->keycodes,
                                 keyboard->num_keycodes, &keyboard->modifiers);
}

ti::seat::seat(ti::desktop *d) {
  this->desktop = d;

  /*
   * Creates a cursor, which is a wlroots utility for tracking the cursor
   * image shown on screen.
   */
  this->cursor = wlr_cursor_create();
  wlr_cursor_attach_output_layout(this->cursor, desktop->output_layout);

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

  /*
   * Configures a seat, which is a single "seat" at which a user sits and
   * operates the computer. This conceptually includes up to one keyboard,
   * pointer, touch, and drawing tablet device. We also rig up a listener to
   * let us know when new input devices are available on the backend.
   */
  wl_list_init(&this->keyboards);

  this->wlr_seat = wlr_seat_create(desktop->server->display, "seat0");
  this->request_cursor.notify = seat_request_cursor;
  wl_signal_add(&this->wlr_seat->events.request_set_cursor,
                &this->request_cursor);
}

void ti::seat::setup_xwayland_cursor(wlr_xwayland *xwayland) {
#ifdef WLR_HAS_XWAYLAND
  this->xwayland_xcursor_manager = wlr_xcursor_manager_create(nullptr, 24);
  wlr_xcursor_manager_load(this->xwayland_xcursor_manager, 1);
  this->xcursor = wlr_xcursor_manager_get_xcursor(
      this->xwayland_xcursor_manager, "left_ptr", 1);
  if (this->xcursor != NULL) {
    struct wlr_xcursor_image *image = this->xcursor->images[0];
    wlr_xwayland_set_cursor(xwayland, image->buffer, image->width * 4,
                            image->width, image->height, image->hotspot_x,
                            image->hotspot_y);
  }
#endif
}

ti::seat::~seat() {}