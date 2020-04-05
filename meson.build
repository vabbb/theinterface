project(
  'theinterface', 
  'c',
  version : '0.0.1',
  license: 'Apache-2.0',
  default_options: [
    'c_std=c18',
    'warning_level=2',
    'werror=true',
    'optimization=2'
  ],
)

add_project_arguments([
    '-Wno-unused-parameter',
    '-Wno-unused-variable',
    '-DWLR_USE_UNSTABLE',
    '-DWL_HIDE_DEPRECATED' # Avoid wl_buffer deprecation warnings
  ],
  language: 'c',
)

cc = meson.get_compiler('c')
# Clang complains about some zeroed initializer lists (= {0}), even though they
# are valid
if cc.get_id() == 'clang'
	add_project_arguments('-Wno-missing-field-initializers', language: 'c')
	add_project_arguments('-Wno-missing-braces', language: 'c')
endif

wlroots_version = ['>=0.10.0', '<0.11.0']
wlroots = dependency('wlroots', version: wlroots_version)
wlroots_has_xwayland = cc.get_define('WLR_HAS_XWAYLAND', prefix: '#include <wlr/config.h>', dependencies: wlroots) == '1'

theinterface_inc = include_directories('include')

pixman         = dependency('pixman-1')
wayland_server = dependency('wayland-server', version: '>=1.18')
xkbcommon      = dependency('xkbcommon')
libdrm         = dependency('libdrm')
cairo          = dependency('cairo')

theinterface_deps = [
  pixman,
  wlroots,
  wayland_server,
]

wayland_protos = dependency('wayland-protocols', version: '>=1.17')
subdir('protocol')

# protos = library(
# 	meson.project_name(), proto_files,
# 	# soversion: soversion,
# 	# dependencies: wlr_deps,
# 	include_directories: [proto_inc],
# 	install: true,
# 	# link_args : symbols_flag,
# 	# link_depends: symbols_file,
# )

subdir('theinterface')
