
#include "polyhymnia-album.h"

typedef enum
{
  PROP_TITLE = 1,
  PROP_ARTIST,
  PROP_RELEASE_DATE,
  N_PROPERTIES,
} PolyhymniaAlbumProperty;

struct _PolyhymniaAlbum
{
  GObject  parent_instance;

  /* Data */
  gchar *title;
  gchar *artist;
  gchar *release_date;
};

G_DEFINE_FINAL_TYPE (PolyhymniaAlbum, polyhymnia_album, G_TYPE_OBJECT)

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

static void
polyhymnia_album_finalize (GObject *gobject)
{
  PolyhymniaAlbum *self = POLYHYMNIA_ALBUM (gobject);

  g_free (self->title);
  g_free (self->artist);
  g_free (self->release_date);

  G_OBJECT_CLASS (polyhymnia_album_parent_class)->finalize (gobject);
}

static void
polyhymnia_album_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  PolyhymniaAlbum *self = POLYHYMNIA_ALBUM (object);

  switch ((PolyhymniaAlbumProperty) property_id)
    {
    case PROP_TITLE:
      g_value_set_string (value, self->title);
      break;
    case PROP_ARTIST:
      g_value_set_string (value, self->artist);
      break;
    case PROP_RELEASE_DATE:
      g_value_set_string (value, self->release_date);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
polyhymnia_album_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  PolyhymniaAlbum *self = POLYHYMNIA_ALBUM (object);

  switch ((PolyhymniaAlbumProperty) property_id)
    {
    case PROP_TITLE:
      g_set_str (&(self->title), g_value_get_string (value));
      break;
    case PROP_ARTIST:
      g_set_str (&(self->artist), g_value_get_string (value));
      break;
    case PROP_RELEASE_DATE:
      g_set_str (&(self->release_date), g_value_get_string (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
polyhymnia_album_class_init (PolyhymniaAlbumClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->finalize = polyhymnia_album_finalize;
  gobject_class->get_property = polyhymnia_album_get_property;
  gobject_class->set_property = polyhymnia_album_set_property;

  obj_properties[PROP_TITLE] =
    g_param_spec_string ("title",
                          "Title",
                          "Title of an album.",
                          NULL,
                          G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);

  obj_properties[PROP_ARTIST] =
    g_param_spec_string ("artist",
                          "Artist",
                          "Album artist.",
                          "Unknown",
                          G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);

  obj_properties[PROP_RELEASE_DATE] =
    g_param_spec_string ("release-date",
                          "Release Date",
                          "Release date (or year) of an album.",
                          "2023",
                          G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);

  g_object_class_install_properties (gobject_class,
                                     N_PROPERTIES,
                                     obj_properties);
}

static void
polyhymnia_album_init (PolyhymniaAlbum *self)
{
}

const gchar *
polyhymnia_album_get_title (const PolyhymniaAlbum *self)
{
  return self->title;
}
