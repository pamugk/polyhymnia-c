
#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define POLYHYMNIA_TYPE_ALBUM_CARD (polyhymnia_album_card_get_type())

G_DECLARE_FINAL_TYPE (PolyhymniaAlbumCard, polyhymnia_album_card, POLYHYMNIA, ALBUM_CARD, GtkWidget)

G_END_DECLS
