conf = configuration_data()
conf.set('APP_NAME', app_name)
conf.set('APP_ID', app_id)
conf.set('DEFAULT_RESOURCE_PATH', default_resource_path)
conf.set('APP_DISPLAY_NAME', app_display_name)
conf.set('APP_MIME_TYPE', app_mime_type)
conf.set('APP_CATEGORIES', app_categories)

gresource_file_in = configure_file(
  input: '@0@.gresource.xml.in'.format(app_id),
  output: '@0@.gresource.xml'.format(app_id),
  configuration: conf,
)

sources += gnome.compile_resources(
  'app-resources',
  gresource_file_in,
)

desktop_file_in = configure_file(
  input: '@0@.desktop.in'.format(app_id),
  output: '@0@.desktop.in'.format(app_id),
  configuration: conf,
)

desktop_file = i18n.merge_file(
  input: desktop_file_in,
  output: '@0@.desktop'.format(app_id),
  type: 'desktop',
  po_dir: '../po',
  install: true,
  install_dir: join_paths(get_option('datadir'), 'applications'),
)

desktop_utils = find_program('desktop-file-validate', required: true)
if desktop_utils.found()
  test('Validate desktop file', desktop_utils, args: [desktop_file.full_path()])
endif

appstream_file_in = configure_file(
  input: '@0@.appdata.xml.in'.format(app_id),
  output: '@0@.appdata.xml.in'.format(app_id),
  configuration: conf,
)

appstream_file = i18n.merge_file(
  input: appstream_file_in,
  output: '@0@.appdata.xml'.format(app_id),
  po_dir: '../po',
  install: true,
  install_dir: join_paths(get_option('datadir'), 'appdata'),
)

appstream_util = find_program('appstream-util', required: false)
if appstream_util.found()
  test(
    'Validate appstream file',
    appstream_util,
    args: ['validate', appstream_file],
  )
endif

gschema_file_in = configure_file(
  input: '@0@.gschema.xml.in'.format(app_id),
  output: '@0@.gschema.xml'.format(app_id),
  configuration: conf,
)

install_data(
  gschema_file_in,
  install_dir: join_paths(get_option('datadir'), 'glib-2.0/schemas'),
)

compile_schemas = find_program('glib-compile-schemas', required: false)
if compile_schemas.found()
  test(
    'Validate schema file',
    compile_schemas,
    args: ['--strict', '--dry-run', meson.current_source_dir()],
  )
endif

subdir('icons')
