
#include "config.h"

#include "polyhymnia-album-card.h"

struct _PolyhymniaAlbumCard
{
  GtkBox  parent_instance;

  /* Template widgets */
  GtkImage        *cover_image;
  GtkLabel        *album_label;
  GtkLabel        *artist_label;
  GtkLabel        *date_label;
};

G_DEFINE_FINAL_TYPE (PolyhymniaAlbumCard, polyhymnia_album_card, GTK_TYPE_WIDGET)

static void
polyhymnia_album_card_class_init (PolyhymniaAlbumCardClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  gtk_widget_class_set_template_from_resource (widget_class, "/com/github/pamugk/polyhymnia/ui/polyhymnia-album-card.ui");

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaAlbumCard, cover_image);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaAlbumCard, album_label);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaAlbumCard, artist_label);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaAlbumCard, date_label);
}

static void
polyhymnia_album_card_init (PolyhymniaAlbumCard *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}
