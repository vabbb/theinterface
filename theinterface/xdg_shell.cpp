#include <cstdlib>

extern "C" {
#include <wlr/util/log.h>
}
#include "server.hpp"

#include "xdg_shell.hpp"

/** This function sets up an interactive move or resize operation, where the
 * compositor stops propegating pointer events to clients and instead
 * consumes them itself, to move or resize windows. */
static void begin_interactive(ti::xdg_view *view, ti::cursor_mode mode,
                              uint32_t edges) {
  ti::server *server = view->server;
  struct wlr_surface *focused_surface =
      server->seat->pointer_state.focused_surface;
  if (view->xdg_surface->surface != focused_surface) {
    /* Deny move/resize requests from unfocused clients. */
    return;
  }
  server->grabbed_view = view;
  server->cursor_mode = mode;
  struct wlr_box geo_box;
  wlr_xdg_surface_get_geometry(view->xdg_surface, &geo_box);
  if (mode == ti::CURSOR_MOVE) {
    server->grab_x = server->cursor->x - view->x;
    server->grab_y = server->cursor->y - view->y;
  } else {
    server->grab_x = server->cursor->x + geo_box.x;
    server->grab_y = server->cursor->y + geo_box.y;
  }
  server->grab_width = geo_box.width;
  server->grab_height = geo_box.height;
  server->resize_edges = edges;
}

/** Called when the surface is mapped, or ready to display on-screen. */
static void handle_xdg_surface_map(struct wl_listener *listener, void *data) {
  ti::xdg_view *view = wl_container_of(listener, view, map);
  view->mapped = true;
  if (view->was_ever_mapped == false) {
    view->was_ever_mapped = true;
    ti::view *v = dynamic_cast<ti::view *>(view);
    wl_list_insert(&view->server->wem_views, &v->wem_link);
  }
  view->focus(view->surface);
}

/** Called when the surface is unmapped, and should no longer be shown. */
static void handle_xdg_surface_unmap(struct wl_listener *listener, void *data) {
  ti::view *view = wl_container_of(listener, view, unmap);
  view->mapped = false;
}

/* Called when the surface is destroyed and should never be shown again. */
static void handle_xdg_surface_destroy(struct wl_listener *listener,
                                       void *data) {
  ti::view *view = wl_container_of(listener, view, destroy);
  wl_list_remove(&view->link);

  if (view->was_ever_mapped) {
    wl_list_remove(&view->wem_link);
  }

  ti::xdg_view *v = dynamic_cast<ti::xdg_view *>(view);
  delete v;
}

/** This event is raised when a client would like to begin an interactive
 * move, typically because the user clicked on their client-side
 * decorations. Note that a more sophisticated compositor should check the
 * provied serial against a list of button press serials sent to this
 * client, to prevent the client from requesting this whenever they want. */
static void handle_xdg_toplevel_request_move(struct wl_listener *listener,
                                             void *data) {
  ti::xdg_view *view = wl_container_of(listener, view, request_move);
  begin_interactive(view, ti::CURSOR_MOVE, 0);
}

/** This event is raised when a client would like to begin an interactive
 * resize, typically because the user clicked on their client-side
 * decorations. Note that a more sophisticated compositor should check the
 * provied serial against a list of button press serials sent to this
 * client, to prevent the client from requesting this whenever they want. */
static void handle_xdg_toplevel_request_resize(struct wl_listener *listener,
                                               void *data) {
  struct wlr_xdg_toplevel_resize_event *event =
      (struct wlr_xdg_toplevel_resize_event *)(data);
  ti::xdg_view *view = wl_container_of(listener, view, request_resize);
  begin_interactive(view, ti::CURSOR_RESIZE, event->edges);
}

void handle_new_xdg_surface(struct wl_listener *listener, void *data) {
  ti::server *server = wl_container_of(listener, server, new_xdg_surface);
  struct wlr_xdg_surface *xdg_surface =
      reinterpret_cast<struct wlr_xdg_surface *>(data);
  if (xdg_surface->role != WLR_XDG_SURFACE_ROLE_TOPLEVEL) {
    return;
  }

  /* Allocate a ti::view for this surface */
  ti::xdg_view *view = new ti::xdg_view;

  view->server = server;
  view->xdg_surface = xdg_surface;
  view->surface = xdg_surface->surface;

  // Find out the client's pid
  // ONLY WORKS WITH WAYLAND SURFACES!! DOESNT WORK WITH XWAYLAND
  struct wl_client *client =
      wl_resource_get_client(view->xdg_surface->resource);
  wl_client_get_credentials(client, &view->pid, &view->uid, &view->gid);

  /* Listen to the various events it can emit */
  view->map.notify = handle_xdg_surface_map;
  wl_signal_add(&xdg_surface->events.map, &view->map);
  view->unmap.notify = handle_xdg_surface_unmap;
  wl_signal_add(&xdg_surface->events.unmap, &view->unmap);
  view->destroy.notify = handle_xdg_surface_destroy;
  wl_signal_add(&xdg_surface->events.destroy, &view->destroy);

  /* cotd */
  struct wlr_xdg_toplevel *toplevel = xdg_surface->toplevel;
  view->request_move.notify = handle_xdg_toplevel_request_move;
  wl_signal_add(&toplevel->events.request_move, &view->request_move);
  view->request_resize.notify = handle_xdg_toplevel_request_resize;
  wl_signal_add(&toplevel->events.request_resize, &view->request_resize);

  /* Add it to the list of views. */
  ti::view *v = dynamic_cast<ti::view *>(view);
  wl_list_insert(&server->views, &v->link);
}

std::string ti::xdg_view::get_title() {
  return this->xdg_surface->toplevel->title;
}

ti::xdg_view::xdg_view() : view(ti::XDG_SHELL_VIEW) {}
ti::xdg_view::~xdg_view() {}