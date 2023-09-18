polyhymnia_sources = [
  'main.c',
  'polyhymnia-application.c',
  'polyhymnia-window.c',
  'preferences-window.c',
]

polyhymnia_deps = [
  dependency('gtk4'),
  dependency('libadwaita-1', version: '>= 1.3'),
]

executable('polyhymnia', polyhymnia_sources + resources,
  dependencies: polyhymnia_deps,
       install: true,
)
