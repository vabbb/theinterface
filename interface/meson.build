interface_sources = files(
  'main.c',
)

interface_deps = [
  pixman,
  wlroots,
  wayland_server,
  xkbcommon,
  cairo,
]

executable(
  meson.project_name(),
  interface_sources,
  include_directories: [ interface_inc ],
  dependencies: interface_deps,
  install: true,
)