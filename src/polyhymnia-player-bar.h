
#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define POLYHYMNIA_TYPE_PLAYER_BAR (polyhymnia_player_bar_get_type())

G_DECLARE_FINAL_TYPE (PolyhymniaPlayerBar, polyhymnia_player_bar, POLYHYMNIA, PLAYER_BAR, GtkWidget)

GtkWidget *
polyhymnia_player_bar_get_lyrics_toggle_button (const PolyhymniaPlayerBar *self);

GtkWidget *
polyhymnia_player_bar_get_queue_toggle_button (const PolyhymniaPlayerBar *self);

G_END_DECLS
