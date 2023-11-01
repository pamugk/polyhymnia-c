
#include "config.h"

#include "polyhymnia-album-card.h"

typedef enum
{
  PROP_ALBUM = 1,
  N_PROPERTIES,
} PolyhymniaAlbumCardProperty;

struct _PolyhymniaAlbumCard
{
  GtkWidget  parent_instance;

  /* State fields */
  gchar *album;

  /* Template widgets */
  GtkBox          *container;
  GtkImage        *cover_image;
  GtkLabel        *album_label;
  GtkLabel        *artist_label;
  GtkLabel        *date_label;
};

G_DEFINE_FINAL_TYPE (PolyhymniaAlbumCard, polyhymnia_album_card, GTK_TYPE_WIDGET)

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

/* Class stuff - constructors, destructors, etc */
static void
polyhymnia_album_card_constructed(GObject *gobject)
{
  PolyhymniaAlbumCard *self = POLYHYMNIA_ALBUM_CARD (gobject);

  gtk_label_set_label (self->album_label, self->album);

  G_OBJECT_CLASS (polyhymnia_album_card_parent_class)->constructed (gobject);
}

static void
polyhymnia_album_card_dispose(GObject *gobject)
{
  PolyhymniaAlbumCard *self = POLYHYMNIA_ALBUM_CARD (gobject);

  gtk_widget_unparent (GTK_WIDGET (self->container));
  gtk_widget_dispose_template (GTK_WIDGET (self), POLYHYMNIA_TYPE_ALBUM_CARD);

  G_OBJECT_CLASS (polyhymnia_album_card_parent_class)->dispose (gobject);
}

static void
polyhymnia_album_card_finalize (GObject *gobject)
{
  PolyhymniaAlbumCard *self = POLYHYMNIA_ALBUM_CARD (gobject);

  g_free (self->album);

  G_OBJECT_CLASS (polyhymnia_album_card_parent_class)->finalize (gobject);
}

static void
polyhymnia_album_card_get_property (GObject    *object,
                          guint       property_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
  PolyhymniaAlbumCard *self = POLYHYMNIA_ALBUM_CARD (object);

  switch ((PolyhymniaAlbumCardProperty) property_id)
    {
    case PROP_ALBUM:
      g_value_set_string (value, self->album);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
polyhymnia_album_card_set_property (GObject      *object,
                          guint         property_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  PolyhymniaAlbumCard *self = POLYHYMNIA_ALBUM_CARD (object);

  switch ((PolyhymniaAlbumCardProperty) property_id)
    {
    case PROP_ALBUM:
      {
        gchar *old_album = self->album;
        self->album = g_strdup (g_value_get_string (value));
        g_free (old_album);
        break;
      }

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
polyhymnia_album_card_class_init (PolyhymniaAlbumCardClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->constructed = polyhymnia_album_card_constructed;
  gobject_class->dispose = polyhymnia_album_card_dispose;
  gobject_class->finalize = polyhymnia_album_card_finalize;
  gobject_class->get_property = polyhymnia_album_card_get_property;
  gobject_class->set_property = polyhymnia_album_card_set_property;

  obj_properties[PROP_ALBUM] =
    g_param_spec_string ("album",
                         "Title",
                         "Title of an album.",
                         NULL,
                         G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);

  g_object_class_install_properties (gobject_class,
                                     N_PROPERTIES,
                                     obj_properties);

  gtk_widget_class_set_layout_manager_type(widget_class, GTK_TYPE_BIN_LAYOUT);
  gtk_widget_class_set_template_from_resource (widget_class, "/com/github/pamugk/polyhymnia/ui/polyhymnia-album-card.ui");

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaAlbumCard, container);
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
