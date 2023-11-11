
#pragma once

#include <glib-2.0/glib-object.h>

G_BEGIN_DECLS

#define POLYHYMNIA_TYPE_TRACK (polyhymnia_track_get_type())

G_DECLARE_FINAL_TYPE (PolyhymniaTrack, polyhymnia_track, POLYHYMNIA, TRACK, GObject)

const gchar *
polyhymnia_track_get_artist (const PolyhymniaTrack *self);

guint
polyhymnia_track_get_duration (const PolyhymniaTrack *self);

guint
polyhymnia_track_get_id (const PolyhymniaTrack *self);

const gchar *
polyhymnia_track_get_title (const PolyhymniaTrack *self);

const gchar *
polyhymnia_track_get_uri (const PolyhymniaTrack *self);

G_END_DECLS
