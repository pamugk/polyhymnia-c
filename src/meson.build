polyhymnia_sources = [
  'main.c',
  'polyhymnia-album-card.c',
  'polyhymnia-application.c',
  'polyhymnia-artist-card.c',
  'polyhymnia-genre-card.c',
  'polyhymnia-mpd-client.c',
  'polyhymnia-preferences-window.c',
  'polyhymnia-track-row.c',
  'polyhymnia-window.c',
]

polyhymnia_deps = [
  dependency('gtk4'),
  dependency('libadwaita-1', version: '>= 1.3'),
  dependency('libmpdclient', version: '>= 2.20'),
]

executable('polyhymnia', polyhymnia_sources + resources,
  dependencies: polyhymnia_deps,
       install: true,
)
