
icondir = join_paths(get_option('datadir'), 'hicolor')

# install_data(
#   join_paths(scalable_dir, ('@0@.svg').format(application_id)),
#   install_dir: join_paths(get_option('datadir'), 'icons', scalable_dir)
# )

# symbolic_dir = join_paths('hicolor', 'symbolic', 'apps')
# install_data(
#   join_paths(symbolic_dir, ('@0@-symbolic.svg').format(application_id)),
#   install_dir: join_paths(get_option('datadir'), 'icons', symbolic_dir)
# )

install_data(
  '@0@.svg'.format(app_id),
  install_dir: iconsdir / 'hicolor' / 'scalable' / 'apps'
)

install_data(
  '@0@-symbolic.svg'.format(app_id),
  install_dir: iconsdir / 'hicolor' / 'symbolic' / 'apps',
  rename: '@0@-symbolic.svg'.format(app_id)
)


message(icondir)
message(join_paths(icondir, 'scalable/apps'))

# install_data(
#   '@0@.svg'.format(app_id),
#   install_dir: join_paths(iconsdir, 'scalable/apps'),
# )

# install_data(
#   ('@0@-symbolic.svg').format(app_id),
#   install_dir: join_paths(iconsdir, 'symbolic/apps'),
#   rename: '@0@-symbolic.svg'.format(app_id),
# )