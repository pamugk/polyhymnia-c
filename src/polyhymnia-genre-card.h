
#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define POLYHYMNIA_TYPE_GENRE_CARD (polyhymnia_genre_card_get_type())

G_DECLARE_FINAL_TYPE (PolyhymniaGenreCard, polyhymnia_genre_card, POLYHYMNIA, GENRE_CARD, GtkWidget)

G_END_DECLS
