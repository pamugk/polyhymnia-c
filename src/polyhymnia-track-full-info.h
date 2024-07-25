
#pragma once

#include <glib-2.0/glib-object.h>

#include "polyhymnia-audio-format.h"

G_BEGIN_DECLS

#define POLYHYMNIA_TYPE_TRACK_FULL_INFO (polyhymnia_track_full_info_get_type())

G_DECLARE_FINAL_TYPE (PolyhymniaTrackFullInfo, polyhymnia_track_full_info,
                      POLYHYMNIA, TRACK_FULL_INFO,
                      GObject)

const char *
polyhymnia_track_full_info_get_album (PolyhymniaTrackFullInfo *self);

const char *
polyhymnia_track_full_info_get_album_artist (PolyhymniaTrackFullInfo *self);

const char *
polyhymnia_track_full_info_get_artists (PolyhymniaTrackFullInfo *self);

PolyhymniaAudioFormat *
polyhymnia_track_full_info_get_audio_format (PolyhymniaTrackFullInfo *self);

const char *
polyhymnia_track_full_info_get_comment (PolyhymniaTrackFullInfo *self);

const char *
polyhymnia_track_full_info_get_composers (PolyhymniaTrackFullInfo *self);

const char *
polyhymnia_track_full_info_get_conductors (PolyhymniaTrackFullInfo *self);

const char *
polyhymnia_track_full_info_get_date (PolyhymniaTrackFullInfo *self);

const char *
polyhymnia_track_full_info_get_disc (PolyhymniaTrackFullInfo *self);

const char *
polyhymnia_track_full_info_get_ensemble (PolyhymniaTrackFullInfo *self);

const char *
polyhymnia_track_full_info_get_genre (PolyhymniaTrackFullInfo *self);

const char *
polyhymnia_track_full_info_get_location (PolyhymniaTrackFullInfo *self);

const char *
polyhymnia_track_full_info_get_movement (PolyhymniaTrackFullInfo *self);

const char *
polyhymnia_track_full_info_get_movement_number (PolyhymniaTrackFullInfo *self);

const char *
polyhymnia_track_full_info_get_musicbrainz_album_id (PolyhymniaTrackFullInfo *self);

const char *
polyhymnia_track_full_info_get_musicbrainz_album_artist_id (PolyhymniaTrackFullInfo *self);

const char *
polyhymnia_track_full_info_get_musicbrainz_artist_id (PolyhymniaTrackFullInfo *self);

const char *
polyhymnia_track_full_info_get_musicbrainz_release_group_id (PolyhymniaTrackFullInfo *self);

const char *
polyhymnia_track_full_info_get_musicbrainz_release_track_id (PolyhymniaTrackFullInfo *self);

const char *
polyhymnia_track_full_info_get_musicbrainz_track_id (PolyhymniaTrackFullInfo *self);

const char *
polyhymnia_track_full_info_get_musicbrainz_work_id (PolyhymniaTrackFullInfo *self);

const char *
polyhymnia_track_full_info_get_original_date (PolyhymniaTrackFullInfo *self);

const char *
polyhymnia_track_full_info_get_performers (PolyhymniaTrackFullInfo *self);

const char *
polyhymnia_track_full_info_get_position (PolyhymniaTrackFullInfo *self);

const char *
polyhymnia_track_full_info_get_publisher (PolyhymniaTrackFullInfo *self);

const char *
polyhymnia_track_full_info_get_title (PolyhymniaTrackFullInfo *self);

const char *
polyhymnia_track_full_info_get_uri (PolyhymniaTrackFullInfo *self);

const char *
polyhymnia_track_full_info_get_work (PolyhymniaTrackFullInfo *self);

G_END_DECLS
