project(
  'theinterface', 
  'c', 'cpp',
  version : '0.0.1',
  license: 'Apache-2.0',
  default_options: [
    'c_std=c18',
    'cpp_std=c++2a',
    'warning_level=2',
    'werror=true',
    'b_lto=true',
    'b_pie=true',
    'optimization=2',
    'debug=false',
  ],
)

add_project_arguments([
    '-Wno-unused-parameter',
    '-Wno-unused-variable',
    '-DWLR_USE_UNSTABLE',
    '-DWL_HIDE_DEPRECATED', # Avoid wl_buffer deprecation warnings
    ],
  language: ['c', 'cpp'],
)

cppc = meson.get_compiler('cpp')
cc = meson.get_compiler('c')
# Clang complains about some zeroed initializer lists (= {0}), even though they
# are valid
if cc.get_id() == 'clang'
	add_project_arguments('-Wno-missing-field-initializers', language: 'c')
	add_project_arguments('-Wno-missing-braces', language: 'c')
endif

theinterface_inc = include_directories('include')

cairo          = dependency('cairo')
libdrm         = dependency('libdrm')
libinput       = dependency('libinput', version: '>=1.7.0')
libgomp        = cppc.find_library('gomp')
pixman         = dependency('pixman-1')
udev           = dependency('libudev')
wayland_server = dependency('wayland-server', version: '>=1.18')
wayland_protos = dependency('wayland-protocols', version: '>=1.20')
xkbcommon      = dependency('xkbcommon')

wlroots_version = ['>=0.10.0', '<0.11.0']
wlroots = dependency('wlroots', version: wlroots_version)
wlroots_has_xwayland = cc.get_define('WLR_HAS_XWAYLAND', prefix: '#include <wlr/config.h>', dependencies: wlroots) == '1'
wlroots_has_xwayland = cppc.get_define('WLR_HAS_XWAYLAND', prefix: '#include <wlr/config.h>', dependencies: wlroots) == '1'

#build the wayland protocols into a static library
subdir('protocol')

theinterface_deps = [
  pixman,
  wlroots,
  wayland_server,
  xkbcommon,
  server_protos, # this is declared inside protocol/build.meson
  libdrm,
  udev,
  # libgomp
]
subdir('theinterface')
