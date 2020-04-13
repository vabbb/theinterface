#include <cstdlib>

extern "C" {
#include <wlr/types/wlr_xcursor_manager.h>
#include <wlr/util/log.h>
}

#include "keyboard.hpp"
#include "seat.hpp"
#include "server.hpp"
#include "xdg_shell.hpp"

#include "cursor.hpp"

void ti::server::new_pointer(struct wlr_input_device *device) {
  wlr_cursor_attach_input_device(this->cursor, device);
}

void handle_new_input(struct wl_listener *listener, void *data) {
  ti::server *server = wl_container_of(listener, server, new_input);
  struct wlr_input_device *device =
      reinterpret_cast<struct wlr_input_device *>(data);

  switch (device->type) {
  case WLR_INPUT_DEVICE_KEYBOARD:
    server->new_keyboard(device);
    break;
  case WLR_INPUT_DEVICE_POINTER:
    server->new_pointer(device);
    break;
  default:
    break;
  }
  /* We need to let the wlr_seat know what our capabilities are, which is
   * communiciated to the client. In TinyWL we always have a cursor, even if
   * there are no pointer devices, so we always include that capability. */
  uint32_t caps = WL_SEAT_CAPABILITY_POINTER;
  if (!wl_list_empty(&server->keyboards)) {
    caps |= WL_SEAT_CAPABILITY_KEYBOARD;
  }
  wlr_seat_set_capabilities(server->seat, caps);
}

/* Move the grabbed view to the new position. */
static void process_cursor_move(ti::server *server, uint32_t time) {
  server->grabbed_view->x = server->cursor->x - server->grab_x;
  server->grabbed_view->y = server->cursor->y - server->grab_y;
}

/** Resizing the grabbed view can be a little bit complicated, because we
 * could be resizing from any corner or edge. This not only resizes the view
 * on one or two axes, but can also move the view if you resize from the top
 * or left edges (or top-left corner).
 *
 * Note that I took some shortcuts here. In a more fleshed-out compositor,
 * you'd wait for the client to prepare a buffer at the new size, then
 * commit any movement that was prepared.
 */
static void process_cursor_resize(ti::server *server, uint32_t time) {
  ti::view *view = server->grabbed_view;
  double dx = server->cursor->x - server->grab_x;
  double dy = server->cursor->y - server->grab_y;
  double x = view->x;
  double y = view->y;
  int width = server->grab_width;
  int height = server->grab_height;
  if (server->resize_edges & WLR_EDGE_TOP) {
    y = server->grab_y + dy;
    height -= dy;
    if (height < 1) {
      y += height;
    }
  } else if (server->resize_edges & WLR_EDGE_BOTTOM) {
    height += dy;
  }
  if (server->resize_edges & WLR_EDGE_LEFT) {
    x = server->grab_x + dx;
    width -= dx;
    if (width < 1) {
      x += width;
    }
  } else if (server->resize_edges & WLR_EDGE_RIGHT) {
    width += dx;
  }
  view->x = x;
  view->y = y;
  wlr_xdg_toplevel_set_size(view->xdg_surface, width, height);
}

static void process_cursor_motion(ti::server *server, uint32_t time) {
  /* If the mode is non-passthrough, delegate to those functions. */
  if (server->cursor_mode == ti::CURSOR_MOVE) {
    process_cursor_move(server, time);
    return;
  } else if (server->cursor_mode == ti::CURSOR_RESIZE) {
    process_cursor_resize(server, time);
    return;
  }

  /* Otherwise, find the view under the pointer and send the event along. */
  double sx, sy;
  struct wlr_seat *seat = server->seat;
  struct wlr_surface *surface = NULL;
  ti::view *view = desktop_view_at(server, server->cursor->x, server->cursor->y,
                                   &surface, &sx, &sy);
  if (!view) {
    /* If there's no view under the cursor, set the cursor image to a
     * default. This is what makes the cursor image appear when you move it
     * around the screen, not over any views. */
    wlr_xcursor_manager_set_cursor_image(server->cursor_mgr, "left_ptr",
                                         server->cursor);
  }
  if (surface) {
    bool focus_changed = seat->pointer_state.focused_surface != surface;
    /*
     * "Enter" the surface if necessary. This lets the client know that the
     * cursor has entered one of its surfaces.
     *
     * Note that this gives the surface "pointer focus", which is distinct
     * from keyboard focus. You get pointer focus by moving the pointer over
     * a window.
     */
    wlr_seat_pointer_notify_enter(seat, surface, sx, sy);
    if (!focus_changed) {
      /* The enter event contains coordinates, so we only need to notify
       * on motion if the focus did not change. */
      wlr_seat_pointer_notify_motion(seat, time, sx, sy);
    }
  } else {
    /* Clear pointer focus so future button events and such are not sent to
     * the last client to have the cursor over it. */
    wlr_seat_pointer_clear_focus(seat);
  }
}

void handle_cursor_motion(struct wl_listener *listener, void *data) {
  ti::server *server = wl_container_of(listener, server, cursor_motion);
  struct wlr_event_pointer_motion *event =
      (struct wlr_event_pointer_motion *)data;
  /* The cursor doesn't move unless we tell it to. The cursor automatically
   * handles constraining the motion to the output layout, as well as any
   * special configuration applied for the specific input device which
   * generated the event. You can pass NULL for the device if you want to move
   * the cursor around without any input. */
  wlr_cursor_move(server->cursor, event->device, event->delta_x,
                  event->delta_y);
  process_cursor_motion(server, event->time_msec);
}

void handle_cursor_motion_absolute(struct wl_listener *listener, void *data) {
  ti::server *server =
      wl_container_of(listener, server, cursor_motion_absolute);
  struct wlr_event_pointer_motion_absolute *event =
      (struct wlr_event_pointer_motion_absolute *)data;
  wlr_cursor_warp_absolute(server->cursor, event->device, event->x, event->y);
  process_cursor_motion(server, event->time_msec);
}

void handle_cursor_button(struct wl_listener *listener, void *data) {
  ti::server *server = wl_container_of(listener, server, cursor_button);
  struct wlr_event_pointer_button *event =
      reinterpret_cast<struct wlr_event_pointer_button *>(data);
  /* Notify the client with pointer focus that a button press has occurred */
  wlr_seat_pointer_notify_button(server->seat, event->time_msec, event->button,
                                 event->state);
  double sx, sy;
  struct wlr_seat *seat = server->seat;
  struct wlr_surface *surface = NULL;
  ti::view *view = desktop_view_at(server, server->cursor->x, server->cursor->y,
                                   &surface, &sx, &sy);
  if (event->state == WLR_BUTTON_RELEASED) {
    /* If you released any buttons, we exit interactive move/resize mode. */
    server->cursor_mode = ti::CURSOR_PASSTHROUGH;
  } else {
    if (!view) {
      // unfocus();
      return;
    }
    switch (view->type) {
    case ti::XDG_SHELL_VIEW: {
      ti::xdg_view *v = dynamic_cast<ti::xdg_view *>(view);
      v->focus(surface);
      break;
    }
    case ti::XWAYLAND_VIEW: {
      ti::xwayland_view *v = dynamic_cast<ti::xwayland_view *>(view);
      v->focus(surface);
      break;
    }
    default: {
      break;
    }
    }
  }
}

void handle_cursor_axis(struct wl_listener *listener, void *data) {
  ti::server *server = wl_container_of(listener, server, cursor_axis);
  struct wlr_event_pointer_axis *event = (struct wlr_event_pointer_axis *)data;
  /* Notify the client with pointer focus of the axis event. */
  wlr_seat_pointer_notify_axis(server->seat, event->time_msec,
                               event->orientation, event->delta,
                               event->delta_discrete, event->source);
}

void handle_cursor_frame(struct wl_listener *listener, void *data) {
  ti::server *server = wl_container_of(listener, server, cursor_frame);
  /* Notify the client with pointer focus of the frame event. */
  wlr_seat_pointer_notify_frame(server->seat);
}
