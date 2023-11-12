polyhymnia_sources = [
  'main.c',
  'polyhymnia-album.c',
  'polyhymnia-artist.c',
  'polyhymnia-application.c',
  'polyhymnia-format-utils.c',
  'polyhymnia-mpd-client.c',
  'polyhymnia-player.c',
  'polyhymnia-player-bar.c',
  'polyhymnia-preferences-window.c',
  'polyhymnia-queue-pane.c',
  'polyhymnia-track.c',
  'polyhymnia-window.c',
]

polyhymnia_enum_headers = [
  'polyhymnia-player-playback-status.h',
]

enums = gnome.mkenums_simple('polyhymnia-enums', sources : polyhymnia_enum_headers)

polyhymnia_deps = [
  dependency('gtk4'),
  dependency('libadwaita-1', version: '>= 1.4'),
  dependency('libmpdclient', version: '>= 2.20'),
]

executable('polyhymnia', polyhymnia_sources + enums + resources,
  dependencies: polyhymnia_deps,
       install: true,
)
