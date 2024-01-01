polyhymnia_sources = [
  'main.c',
  'polyhymnia-album.c',
  'polyhymnia-album-header.c',
  'polyhymnia-album-page.c',
  'polyhymnia-albums-page.c',
  'polyhymnia-artist-page.c',
  'polyhymnia-artists-page.c',
  'polyhymnia-artist.c',
  'polyhymnia-application.c',
  'polyhymnia-audio-format.c',
  'polyhymnia-format-utils.c',
  'polyhymnia-mpd-client.c',
  'polyhymnia-output.c',
  'polyhymnia-player.c',
  'polyhymnia-player-bar.c',
  'polyhymnia-playlist-page.c',
  'polyhymnia-playlists-page.c',
  'polyhymnia-preferences-window.c',
  'polyhymnia-queue-pane.c',
  'polyhymnia-statistics.c',
  'polyhymnia-statistics-window.c',
  'polyhymnia-track.c',
  'polyhymnia-track-details-window.c',
  'polyhymnia-track-full-info.c',
  'polyhymnia-tracks-page.c',
  'polyhymnia-window.c',
]

polyhymnia_enum_headers = [
  'polyhymnia-player-playback-status.h',
]

enums = gnome.mkenums_simple('polyhymnia-enums', sources : polyhymnia_enum_headers)

polyhymnia_deps = [
  dependency('gtk4', version: '>= 4.12'),
  dependency('libadwaita-1', version: '>= 1.4'),
  dependency('libmpdclient', version: '>= 2.20'),
]

executable('polyhymnia', polyhymnia_sources + enums + resources,
  dependencies: polyhymnia_deps,
       install: true,
)
