# Basic app sources & dependencies
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
  'polyhymnia-last-modified-page.c',
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
  dependency('libadwaita-1', version: '>= 1.5'),
  dependency('libmpdclient', version: '>= 2.20'),
]

# Additional app features configuration
external_data_feature = get_option('external_data_feature')
json_glib = dependency('json-glib-1.0', version: '>= 1.2', required: external_data_feature)
libsoup = dependency('libsoup-3.0', required: external_data_feature)
external_data_feature_enabled = json_glib.found() and libsoup.found()
if external_data_feature_enabled
  polyhymnia_sources += [
    'polyhymnia-additional-info-provider.c',
    'polyhymnia-album-details-dialog.c',
  ]
  polyhymnia_deps += [
    json_glib,
    libsoup,
  ]
endif

lirycs_feature = get_option('lyrics_feature')
webkitgtk = dependency('webkitgtk-6.0', required: lirycs_feature)
lirycs_feature_enabled = external_data_feature_enabled and webkitgtk.found()
if lirycs_feature_enabled
  polyhymnia_deps += webkitgtk
  polyhymnia_sources += [
    'polyhymnia-current-lyrics-pane.c',
    'polyhymnia-lyrics-provider.c',
  ]
endif

# Configuration headers generation
configure_file(input : 'secrets.h.in', output: 'secrets.h',
  configuration: {
    'genius_client_access_token': get_option('genius_client_access_token'),
    'lastfm_api_key': get_option('lastfm_api_key'),
  })
configure_file(input: 'app-features.h.in', output: 'app-features.h',
  configuration: {
    'POLYHYMNIA_FEATURE_EXTERNAL_DATA': external_data_feature_enabled,
    'POLYHYMNIA_FEATURE_LYRICS': lirycs_feature_enabled,
  })

# App executable definition
executable('polyhymnia', polyhymnia_sources + enums + resources,
  dependencies: polyhymnia_deps,
  install: true,
)
