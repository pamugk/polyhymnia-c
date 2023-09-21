
#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define POLYHYMNIA_TYPE_TRACK_ROW (polyhymnia_track_row_get_type())

G_DECLARE_FINAL_TYPE (PolyhymniaTrackRow, polyhymnia_track_row, POLYHYMNIA, TRACK_ROW, GtkWidget)

G_END_DECLS
