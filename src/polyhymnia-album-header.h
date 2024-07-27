
#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define POLYHYMNIA_TYPE_ALBUM_HEADER (polyhymnia_album_header_get_type())

G_DECLARE_FINAL_TYPE (PolyhymniaAlbumHeader, polyhymnia_album_header, POLYHYMNIA, ALBUM_HEADER, GtkWidget)

const gchar *
polyhymnia_album_header_get_album_title (PolyhymniaAlbumHeader *self);

const gchar *
polyhymnia_album_header_get_album_musicbrainz_id (PolyhymniaAlbumHeader *self);

G_END_DECLS
