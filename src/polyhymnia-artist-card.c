
#include "config.h"

#include "polyhymnia-artist-card.h"

#include <adwaita.h>

struct _PolyhymniaArtistCard
{
  GtkBox  parent_instance;

  /* Template widgets */
  AdwAvatar         *avatar;
  GtkLabel          *name_label;
};

G_DEFINE_FINAL_TYPE (PolyhymniaArtistCard, polyhymnia_artist_card, GTK_TYPE_WIDGET)

static void
polyhymnia_artist_card_class_init (PolyhymniaArtistCardClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  gtk_widget_class_set_template_from_resource (widget_class, "/com/github/pamugk/polyhymnia/ui/polyhymnia-artist-card.ui");

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaArtistCard, avatar);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaArtistCard, name_label);
}

static void
polyhymnia_artist_card_init (PolyhymniaArtistCard *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}
