#include "cursor.hpp"
#include "layer_shell.hpp"
#include "output.hpp"
#include "seat.hpp"
#include "server.hpp"
#include "xdg_shell.hpp"
#include "xwayland.hpp"

#include "desktop.hpp"

ti::view *ti::desktop::view_at(double lx, double ly,
                               struct wlr_surface **surface, double *sx,
                               double *sy) {
  ti::view *view;
  wl_list_for_each(view, &this->wem_views, wem_link) {
    if (view->at(lx, ly, surface, sx, sy)) {
      return view;
    }
  }
  return NULL;
}

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

  this->seat = new ti::seat(this);

  this->xdg_shell = wlr_xdg_shell_create(server->display);
  this->new_xdg_surface.notify = handle_new_xdg_surface;
  wl_signal_add(&this->xdg_shell->events.new_surface, &this->new_xdg_surface);

  this->layer_shell = wlr_layer_shell_v1_create(server->display);
  this->new_layer_shell_surface.notify = handle_new_layer_shell_surface;
  wl_signal_add(&this->layer_shell->events.new_surface,
                &this->new_layer_shell_surface);

  this->new_input.notify = handle_new_input;
  wl_signal_add(&this->server->backend->events.new_input, &this->new_input);

  this->foreign_toplevel_manager_v1 =
      wlr_foreign_toplevel_manager_v1_create(server->display);

#ifdef WLR_HAS_XWAYLAND
  this->xwayland =
      wlr_xwayland_create(server->display, this->compositor, false);
  this->new_xwayland_surface.notify = handle_new_xwayland_surface;
  wl_signal_add(&this->xwayland->events.new_surface,
                &this->new_xwayland_surface);
  setenv("DISPLAY", this->xwayland->display_name, true);

  this->seat->setup_xwayland_cursor(this->xwayland);

  wlr_xwayland_set_seat(this->xwayland, this->seat->wlr_seat);
#endif

  // struct wl_surface *wl_surface = wl_compositor_create_surface(compositor);
  // this->zwlr_layer_surface = zwlr_layer_shell_v1_get_layer_surface(
  //     layer_shell, wl_surface, wl_output, layer, namespace);
  // assert(layer_surface);
  // zwlr_layer_surface_v1_set_size(layer_surface, width, height);
  // zwlr_layer_surface_v1_set_anchor(layer_surface, anchor);
  // zwlr_layer_surface_v1_set_exclusive_zone(layer_surface, exclusive_zone);
  // zwlr_layer_surface_v1_set_margin(layer_surface, margin_top, margin_right,
  //                                  margin_bottom, margin_left);
  // zwlr_layer_surface_v1_set_keyboard_interactivity(layer_surface,
  //                                                  keyboard_interactive);
  // zwlr_layer_surface_v1_add_listener(layer_surface, &layer_surface_listener,
  //                                    layer_surface);
  // wl_surface_commit(wl_surface);
  // wl_display_roundtrip(display);

  this->presentation =
      wlr_presentation_create(server->display, server->backend);
}

ti::desktop::~desktop() {
  delete this->seat;
#ifdef WLR_HAS_XWAYLAND
  wlr_xwayland_destroy(this->xwayland);
#endif
}
