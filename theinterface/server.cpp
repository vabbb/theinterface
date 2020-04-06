#include "server.hpp"

namespace ti {
ti_server &server::get() {
  static ti_server s;
  return s;
}
} // namespace ti