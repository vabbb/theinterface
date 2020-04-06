#ifndef TI_WLROOTS_HPP
#define TI_WLROOTS_HPP

// these need to be imported before <wlr/xwayland.h> otherwise nothing works
#include <cmath>
#include <xcb/xcb.h>

extern "C" {
#include <wayland-server-core.h>
#include <wayland-util.h>

#include <wlr/backend.h>
#include <wlr/types/wlr_data_device.h>
#include <wlr/types/wlr_keyboard.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_xcursor_manager.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/util/log.h>
#include <xkbcommon/xkbcommon.h> // unused?

#define static
#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_matrix.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_output_layout.h>

#define class c_class
#include <wlr/xwayland.h>
#undef class
#undef static
}

#endif