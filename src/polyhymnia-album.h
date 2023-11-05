
#pragma once

#include <glib-2.0/glib-object.h>

G_BEGIN_DECLS

#define POLYHYMNIA_TYPE_ALBUM (polyhymnia_album_get_type())

G_DECLARE_FINAL_TYPE (PolyhymniaAlbum, polyhymnia_album, POLYHYMNIA, ALBUM, GObject)

G_END_DECLS
