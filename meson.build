project(
  'test_app',
  'c',
  version: '0.1.1.alpha',
  meson_version: '>= 0.64.0',
  default_options: [
    'warning_level=2',
    'werror=false',
    'c_std=gnu11',
  ],
)

cflags = [
  '-Wno-unused-parameter',
  # '-Wno-unused-variable',
  '-Wno-unused-function',
  '-Wno-unused-result',
  # '-Wno-missing-field-initializers',
]

cc = meson.get_compiler('c')
add_project_arguments(cflags, language: 'c')

i18n = import('i18n')
gnome = import('gnome')

app_name = meson.project_name()
app_display_name = 'Test App'

app_categories = 'Utility;TextEditor;'
app_mime_type = 'text/plain;application/x-zerosize;'
developer = 'robertomorrison'
app_id = 'org.' + developer + '.' + app_name
default_resource_path = '/org/' + developer + '/' + app_name
developer_name = 'Roberto Morrison'

website = join_paths('https://github.com/robertomorrison0', app_name)

prefix = get_option('prefix')

datadir = get_option('datadir') #join_paths(prefix, get_option('datadir'))
iconsdir = join_paths(datadir, 'icons')

package_version = meson.project_version()

config_h = configuration_data()
config_h.set_quoted('GETTEXT_PACKAGE', app_id)
config_h.set_quoted('LOCALEDIR', join_paths(prefix, get_option('localedir')))
config_h.set_quoted('APP_ID', app_id)
config_h.set_quoted('APP_DISPLAY_NAME', app_display_name)
config_h.set_quoted('APP_NAME', app_name)
config_h.set_quoted('WEBSITE', website)
config_h.set_quoted('DEFAULT_RESOURCE_PATH', default_resource_path)
config_h.set_quoted('PACKAGE_ICON_NAME', app_id)
config_h.set_quoted('PACKAGE_VERSION', meson.project_version())
config_h.set_quoted('DEVELOPER', developer)
config_h.set_quoted('DEVELOPER_NAME', developer_name)
config_h.set_quoted('DATA_DIR', datadir)
configure_file(output: 'config.h', configuration: config_h)

inc = include_directories('.', 'src')

sources = []

subdir('data')
subdir('src')
