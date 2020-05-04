extern "C" {
#include <wlr/util/log.h>
}
#include "desktop.hpp"
#include "layer_shell.hpp"
#include "output.hpp"
#include "seat.hpp"

static void handle_layer_surface_commit(struct wl_listener *listener,
                                        void *data) {
  ti::layer_view *layer = wl_container_of(listener, layer, surface_commit);
  struct wlr_layer_surface_v1 *layer_surface = layer->layer_surface;
  struct wlr_output *wlr_output = layer_surface->output;
  if (wlr_output != NULL) {
    auto output = reinterpret_cast<ti::output *>(wlr_output->data);
    struct wlr_box old_geo = layer->geo;
    // arrange_layers(output);

    // Cursor changes which happen as a consequence of resizing a layer
    // surface are applied in arrange_layers. Because the resize happens
    // before the underlying surface changes, it will only receive a cursor
    // update if the new cursor position crosses the *old* sized surface in
    // the *new* layer surface.
    // Another cursor move event is needed when the surface actually
    // changes.
    struct wlr_surface *surface = layer_surface->surface;
    if (surface->previous.width != surface->current.width ||
        surface->previous.height != surface->current.height) {
      //   update_cursors(layer, &output->desktop->server->input->seats);
    }

    bool geo_changed =
        memcmp(&old_geo, &layer->geo, sizeof(struct wlr_box)) != 0;
    bool layer_changed = layer->layer != layer_surface->current.layer;
    if (layer_changed) {
      wl_list_remove(&layer->link);
      wl_list_insert(&output->layers[layer_surface->current.layer],
                     &layer->link);
      layer->layer = layer_surface->current.layer;
    }
    if (geo_changed || layer_changed) {
      //   output_damage_whole_local_surface(output, layer_surface->surface,
      //                                     old_geo.x, old_geo.y);
      //   output_damage_whole_local_surface(output, layer_surface->surface,
      //                                     layer->geo.x, layer->geo.y);
    } else {
      //   output_damage_from_local_surface(output, layer_surface->surface,
      //                                    layer->geo.x, layer->geo.y);
    }
  }
}

void handle_new_layer_shell_surface(struct wl_listener *listener, void *data) {
  auto layer_surface = reinterpret_cast<struct wlr_layer_surface_v1 *>(data);
  ti::desktop *desktop =
      wl_container_of(listener, desktop, new_layer_shell_surface);

  wlr_log(WLR_DEBUG,
          "new layer surface: namespace %s layer %d anchor %d "
          "size %dx%d margin %d,%d,%d,%d",
          layer_surface->c_namespace, layer_surface->client_pending.layer,
          layer_surface->client_pending.anchor,
          layer_surface->client_pending.desired_width,
          layer_surface->client_pending.desired_height,
          layer_surface->client_pending.margin.top,
          layer_surface->client_pending.margin.right,
          layer_surface->client_pending.margin.bottom,
          layer_surface->client_pending.margin.left);

  if (!layer_surface->output) {
    // struct roots_input *input = desktop->server->input;
    // struct roots_seat *seat = input_last_active_seat(input);
    // assert(seat); // Technically speaking we should handle this case
    ti::seat *seat = desktop->seat;
    struct wlr_output *output = wlr_output_layout_output_at(
        desktop->output_layout, seat->cursor->x, seat->cursor->y);
    if (!output) {
      wlr_log(WLR_ERROR, "Couldn't find output at (%.0f,%.0f)", seat->cursor->x,
              seat->cursor->y);
      output = wlr_output_layout_get_center_output(desktop->output_layout);
    }
    if (output) {
      layer_surface->output = output;
    } else {
      wlr_layer_surface_v1_close(layer_surface);
      return;
    }
  }

  ti::layer_view *layer_view = new ti::layer_view();
  if (!layer_view) {
    return;
  }

  layer_view->surface_commit.notify = handle_layer_surface_commit;
  wl_signal_add(&layer_surface->surface->events.commit,
                &layer_view->surface_commit);

  //   layer_view->output_destroy.notify = handle_output_destroy;
  //   wl_signal_add(&layer_surface->output->events.destroy,
  //                 &layer_view->output_destroy);

  //   layer_view->destroy.notify = handle_layer_destroy;
  //   wl_signal_add(&layer_surface->events.destroy, &layer_view->destroy);
  //   layer_view->map.notify = handle_layer_map;
  //   wl_signal_add(&layer_surface->events.map, &layer_view->map);
  //   layer_view->unmap.notify = handle_layer_unmap;
  //   wl_signal_add(&layer_surface->events.unmap, &layer_view->unmap);
  //   layer_view->new_popup.notify = handle_new_layer_popup;
  //   wl_signal_add(&layer_surface->events.new_popup, &layer_view->new_popup);
  // TODO: Listen for subsurfaces

  layer_view->layer_surface = layer_surface;
  layer_surface->data = layer_view;

  auto output = reinterpret_cast<ti::output *>(layer_surface->output->data);
  wl_list_insert(&output->layers[layer_surface->current.layer],
                 &layer_view->link);

  // Temporarily set the layer's current state to client_pending
  // So that we can easily arrange it
  struct wlr_layer_surface_v1_state old_state = layer_surface->current;
  layer_surface->current = layer_surface->client_pending;

  //   arrange_layers(output);

  layer_surface->current = old_state;
}

ti::layer_view::layer_view() {}
ti::layer_view::~layer_view() {}

ti::layer_popup::layer_popup() {}
ti::layer_popup::~layer_popup() {}