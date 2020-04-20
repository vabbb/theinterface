#include <cstdlib>

extern "C" {
#include <wlr/types/wlr_foreign_toplevel_management_v1.h>
#include <wlr/util/log.h>
}

#include "seat.hpp"

#include "server.hpp"

#include "xdg_shell.hpp"

static void handle_xdg_surface_commit(struct wl_listener *listener,
                                      void *data) {
  ti::xdg_view *view = wl_container_of(listener, view, surface_commit);
  view->damage_partial();
}

/** Called when the surface is mapped, or ready to display on-screen. */
static void handle_xdg_surface_map(struct wl_listener *listener, void *data) {
  ti::xdg_view *view = wl_container_of(listener, view, map);
  view->mapped = true;

  if (view->was_ever_mapped == false) {
    view->was_ever_mapped = true;
    ti::view *v = dynamic_cast<ti::view *>(view);
    wl_list_insert(&v->desktop->wem_views, &v->wem_link);
  }

  view->toplevel_handle = wlr_foreign_toplevel_handle_v1_create(
      view->desktop->foreign_toplevel_manager_v1);

  view->focus();
}

/** Called when the surface is unmapped, and should no longer be shown. */
static void handle_xdg_surface_unmap(struct wl_listener *listener, void *data) {
  ti::view *view = wl_container_of(listener, view, unmap);
  view->mapped = false;
  if (view->toplevel_handle) {
    wlr_foreign_toplevel_handle_v1_destroy(view->toplevel_handle);
    view->toplevel_handle = NULL;
  }
  view->damage_whole();
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
  view->begin_interactive(ti::CURSOR_MOVE, 0);
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
  view->begin_interactive(ti::CURSOR_RESIZE, event->edges);
}

void handle_new_xdg_surface(struct wl_listener *listener, void *data) {
  ti::desktop *desktop = wl_container_of(listener, desktop, new_xdg_surface);
  struct wlr_xdg_surface *xdg_surface =
      reinterpret_cast<struct wlr_xdg_surface *>(data);
  if (xdg_surface->role != WLR_XDG_SURFACE_ROLE_TOPLEVEL) {
    return;
  }

  /* Allocate a ti::view for this surface */
  ti::xdg_view *view = new ti::xdg_view;

  view->desktop = desktop;
  view->xdg_surface = xdg_surface;
  view->surface = xdg_surface->surface;

  // box.x and box.y are the coordinates at which the view is displayed
  wlr_xdg_surface_get_geometry(xdg_surface, &view->box);

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

  view->surface_commit.notify = handle_xdg_surface_commit;
  wl_signal_add(&xdg_surface->surface->events.commit, &view->surface_commit);

  /* cotd */
  struct wlr_xdg_toplevel *toplevel = xdg_surface->toplevel;
  view->request_move.notify = handle_xdg_toplevel_request_move;
  wl_signal_add(&toplevel->events.request_move, &view->request_move);
  view->request_resize.notify = handle_xdg_toplevel_request_resize;
  wl_signal_add(&toplevel->events.request_resize, &view->request_resize);

  /* Add it to the list of views. */
  ti::view *v = dynamic_cast<ti::view *>(view);
  wl_list_insert(&desktop->views, &v->link);
}

std::string ti::xdg_view::get_title() {
  return this->xdg_surface->toplevel->title;
}

void ti::xdg_view::for_each_surface(wlr_surface_iterator_func_t iterator,
                                    void *user_data) {
  wlr_xdg_surface_for_each_surface(xdg_surface, iterator, user_data);
}

void ti::xdg_view::activate() {
  if (xdg_surface->role == WLR_XDG_SURFACE_ROLE_TOPLEVEL) {
    wlr_xdg_toplevel_set_activated(xdg_surface, true);
  }
}
void ti::xdg_view::deactivate() {
  if (xdg_surface->role == WLR_XDG_SURFACE_ROLE_TOPLEVEL) {
    wlr_xdg_toplevel_set_activated(xdg_surface, false);
  }
}
ti::xdg_view::xdg_view() : view(ti::XDG_SHELL_VIEW) {}
ti::xdg_view::~xdg_view() {}