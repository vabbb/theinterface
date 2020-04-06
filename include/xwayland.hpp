#ifndef TI_XWAYLAND_HPP
#define TI_XWAYLAND_HPP

// these need to be imported before <wlr/xwayland.h> otherwise nothing works
#include <cmath>
#include <xcb/xcb.h>

extern "C" {
#define static
#define class c_class
#include <wlr/xwayland.h>
#undef class
#undef static
}

#endif