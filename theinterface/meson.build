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

executable(
  meson.project_name(),
  theinterface_sources,
  include_directories: [ theinterface_inc ],
  dependencies: theinterface_deps,
  # cpp_args: '-fopenmp',
  install: true,
)