
#pragma once

#include <glib-2.0/glib-object.h>

G_BEGIN_DECLS

#define POLYHYMNIA_TYPE_TRACK_FULL_INFO (polyhymnia_track_full_info_get_type())

G_DECLARE_FINAL_TYPE (PolyhymniaTrackFullInfo, polyhymnia_track_full_info,
                      POLYHYMNIA, TRACK_FULL_INFO,
                      GObject)

const gchar *
polyhymnia_track_full_info_get_album (PolyhymniaTrackFullInfo *self);

const gchar *
polyhymnia_track_full_info_get_album_artist (PolyhymniaTrackFullInfo *self);

guint
polyhymnia_track_full_info_get_disc (PolyhymniaTrackFullInfo *self);

const gchar *
polyhymnia_track_full_info_get_title (PolyhymniaTrackFullInfo *self);

const gchar *
polyhymnia_track_full_info_get_uri (PolyhymniaTrackFullInfo *self);

G_END_DECLS
