
#include "polyhymnia-artist.h"

typedef enum
{
  PROP_NAME = 1,
  N_PROPERTIES,
} PolyhymniaArtistProperty;

struct _PolyhymniaArtist
{
  GObject  parent_instance;

  /* Data */
  gchar *name;
};

G_DEFINE_FINAL_TYPE (PolyhymniaArtist, polyhymnia_artist, G_TYPE_OBJECT)

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

static void
polyhymnia_artist_finalize (GObject *gobject)
{
  PolyhymniaArtist *self = POLYHYMNIA_ARTIST (gobject);

  g_free (self->name);

  G_OBJECT_CLASS (polyhymnia_artist_parent_class)->finalize (gobject);
}

static void
polyhymnia_artist_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  PolyhymniaArtist *self = POLYHYMNIA_ARTIST (object);

  switch ((PolyhymniaArtistProperty) property_id)
    {
    case PROP_NAME:
      g_value_set_string (value, self->name);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
polyhymnia_artist_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  PolyhymniaArtist *self = POLYHYMNIA_ARTIST (object);

  switch ((PolyhymniaArtistProperty) property_id)
    {
    case PROP_NAME:
      g_set_str (&(self->name), g_value_get_string (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
polyhymnia_artist_class_init (PolyhymniaArtistClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->finalize = polyhymnia_artist_finalize;
  gobject_class->get_property = polyhymnia_artist_get_property;
  gobject_class->set_property = polyhymnia_artist_set_property;

  obj_properties[PROP_NAME] =
    g_param_spec_string ("name",
                          "Name",
                          "Name of an artist.",
                          NULL,
                          G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);

  g_object_class_install_properties (gobject_class,
                                     N_PROPERTIES,
                                     obj_properties);
}

static void
polyhymnia_artist_init (PolyhymniaArtist *self)
{
}

/* Instance methods */
const gchar *
polyhymnia_artist_get_name (PolyhymniaArtist *self)
{
  g_return_val_if_fail (POLYHYMNIA_IS_ARTIST (self), NULL);
  return self->name;
}
