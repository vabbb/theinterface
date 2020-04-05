theinterface_sources = files(
  'main.c',
  'xdg_shell.c',
  'keyboard.c',
)

theinterface_deps = [
  pixman,
  wlroots,
  wayland_server,
  xkbcommon,
  cairo,
  server_protos,
]

executable(
  meson.project_name(),
  theinterface_sources,
  include_directories: [ theinterface_inc ],
  dependencies: theinterface_deps,
  install: true,
)