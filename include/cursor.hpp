#ifndef TI_CURSOR_HPP
#define TI_CURSOR_HPP

namespace ti {
class seat;
enum cursor_mode {
  CURSOR_PASSTHROUGH,
  CURSOR_MOVE,
  CURSOR_RESIZE,
};
} // namespace ti

void process_cursor_motion(ti::seat *seat, unsigned time);

/** This event is raised by the backend when a new input device becomes
 * available. */
void handle_new_input(struct wl_listener *listener, void *data);

/** This event is forwarded by the cursor when a pointer emits a _relative_
 * pointer motion event (i.e. a delta) */
void handle_cursor_motion(struct wl_listener *listener, void *data);

/** This event is forwarded by the cursor when a pointer emits an _absolute_
 * motion event, from 0..1 on each axis. This happens, for example, when
 * wlroots is running under a Wayland window rather than KMS+DRM, and you
 * move the mouse over the window. You could enter the window from any edge,
 * so we have to warp the mouse there. There is also some hardware which
 * emits these events. */
void handle_cursor_motion_absolute(struct wl_listener *listener, void *data);

/** This event is forwarded by the cursor when a pointer emits a button
 * event. */
void handle_cursor_button(struct wl_listener *listener, void *data);

/** This event is forwarded by the cursor when a pointer emits an axis event,
 * for example when you move the scroll wheel. */
void handle_cursor_axis(struct wl_listener *listener, void *data);

/** This event is forwarded by the cursor when a pointer emits an frame
 * event. Frame events are sent after regular pointer events to group
 * multiple events together. For instance, two axis events may happen at the
 * same time, in which case a frame event won't be sent in between. */
void handle_cursor_frame(struct wl_listener *listener, void *data);

#endif