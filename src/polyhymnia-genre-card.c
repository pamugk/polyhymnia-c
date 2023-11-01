
#include "config.h"

#include "polyhymnia-genre-card.h"

typedef enum
{
  PROP_GENRE = 1,
  N_PROPERTIES,
} PolyhymniaGenreCardProperty;

struct _PolyhymniaGenreCard
{
  GtkWidget  parent_instance;

  /* State fields */
  gchar *genre;

  /* Template widgets */
  GtkBox          *container;
  GtkLabel        *name_label;
};

G_DEFINE_FINAL_TYPE (PolyhymniaGenreCard, polyhymnia_genre_card, GTK_TYPE_WIDGET)

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

/* Class stuff - constructors, destructors, etc */
static void
polyhymnia_genre_card_constructed(GObject *gobject)
{
  PolyhymniaGenreCard *self = POLYHYMNIA_GENRE_CARD (gobject);

  gtk_label_set_label (self->name_label, self->genre);

  G_OBJECT_CLASS (polyhymnia_genre_card_parent_class)->constructed (gobject);
}

static void
polyhymnia_genre_card_dispose (GObject *gobject)
{
  PolyhymniaGenreCard *self = POLYHYMNIA_GENRE_CARD (gobject);

  gtk_widget_unparent (GTK_WIDGET (self->container));
  gtk_widget_dispose_template (GTK_WIDGET (self), POLYHYMNIA_TYPE_GENRE_CARD);

  G_OBJECT_CLASS (polyhymnia_genre_card_parent_class)->dispose (gobject);
}

static void
polyhymnia_genre_card_finalize (GObject *gobject)
{
  PolyhymniaGenreCard *self = POLYHYMNIA_GENRE_CARD (gobject);

  g_free (self->genre);

  G_OBJECT_CLASS (polyhymnia_genre_card_parent_class)->finalize (gobject);
}

static void
polyhymnia_genre_card_get_property (GObject    *object,
                          guint       property_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
  PolyhymniaGenreCard *self = POLYHYMNIA_GENRE_CARD (object);

  switch ((PolyhymniaGenreCardProperty) property_id)
    {
    case PROP_GENRE:
      g_value_set_string (value, self->genre);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
polyhymnia_genre_card_set_property (GObject      *object,
                          guint         property_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  PolyhymniaGenreCard *self = POLYHYMNIA_GENRE_CARD (object);

  switch ((PolyhymniaGenreCardProperty) property_id)
    {
    case PROP_GENRE:
      {
        gchar *old_genre = self->genre;
        self->genre = g_strdup (g_value_get_string (value));
        g_free (old_genre);
        break;
      }

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
polyhymnia_genre_card_class_init (PolyhymniaGenreCardClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->constructed = polyhymnia_genre_card_constructed;
  gobject_class->dispose = polyhymnia_genre_card_dispose;
  gobject_class->finalize = polyhymnia_genre_card_finalize;
  gobject_class->get_property = polyhymnia_genre_card_get_property;
  gobject_class->set_property = polyhymnia_genre_card_set_property;

  obj_properties[PROP_GENRE] =
    g_param_spec_string ("genre",
                         "Genre Name",
                         "Name of a genre.",
                         NULL,
                         G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);

  g_object_class_install_properties (gobject_class,
                                     N_PROPERTIES,
                                     obj_properties);

  gtk_widget_class_set_layout_manager_type(widget_class, GTK_TYPE_BIN_LAYOUT);
  gtk_widget_class_set_template_from_resource (widget_class, "/com/github/pamugk/polyhymnia/ui/polyhymnia-genre-card.ui");

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaGenreCard, container);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaGenreCard, name_label);
}

static void
polyhymnia_genre_card_init (PolyhymniaGenreCard *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}

