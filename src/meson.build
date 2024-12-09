sources += files(
  'lol_application.c',
  'lol_window.c',
  'main.c',
)

deps = [
  dependency('gtk4'),
  dependency('libadwaita-1'),
  # dependency('pistachio_text_editor-0'),
  dependency('gtksourceview-5'),
]

executable(
  app_name,
  sources: sources,
  include_directories: inc,
  dependencies: deps,
  install: true,
)
