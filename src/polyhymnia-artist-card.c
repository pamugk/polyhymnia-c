
#include "config.h"

#include "polyhymnia-artist-card.h"

#include <adwaita.h>

typedef enum
{
  PROP_ARTIST_NAME = 1,
  N_PROPERTIES,
} PolyhymniaArtistCardProperty;

struct _PolyhymniaArtistCard
{
  GtkBox  parent_instance;

  /* State fields */
  gchar *artist_name;

  /* Template widgets */
  AdwAvatar         *avatar;
  GtkLabel          *name_label;
};

G_DEFINE_FINAL_TYPE (PolyhymniaArtistCard, polyhymnia_artist_card, GTK_TYPE_WIDGET)

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

/* Class stuff - constructors, destructors, etc */
static void
polyhymnia_artist_card_constructed(GObject *gobject)
{
  PolyhymniaArtistCard *self = POLYHYMNIA_ARTIST_CARD (gobject);

  adw_avatar_set_text (self->avatar, self->artist_name);
  gtk_label_set_label (self->name_label, self->artist_name);

  G_OBJECT_CLASS (polyhymnia_artist_card_parent_class)->constructed (gobject);
}

static void
polyhymnia_artist_card_finalize (GObject *gobject)
{
  PolyhymniaArtistCard *self = POLYHYMNIA_ARTIST_CARD (gobject);

  g_free (self->artist_name);

  G_OBJECT_CLASS (polyhymnia_artist_card_parent_class)->finalize (gobject);
}

static void
polyhymnia_artist_card_set_property (GObject      *object,
                          guint         property_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  PolyhymniaArtistCard *self = POLYHYMNIA_ARTIST_CARD (object);

  switch ((PolyhymniaArtistCardProperty) property_id)
    {
    case PROP_ARTIST_NAME:
      {
        gchar *old_name = self->artist_name;
        self->artist_name = g_strdup (g_value_get_string (value));
        g_free (old_name);
        break;
      }

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
polyhymnia_artist_card_get_property (GObject    *object,
                          guint       property_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
  PolyhymniaArtistCard *self = POLYHYMNIA_ARTIST_CARD (object);

  switch ((PolyhymniaArtistCardProperty) property_id)
    {
    case PROP_ARTIST_NAME:
      g_value_set_string (value, self->artist_name);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
polyhymnia_artist_card_class_init (PolyhymniaArtistCardClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->constructed = polyhymnia_artist_card_constructed;
  gobject_class->finalize = polyhymnia_artist_card_finalize;
  gobject_class->get_property = polyhymnia_artist_card_get_property;
  gobject_class->set_property = polyhymnia_artist_card_set_property;

  obj_properties[PROP_ARTIST_NAME] =
    g_param_spec_string ("artist-name",
                         "Artist Name",
                         "Name of an artist.",
                         NULL,
                         G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties (gobject_class,
                                     N_PROPERTIES,
                                     obj_properties);

  gtk_widget_class_set_template_from_resource (widget_class, "/com/github/pamugk/polyhymnia/ui/polyhymnia-artist-card.ui");

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaArtistCard, avatar);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaArtistCard, name_label);
}

static void
polyhymnia_artist_card_init (PolyhymniaArtistCard *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}
