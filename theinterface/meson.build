theinterface_sources = files(
  'cursor.cpp',
  'main.cpp',
  'keyboard.cpp',
  'output.cpp',
  'render.cpp',
  'seat.cpp',
  'server.cpp',
  'util.cpp',
  'xdg_shell.cpp',
  'xwayland.cpp',
)

# theinterface_deps = [
#   pixman,
#   wlroots,
#   wayland_server,
#   xkbcommon,
#   cairo,
#   server_protos,
# ]

executable(
  meson.project_name(),
  theinterface_sources,
  include_directories: [ theinterface_inc ],
  dependencies: theinterface_deps,
  install: true,
)