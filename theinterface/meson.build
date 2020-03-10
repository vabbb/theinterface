theinterface_sources = files(
  'main.c',
)

theinterface_deps = [
  pixman,
  wlroots,
  wayland_server,
  xkbcommon,
  cairo,
]

executable(
  meson.project_name(),
  theinterface_sources,
  include_directories: [ theinterface_inc ],
  dependencies: theinterface_deps,
  install: true,
)