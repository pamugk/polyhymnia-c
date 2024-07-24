
#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define POLYHYMNIA_TYPE_CURRENT_LYRICS_PANE (polyhymnia_current_lyrics_pane_get_type())

G_DECLARE_FINAL_TYPE (PolyhymniaCurrentLyricsPane, polyhymnia_current_lyrics_pane, POLYHYMNIA, CURRENT_LYRICS_PANE, GtkWidget)

G_END_DECLS
