
#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define POLYHYMNIA_TYPE_QUEUE_PANE (polyhymnia_queue_pane_get_type())

G_DECLARE_FINAL_TYPE (PolyhymniaQueuePane, polyhymnia_queue_pane, POLYHYMNIA, QUEUE_PANE, GtkWidget)

G_END_DECLS
