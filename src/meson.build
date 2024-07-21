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
  'polyhymnia-current-lyrics-pane.c',
  'polyhymnia-format-utils.c',
  'polyhymnia-last-modified-page.c',
  'polyhymnia-lyrics-provider.c',
  'polyhymnia-mpd-client.c',
  'polyhymnia-output.c',
  'polyhymnia-player.c',
  'polyhymnia-player-bar.c',
  'polyhymnia-playlist-page.c',
  'polyhymnia-playlists-page.c',
  'polyhymnia-preferences-dialog.c',
  'polyhymnia-queue-pane.c',
  'polyhymnia-statistics.c',
  'polyhymnia-search-page.c',
  'polyhymnia-statistics-dialog.c',
  'polyhymnia-track.c',
  'polyhymnia-track-details-dialog.c',
  'polyhymnia-track-full-info.c',
  'polyhymnia-tracks-page.c',
  'polyhymnia-window.c',
]

polyhymnia_enum_headers = [
  'polyhymnia-player-playback-status.h',
]

enums = gnome.mkenums_simple('polyhymnia-enums', sources : polyhymnia_enum_headers)

polyhymnia_deps = [
  dependency('gtk4', version: '>= 4.11.5'),
  dependency('json-glib-1.0', version: '>= 1.2'),
  dependency('libadwaita-1', version: '>= 1.5'),
  dependency('libmpdclient', version: '>= 2.20'),
  dependency('libsoup-3.0'),
  dependency('webkitgtk-6.0'),
]

executable('polyhymnia', polyhymnia_sources + enums + resources,
  dependencies: polyhymnia_deps,
       install: true,
)
