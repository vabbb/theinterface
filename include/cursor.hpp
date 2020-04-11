#ifndef TI_CURSOR_HPP
#define TI_CURSOR_HPP

/* For brevity's sake, struct members are annotated where they are used. */
namespace ti {
enum cursor_mode {
  CURSOR_PASSTHROUGH,
  CURSOR_MOVE,
  CURSOR_RESIZE,
};
}

void server_new_input(struct wl_listener *listener, void *data);
void server_cursor_motion(struct wl_listener *listener, void *data);
void server_cursor_motion_absolute(struct wl_listener *listener, void *data);
void server_cursor_button(struct wl_listener *listener, void *data);
void server_cursor_axis(struct wl_listener *listener, void *data);
void server_cursor_frame(struct wl_listener *listener, void *data);

#endif