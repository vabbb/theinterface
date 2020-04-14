extern "C" {
#include <wlr/config.h>
#include <wlr/types/wlr_foreign_toplevel_management_v1.h>
#include <wlr/util/log.h>
}

#include "server.hpp"

#include "xwayland.hpp"

#ifdef WLR_HAS_XWAYLAND

static void handle_xwayland_surface_commit(struct wl_listener *listener,
                                           void *data) {
  ti::xwayland_view *view = wl_container_of(listener, view, commit);
  // view_damage_part(view);
}

static void handle_xwayland_surface_map(struct wl_listener *listener,
                                        void *data) {
  ti::xwayland_view *view = wl_container_of(listener, view, map);
  struct wlr_xwayland_surface *xwayland_surface =
      reinterpret_cast<struct wlr_xwayland_surface *>(data);

  if (view->was_ever_mapped == false) {
    view->was_ever_mapped = true;
    ti::xwayland_view *v = dynamic_cast<ti::xwayland_view *>(view);
    wl_list_insert(&view->server->wem_views, &v->wem_link);
  }

  if (xwayland_surface->override_redirect) {
    // This window used not to have the override redirect flag and has it
    // now. Switch to unmanaged.
    handle_xwayland_surface_destroy(&view->destroy, &view->xwayland_surface);
    xwayland_surface->data = NULL;
    /// TODO: create unmanaged view and map it
    return;
  }

  view->surface = xwayland_surface->surface;
  view->pid = xwayland_surface->pid;
  view->mapped = true;

  if (view->xwayland_surface->decorations ==
      WLR_XWAYLAND_SURFACE_DECORATIONS_ALL) {
    view->decorated = true;
    view->border_width = 4;
    view->titlebar_height = 12;
  }

  view->toplevel_handle = wlr_foreign_toplevel_handle_v1_create(
      view->server->foreign_toplevel_manager_v1);

  wlr_foreign_toplevel_handle_v1_set_title(
      view->toplevel_handle, view->xwayland_surface->title ?: "none");
  wlr_foreign_toplevel_handle_v1_set_app_id(
      view->toplevel_handle, view->xwayland_surface->c_class ?: "none");

  view->commit.notify = handle_xwayland_surface_commit;
  wl_signal_add(&view->xwayland_surface->surface->events.commit, &view->commit);

  if (wlr_xwayland_or_surface_wants_focus(view->xwayland_surface)) {
    view->focus(view->surface);
  }
}

/** Called when the surface is unmapped, and should no longer be shown. */
static void handle_xwayland_surface_unmap(struct wl_listener *listener,
                                          void *data) {
  ti::xwayland_view *view = wl_container_of(listener, view, unmap);
  view->mapped = false;
  if (view->toplevel_handle) {
    wlr_foreign_toplevel_handle_v1_destroy(view->toplevel_handle);
    view->toplevel_handle = NULL;
  }
}

/** Called when the surface is destroyed and should never be shown again. */
void handle_xwayland_surface_destroy(struct wl_listener *listener, void *data) {
  ti::xwayland_view *view = wl_container_of(listener, view, destroy);

  // if view is mapped, we call handle_xwayland_surface_unmap first
  if (view->mapped) {
    handle_xwayland_surface_unmap(&view->unmap, &view->xwayland_surface);
  }

  wl_list_remove(&view->link);

  if (view->was_ever_mapped) {
    wl_list_remove(&view->wem_link);
  }

  ti::xwayland_view *v = dynamic_cast<ti::xwayland_view *>(view);
  delete v;
}

/** called on title change */
static void handle_set_title(struct wl_listener *listener, void *data) {
  // do nothing...?
  // ti::xwayland_view *view = wl_container_of(listener, view, destroy);
}

static void handle_request_configure(struct wl_listener *listener, void *data) {
  ti::xwayland_view *view = wl_container_of(listener, view, request_configure);
  struct wlr_xwayland_surface_configure_event *event =
      reinterpret_cast<struct wlr_xwayland_surface_configure_event *>(data);

  wlr_xwayland_surface_configure(view->xwayland_surface, event->x, event->y,
                                 event->width, event->height);
}

void handle_new_xwayland_surface(struct wl_listener *listener, void *data) {
  ti::server *server = wl_container_of(listener, server, new_xwayland_surface);
  struct wlr_xwayland_surface *xwayland_surface =
      reinterpret_cast<struct wlr_xwayland_surface *>(data);

  if (xwayland_surface->override_redirect) {
    wlr_log(WLR_DEBUG, "New xwayland unmanaged surface");
    /// TODO: something
    return;
  }

  wlr_log(WLR_DEBUG, "New xwayland surface title='%s' class='%s'",
          xwayland_surface->title, xwayland_surface->c_class);

  /* Allocate a ti::view for this surface */
  ti::xwayland_view *view = new ti::xwayland_view;
  view->server = server;
  view->xwayland_surface = xwayland_surface;

  /* Listen to the various events it can emit */
  view->map.notify = handle_xwayland_surface_map;
  wl_signal_add(&xwayland_surface->events.map, &view->map);
  view->unmap.notify = handle_xwayland_surface_unmap;
  wl_signal_add(&xwayland_surface->events.unmap, &view->unmap);
  view->destroy.notify = handle_xwayland_surface_destroy;
  wl_signal_add(&xwayland_surface->events.destroy, &view->destroy);

  view->set_title.notify = handle_set_title;
  wl_signal_add(&xwayland_surface->events.set_title, &view->set_title);
  view->request_configure.notify = handle_request_configure;
  wl_signal_add(&xwayland_surface->events.request_configure,
                &view->request_configure);

  /* cotd */
  //   struct wlr_xdg_toplevel *toplevel = xwayland_surface->toplevel;
  //   view->request_move.notify = xdg_toplevel_request_move;
  //   wl_signal_add(&toplevel->events.request_move, &view->request_move);
  //   view->request_resize.notify = xdg_toplevel_request_resize;
  //   wl_signal_add(&toplevel->events.request_resize, &view->request_resize);

  /* Add it to the list of views. */
  ti::view *v = dynamic_cast<ti::view *>(view);
  wl_list_insert(&server->views, &v->link);
}

std::string ti::xwayland_view::get_title() {
  return this->xwayland_surface->title;
}

struct wlr_surface *ti::xwayland_view::get_wlr_surface() {
  return xwayland_surface->surface;
}

ti::xwayland_view::xwayland_view() : view(ti::XWAYLAND_VIEW) {}
ti::xwayland_view::~xwayland_view() {}
#endif