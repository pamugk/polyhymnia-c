
#pragma once

#include <glib-2.0/glib-object.h>

G_BEGIN_DECLS

#define POLYHYMNIA_TYPE_ARTIST (polyhymnia_artist_get_type())

G_DECLARE_FINAL_TYPE (PolyhymniaArtist, polyhymnia_artist, POLYHYMNIA, ARTIST, GObject)

G_END_DECLS
