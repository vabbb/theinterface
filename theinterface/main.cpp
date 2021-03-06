#include <getopt.h>
#include <unistd.h>

extern "C" {
#include <wlr/backend.h>
#include <wlr/backend/interface.h>
#include <wlr/types/wlr_data_device.h>
#include <wlr/util/log.h>
#include <xf86drm.h>
}

#include "cursor.hpp"
#include "server.hpp"
#include "util.hpp"

ti::server *server;

static void ti_atexit() { delete server; }

int main(int argc, char *argv[]) {
  /// using WLR_DEBUG slows the compositor down significantly, so the user needs
  /// to set the TI_DEBUG env var explicitly to have the debug log
  const bool TI_DEBUG = getenv("TI_DEBUG");
  wlr_log_init(TI_DEBUG ? WLR_DEBUG : WLR_INFO, NULL);

  char *startup_cmd = NULL;

  int c;
  while ((c = getopt(argc, argv, "s:h")) != -1) {
    switch (c) {
    case 's':
      startup_cmd = optarg;
      break;
    default:
      std::printf("Usage: %s [-s startup command]\n", argv[0]);
      return 0;
    }
  }
  if (optind < argc) {
    std::printf("Usage: %s [-s startup command]\n", argv[0]);
    return 0;
  }

  // if vmwgfx driver is detected, don't use hardware cursors unless the user
  // asks for it explicitly by exporting WLR_NO_HARDWARE_CURSORS=0
  if (getenv("WLR_NO_HARDWARE_CURSORS") == nullptr) {
    if (possible_no_hardware_cursor_support()) {
      setenv("WLR_NO_HARDWARE_CURSORS", "1", true);
    }
  }

  // Run the constructor for the ti::server class
  server = new ti::server();
  // we will need atexit from now on, for cleanly closing the server
  atexit(ti_atexit);

  if (startup_cmd) {
    if (fork() == 0) {
      execl("/bin/sh", "/bin/sh", "-c", startup_cmd, nullptr);
    }
  }

  /* Run the Wayland event loop. This does not return until you exit the
   * compositor. Starting the backend rigged up all of the necessary event
   * loop configuration to listen to libinput events, DRM events, generate
   * frame events at the refresh rate, and so on. */
  wl_display_run(server->display);

  return EXIT_SUCCESS;
}