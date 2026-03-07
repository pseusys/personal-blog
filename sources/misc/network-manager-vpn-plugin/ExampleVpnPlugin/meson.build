project('nm-examplevpn-plugin', 'c', version : '0.0.4', default_options : ['prefix=/usr', 'warning_level=everything', 'werror=true', 'buildtype=debug'])

fs = import('fs')
gnome = import('gnome')

cc = meson.get_compiler('c')
pkgconfig = find_program('pkg-config')
clang_format = find_program('clang-format')
clang_tidy = find_program('clang-tidy')

glib_dep = dependency('glib-2.0', include_type: 'system')
libnm_dep = dependency('libnm', include_type: 'system')
gtk3_dep = dependency('gtk+-3.0', include_type: 'system')
gtk4_dep = dependency('gtk4', include_type: 'system')
libsecret_dep = dependency('libsecret-1', include_type: 'system')


# Prepare C building flags:

base_flags = ['-Wno-padded', '-Wno-c++-compat']
if get_option('buildtype') == 'debug'
  base_flags += '-Og'
endif

add_project_arguments(base_flags, language : 'c')


# Prepare code definitions, names and paths:

plugin_name = 'examplevpn'
service_name = 'org.freedesktop.NetworkManager.' + plugin_name
service_path = '/org/freedesktop/NetworkManager/' + plugin_name

nm_libdir = get_option('libdir') / 'NetworkManager'
plugin_interface_base = get_option('prefix') / nm_libdir / 'libnm-vpn-editor-@0@'.format(plugin_name)

extra_flags = [
  '-DEXAMPLEVPN_PLUGIN_PATH="@0@"'.format(service_path),
  '-DEXAMPLEVPN_PLUGIN_SERVICE="@0@"'.format(service_name),
  '-DEDITOR_INTERFACE_PATH="@0@"'.format(plugin_interface_base)
]


# Compile and create static library for resources:

gresources = gnome.compile_resources(
  'resources',
  'res/gresource.xml',
  source_dir : 'res',
  c_name : 'resources'
)

greslib = static_library(
  'gresources',
  gresources,
  dependencies : [glib_dep],
  override_options : ['warning_level=0']
)


# Compile auth-dialog executable:

auth_dialog = executable('nm-@0@-auth-dialog'.format(plugin_name),
  'src/auth_dialog.c',
  dependencies : [libnm_dep, libsecret_dep],
  install : true,
  install_dir : get_option('libexecdir'),
  install_mode : 'rwxr-xr-x'
)


# Compile editor shared library:

editor = shared_library('nm-vpn-plugin-@0@'.format(plugin_name),
  'src/editor.c',
  include_directories : include_directories('src'),
  dependencies : [glib_dep, libnm_dep],
  c_args : extra_flags,
  install : true,
  install_dir : nm_libdir,
  install_mode : 'rw-r--r--'
)


# Compile editor GUI for GTK3 shared library:

interface_gtk3 = shared_library('nm-vpn-editor-@0@-gtk3'.format(plugin_name),
  'src/interface_gtk3.c',
  include_directories : include_directories('src'),
  dependencies : [glib_dep, libnm_dep, gtk3_dep],
  link_with : [greslib],
  c_args : extra_flags,
  install : true,
  install_dir : nm_libdir,
  install_mode : 'rw-r--r--'
)


# Compile editor GUI for GTK4 shared library:

interface_gtk4 = shared_library('nm-vpn-editor-@0@-gtk4'.format(plugin_name),
  'src/interface_gtk4.c',
  include_directories : include_directories('src'),
  dependencies : [glib_dep, libnm_dep, gtk4_dep],
  link_with : [greslib],
  c_args : extra_flags,
  install : true,
  install_dir : nm_libdir,
  install_mode : 'rw-r--r--'
)


# Compile VPN core stub shared library:

vpn_core = shared_library('@0@-core'.format(plugin_name),
  'plugin/examplevpn_core.c',
  include_directories : include_directories('plugin'),
  override_options : ['warning_level=2'],
  install : true,
  install_dir : nm_libdir,
  install_mode : 'rw-r--r--'
)


# Compile D-Bus plugin executable:

plugin = executable('nm-@0@-service'.format(plugin_name),
  'src/plugin.c',
  include_directories : include_directories('src', 'plugin'),
  dependencies : [glib_dep, libnm_dep],
  c_args : extra_flags,
  install : true,
  install_dir : get_option('libexecdir'),
  install_mode : 'rwxr-xr-x'
)


# Prepare service descriptor file for installation:

conf_data = configuration_data()
conf_data.set('examplevpn-plugin-name', plugin_name)
conf_data.set('dbus-service-name', service_name)
conf_data.set('path-to-vpn-plugin-service', get_option('prefix') / get_option('libexecdir') / 'nm-@0@-service'.format(plugin_name))
conf_data.set('path-to-vpn-editor-plugin', get_option('prefix') / nm_libdir / 'libnm-vpn-plugin-@0@.so'.format(plugin_name))
conf_data.set('path-to-auth-dialog', get_option('prefix') / get_option('libexecdir') / 'nm-@0@-auth-dialog'.format(plugin_name))

configure_file(
  input : 'conf/descriptor.ini',
  output : 'nm-@0@-service.name'.format(plugin_name),
  configuration : conf_data,
  install : true,
  install_dir : get_option('prefix') / 'lib' / 'NetworkManager' / 'VPN',
  install_mode : 'rw-r--r--'
)


# Prepare service configuration file for installation:

configure_file(
  input : 'conf/service.conf',
  output : 'nm-@0@-service.conf'.format(plugin_name),
  configuration : conf_data,
  install : true,
  install_dir : get_option('datadir') / 'dbus-1' / 'system.d',
  install_mode : 'rw-r--r--'
)
