#include <cstdlib>

extern "C" {
#include <wlr/types/wlr_xcursor_manager.h>
#include <wlr/util/log.h>
}

#include "keyboard.hpp"
#include "seat.hpp"
#include "xdg_shell.hpp"
#include "xwayland.hpp"

#include "server.hpp"

#include "desktop.hpp"

#include "cursor.hpp"

void ti::desktop::new_pointer(struct wlr_input_device *device) {
  wlr_cursor_attach_input_device(this->cursor, device);
}

void handle_new_input(struct wl_listener *listener, void *data) {
  ti::desktop *desktop = wl_container_of(listener, desktop, new_input);
  struct wlr_input_device *device =
      reinterpret_cast<struct wlr_input_device *>(data);

  switch (device->type) {
  case WLR_INPUT_DEVICE_KEYBOARD:
    desktop->new_keyboard(device);
    break;
  case WLR_INPUT_DEVICE_POINTER:
    desktop->new_pointer(device);
    break;
  default:
    break;
  }
  /* We need to let the wlr_seat know what our capabilities are, which is
   * communiciated to the client. In TinyWL we always have a cursor, even if
   * there are no pointer devices, so we always include that capability. */
  uint32_t caps = WL_SEAT_CAPABILITY_POINTER;
  if (!wl_list_empty(&desktop->keyboards)) {
    caps |= WL_SEAT_CAPABILITY_KEYBOARD;
  }
  wlr_seat_set_capabilities(desktop->seat, caps);
}

/* Move the grabbed view to the new position. */
static void process_cursor_move(ti::desktop *desktop, uint32_t time) {
  desktop->grabbed_view->damage_whole();
  desktop->grabbed_view->box.x = desktop->cursor->x - desktop->grab_x;
  desktop->grabbed_view->box.y = desktop->cursor->y - desktop->grab_y;
  desktop->grabbed_view->damage_whole();
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
static void process_cursor_resize(ti::desktop *desktop, uint32_t time) {
  ti::view *view = desktop->grabbed_view;
  double dx = desktop->cursor->x - desktop->grab_x;
  double dy = desktop->cursor->y - desktop->grab_y;
  double x = view->box.x;
  double y = view->box.y;
  int width = desktop->grab_width;
  int height = desktop->grab_height;
  if (desktop->resize_edges & WLR_EDGE_TOP) {
    y = desktop->view_y + dy;
    height -= dy;
    if (height < 1) {
      y += height;
    }
  } else if (desktop->resize_edges & WLR_EDGE_BOTTOM) {
    height += dy;
  }
  if (desktop->resize_edges & WLR_EDGE_LEFT) {
    x = desktop->view_x + dx;
    width -= dx;
    if (width < 1) {
      x += width;
    }
  } else if (desktop->resize_edges & WLR_EDGE_RIGHT) {
    width += dx;
  }

  view->damage_whole();
  view->box = {.x = (int32_t)x,
               .y = (int32_t)y,
               .width = (int32_t)width,
               .height = (int32_t)height};

  switch (view->type) {
  case ti::XDG_SHELL_VIEW: {
    ti::xdg_view *v = dynamic_cast<ti::xdg_view *>(view);
    wlr_xdg_toplevel_set_size(v->xdg_surface, width, height);
    break;
  }
  case ti::XWAYLAND_VIEW: {
    ti::xwayland_view *v = dynamic_cast<ti::xwayland_view *>(view);
    wlr_xwayland_surface_configure(v->xwayland_surface, x, y, width, height);
    break;
  }
  }
  view->damage_whole();
}

static void process_cursor_motion(ti::desktop *desktop, uint32_t time) {
  /* If the mode is non-passthrough, delegate to those functions. */
  if (desktop->cursor_mode == ti::CURSOR_MOVE) {
    process_cursor_move(desktop, time);
    return;
  } else if (desktop->cursor_mode == ti::CURSOR_RESIZE) {
    process_cursor_resize(desktop, time);
    return;
  }

  /* Otherwise, find the view under the pointer and send the event along. */
  double sx, sy;
  struct wlr_seat *seat = desktop->seat;
  struct wlr_surface *surface = NULL;
  ti::view *view = desktop_view_at(desktop, desktop->cursor->x,
                                   desktop->cursor->y, &surface, &sx, &sy);
  if (!view) {
    /* If there's no view under the cursor, set the cursor image to a
     * default. This is what makes the cursor image appear when you move it
     * around the screen, not over any views. */
    wlr_xcursor_manager_set_cursor_image(desktop->cursor_mgr, "left_ptr",
                                         desktop->cursor);
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
  ti::desktop *desktop = wl_container_of(listener, desktop, cursor_motion);
  struct wlr_event_pointer_motion *event =
      (struct wlr_event_pointer_motion *)data;
  /* The cursor doesn't move unless we tell it to. The cursor automatically
   * handles constraining the motion to the output layout, as well as any
   * special configuration applied for the specific input device which
   * generated the event. You can pass NULL for the device if you want to move
   * the cursor around without any input. */
  wlr_cursor_move(desktop->cursor, event->device, event->delta_x,
                  event->delta_y);
  process_cursor_motion(desktop, event->time_msec);
}

void handle_cursor_motion_absolute(struct wl_listener *listener, void *data) {
  ti::desktop *desktop =
      wl_container_of(listener, desktop, cursor_motion_absolute);
  struct wlr_event_pointer_motion_absolute *event =
      (struct wlr_event_pointer_motion_absolute *)data;
  wlr_cursor_warp_absolute(desktop->cursor, event->device, event->x, event->y);
  process_cursor_motion(desktop, event->time_msec);
}

void handle_cursor_button(struct wl_listener *listener, void *data) {
  ti::desktop *desktop = wl_container_of(listener, desktop, cursor_button);
  struct wlr_event_pointer_button *event =
      reinterpret_cast<struct wlr_event_pointer_button *>(data);
  /* Notify the client with pointer focus that a button press has occurred */
  wlr_seat_pointer_notify_button(desktop->seat, event->time_msec, event->button,
                                 event->state);
  double sx, sy;
  struct wlr_seat *seat = desktop->seat;
  struct wlr_surface *surface = NULL;
  ti::view *view = desktop_view_at(desktop, desktop->cursor->x,
                                   desktop->cursor->y, &surface, &sx, &sy);
  if (event->state == WLR_BUTTON_RELEASED) {
    /* If you released any buttons, we exit interactive move/resize mode. */
    desktop->cursor_mode = ti::CURSOR_PASSTHROUGH;
  } else {
    if (!view) {
      // unfocus();
      return;
    }
    // focus view when you click on it
    view->focus();
  }
}

void handle_cursor_axis(struct wl_listener *listener, void *data) {
  ti::desktop *desktop = wl_container_of(listener, desktop, cursor_axis);
  struct wlr_event_pointer_axis *event = (struct wlr_event_pointer_axis *)data;
  /* Notify the client with pointer focus of the axis event. */
  wlr_seat_pointer_notify_axis(desktop->seat, event->time_msec,
                               event->orientation, event->delta,
                               event->delta_discrete, event->source);
}

void handle_cursor_frame(struct wl_listener *listener, void *data) {
  ti::desktop *desktop = wl_container_of(listener, desktop, cursor_frame);
  /* Notify the client with pointer focus of the frame event. */
  wlr_seat_pointer_notify_frame(desktop->seat);
}
