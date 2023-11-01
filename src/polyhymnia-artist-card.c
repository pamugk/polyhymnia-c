
#include "config.h"

#include "polyhymnia-artist-card.h"

#include <adwaita.h>

typedef enum
{
  PROP_ARTIST = 1,
  N_PROPERTIES,
} PolyhymniaArtistCardProperty;

struct _PolyhymniaArtistCard
{
  GtkWidget  parent_instance;

  /* State fields */
  gchar *artist;

  /* Template widgets */
  GtkBox            *container;
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

  adw_avatar_set_text (self->avatar, self->artist);
  gtk_label_set_label (self->name_label, self->artist);

  G_OBJECT_CLASS (polyhymnia_artist_card_parent_class)->constructed (gobject);
}

static void
polyhymnia_artist_card_dispose (GObject *gobject)
{
  PolyhymniaArtistCard *self = POLYHYMNIA_ARTIST_CARD (gobject);

  gtk_widget_unparent (GTK_WIDGET (self->container));
  gtk_widget_dispose_template (GTK_WIDGET (self), POLYHYMNIA_TYPE_ARTIST_CARD);

  G_OBJECT_CLASS (polyhymnia_artist_card_parent_class)->dispose (gobject);
}

static void
polyhymnia_artist_card_finalize (GObject *gobject)
{
  PolyhymniaArtistCard *self = POLYHYMNIA_ARTIST_CARD (gobject);

  g_free (self->artist);

  G_OBJECT_CLASS (polyhymnia_artist_card_parent_class)->finalize (gobject);
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
    case PROP_ARTIST:
      g_value_set_string (value, self->artist);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
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
    case PROP_ARTIST:
      {
        gchar *old_artist = self->artist;
        self->artist = g_strdup (g_value_get_string (value));
        g_free (old_artist);
        break;
      }

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
  gobject_class->dispose = polyhymnia_artist_card_dispose;
  gobject_class->finalize = polyhymnia_artist_card_finalize;
  gobject_class->get_property = polyhymnia_artist_card_get_property;
  gobject_class->set_property = polyhymnia_artist_card_set_property;

  obj_properties[PROP_ARTIST] =
    g_param_spec_string ("artist",
                         "Artist Name",
                         "Name of an artist.",
                         NULL,
                         G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);

  g_object_class_install_properties (gobject_class,
                                     N_PROPERTIES,
                                     obj_properties);

  gtk_widget_class_set_layout_manager_type(widget_class, GTK_TYPE_BIN_LAYOUT);
  gtk_widget_class_set_template_from_resource (widget_class, "/com/github/pamugk/polyhymnia/ui/polyhymnia-artist-card.ui");

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaArtistCard, container);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaArtistCard, avatar);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaArtistCard, name_label);
}

static void
polyhymnia_artist_card_init (PolyhymniaArtistCard *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}
