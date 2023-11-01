
#include "config.h"

#include "polyhymnia-track-row.h"

struct _PolyhymniaTrackRow
{
  GtkWidget  parent_instance;

  /* Template widgets */
  GtkGrid         *container;
  GtkImage        *album_cover_image;
  GtkLabel        *track_label;
  GtkLabel        *artist_label;
  GtkLabel        *album_label;
  GtkLabel        *duration_label;
};

G_DEFINE_FINAL_TYPE (PolyhymniaTrackRow, polyhymnia_track_row, GTK_TYPE_WIDGET)

static void
polyhymnia_track_row_dispose(GObject *gobject)
{
  PolyhymniaTrackRow *self = POLYHYMNIA_TRACK_ROW (gobject);

  gtk_widget_unparent (GTK_WIDGET (self->container));
  gtk_widget_dispose_template (GTK_WIDGET (self), POLYHYMNIA_TYPE_TRACK_ROW);

  G_OBJECT_CLASS (polyhymnia_track_row_parent_class)->dispose (gobject);
}

static void
polyhymnia_track_row_class_init (PolyhymniaTrackRowClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->dispose = polyhymnia_track_row_dispose;

  gtk_widget_class_set_layout_manager_type( widget_class, GTK_TYPE_BIN_LAYOUT);
  gtk_widget_class_set_template_from_resource (widget_class, "/com/github/pamugk/polyhymnia/ui/polyhymnia-track-row.ui");

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackRow, container);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackRow, album_cover_image);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackRow, track_label);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackRow, artist_label);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackRow, album_label);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackRow, duration_label);
}

static void
polyhymnia_track_row_init (PolyhymniaTrackRow *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}
