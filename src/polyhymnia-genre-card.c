
#include "config.h"

#include "polyhymnia-genre-card.h"

struct _PolyhymniaGenreCard
{
  GtkBox  parent_instance;

  /* Template widgets */
  GtkImage        *image;
  GtkLabel        *name_label;
};

G_DEFINE_FINAL_TYPE (PolyhymniaGenreCard, polyhymnia_genre_card, GTK_TYPE_WIDGET)

static void
polyhymnia_genre_card_class_init (PolyhymniaGenreCardClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  gtk_widget_class_set_template_from_resource (widget_class, "/com/github/pamugk/polyhymnia/ui/polyhymnia-genre-card.ui");

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaGenreCard, image);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaGenreCard, name_label);
}

static void
polyhymnia_genre_card_init (PolyhymniaGenreCard *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}
