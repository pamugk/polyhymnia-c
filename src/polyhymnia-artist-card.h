
#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define POLYHYMNIA_TYPE_ARTIST_CARD (polyhymnia_artist_card_get_type())

G_DECLARE_FINAL_TYPE (PolyhymniaArtistCard, polyhymnia_artist_card, POLYHYMNIA, ARTIST_CARD, GtkWidget)

G_END_DECLS
