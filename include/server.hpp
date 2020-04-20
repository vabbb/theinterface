#ifndef TI_SERVER_HPP
#define TI_SERVER_HPP

extern "C" {
#include <wlr/backend.h>
#include <wlr/types/wlr_data_device.h>
}
namespace ti {

class view;
class desktop;

class server {
public:
  ti::desktop *desktop;
  struct wl_display *display;
  struct wlr_backend *backend;
  struct wlr_renderer *renderer;

  struct wlr_data_device_manager *data_device_manager;

  server();
  ~server();
};
} // namespace ti

#endif