#include <csignal>
#include <cstdlib>

extern "C" {
#include <wlr/backend/multi.h>
#include <wlr/types/wlr_keyboard.h>
#include <wlr/util/log.h>
}

#include "keyboard.hpp"
#include "xdg_shell.hpp"

#include "server.hpp"

static void keyboard_handle_modifiers(struct wl_listener *listener,
                                      void *data) {
  /* This event is raised when a modifier key, such as shift or alt, is
   * pressed. We simply communicate this to the client. */
  ti::keyboard *keyboard = wl_container_of(listener, keyboard, modifiers);
  /*
   * A seat can only have one keyboard, but this is a limitation of the
   * Wayland protocol - not wlroots. We assign all connected keyboards to the
   * same seat. You can swap out the underlying wlr_keyboard like this and
   * wlr_seat handles this transparently.
   */
  wlr_seat_set_keyboard(keyboard->server->seat, keyboard->device);
  /* Send modifiers to the client. */
  wlr_seat_keyboard_notify_modifiers(keyboard->server->seat,
                                     &keyboard->device->keyboard->modifiers);
}

/** Change virtual terminal to the one specified by keysym
 * called by using ctrl+alt+fn1..12 keys */
static bool ti_chvt(ti::server *server, uint32_t keysym) {
  if (wlr_backend_is_multi(server->backend)) {
    struct wlr_session *session = wlr_backend_get_session(server->backend);
    if (session) {
      unsigned vt = keysym - XKB_KEY_XF86Switch_VT_1 + 1;
      wlr_session_change_vt(session, vt);
      return true;
    }
  }
  return false;
}

/** Start the alt+tab handler */
static bool ti_alt_tab(ti::server *server, uint32_t keysym) {
  /* Cycle to the next view */
  if (wl_list_length(&server->views) < 2) {
    return false;
  }
  ti::xdg_view *current_view =
      wl_container_of(server->views.next, current_view, link);
  ti::xdg_view *next_view =
      wl_container_of(current_view->link.next, next_view, link);
  next_view->focus(next_view->xdg_surface->surface);
  /* Move the previous view to the end of the list */
  wl_list_remove(&current_view->link);
  wl_list_insert(server->views.prev, &current_view->link);
  return true;
}

/** alt+f4 handler */
static bool ti_alt_f4(ti::server *server, uint32_t keysym) {
  ti::xdg_view *current_view =
      wl_container_of(server->views.next, current_view, link);
  kill(current_view->pid, SIGKILL);
  return true;
}

/*
 * Here we handle compositor keybindings. This is when the compositor is
 * processing keys, rather than passing them on to the client for its own
 * processing.
 */
static bool handle_keybinding(ti::server *server, const xkb_keysym_t *syms,
                              uint32_t modifiers, size_t syms_len) {
  xkb_keysym_t keysym;
  switch (modifiers) {
  case WLR_MODIFIER_LOGO: {
    for (size_t i = 0; i < syms_len; ++i) {
      keysym = syms[i];
      switch (keysym) {
      case XKB_KEY_Escape: {
        // this will terminate wl_display_run, causing it to return
        wl_display_terminate(server->display);
        return true;
      }
      }
    }
    break;
  }
  case (WLR_MODIFIER_ALT | WLR_MODIFIER_CTRL): {
    for (size_t i = 0; i < syms_len; ++i) {
      keysym = syms[i];
      switch (keysym) {
      /* These are specially mapped keys, each of them composed by
       * CTRL+ALT+fnkey1..12*/
      case XKB_KEY_XF86Switch_VT_1 ... XKB_KEY_XF86Switch_VT_12: {
        ti_chvt(server, keysym);
        return true;
      }
      }
    }
    break;
  }
  case WLR_MODIFIER_ALT: {
    for (size_t i = 0; i < syms_len; ++i) {
      keysym = syms[i];
      switch (keysym) {
      case XKB_KEY_Tab: {
        ti_alt_tab(server, keysym);
        return true;
      }
      case XKB_KEY_F4: {
        ti_alt_f4(server, keysym);
        return true;
      }
      }
    }
    break;
  }
  }
  return false;
}

static void keyboard_handle_key(struct wl_listener *listener, void *data) {
  /* This event is raised when a key is pressed or released. */
  ti::keyboard *keyboard = wl_container_of(listener, keyboard, key);
  ti::server *server = keyboard->server;
  struct wlr_event_keyboard_key *event =
      reinterpret_cast<struct wlr_event_keyboard_key *>(data);
  struct wlr_seat *seat = server->seat;

  /* Translate libinput keycode -> xkbcommon */
  uint32_t keycode = event->keycode + 8;
  /* Get a list of keysyms based on the keymap for this keyboard */
  const xkb_keysym_t *syms;
  int nsyms = xkb_state_key_get_syms(keyboard->device->keyboard->xkb_state,
                                     keycode, &syms);

  bool handled = false;
  uint32_t modifiers = wlr_keyboard_get_modifiers(keyboard->device->keyboard);
  if (event->state == WLR_KEY_PRESSED) {
    /* If a key was _pressed_, we attempt to
     * process it as a compositor keybinding. */
    handled = handle_keybinding(server, syms, modifiers, nsyms);
  }

  if (!handled) {
    /* Otherwise, we pass it along to the client. */
    wlr_seat_set_keyboard(seat, keyboard->device);
    wlr_seat_keyboard_notify_key(seat, event->time_msec, event->keycode,
                                 event->state);
  }
}

void ti::server::new_keyboard(struct wlr_input_device *device) {
  ti::keyboard *keyboard = new ti::keyboard;
  keyboard->server = this;
  keyboard->device = device;

  /* We need to prepare an XKB keymap and assign it to the keyboard. This
   * assumes the defaults (e.g. layout = "us"). */
  struct xkb_rule_names rules = {.rules = nullptr,
                                 .model = nullptr,
                                 .layout = nullptr,
                                 .variant = nullptr,
                                 .options = nullptr};
  struct xkb_context *context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
  struct xkb_keymap *keymap =
      xkb_map_new_from_names(context, &rules, XKB_KEYMAP_COMPILE_NO_FLAGS);

  wlr_keyboard_set_keymap(device->keyboard, keymap);
  xkb_keymap_unref(keymap);
  xkb_context_unref(context);
  wlr_keyboard_set_repeat_info(device->keyboard, 25, 600);

  /* Here we set up listeners for keyboard events. */
  keyboard->modifiers.notify = keyboard_handle_modifiers;
  wl_signal_add(&device->keyboard->events.modifiers, &keyboard->modifiers);
  keyboard->key.notify = keyboard_handle_key;
  wl_signal_add(&device->keyboard->events.key, &keyboard->key);

  wlr_seat_set_keyboard(this->seat, device);

  /* And add the keyboard to our list of keyboards */
  wl_list_insert(&this->keyboards, &keyboard->link);
}
