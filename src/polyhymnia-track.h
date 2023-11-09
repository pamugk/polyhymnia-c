
#pragma once

#include <glib-2.0/glib-object.h>

G_BEGIN_DECLS

#define POLYHYMNIA_TYPE_TRACK (polyhymnia_track_get_type())

G_DECLARE_FINAL_TYPE (PolyhymniaTrack, polyhymnia_track, POLYHYMNIA, TRACK, GObject)

const gchar *
polyhymnia_track_get_uri (const PolyhymniaTrack *track);

G_END_DECLS
