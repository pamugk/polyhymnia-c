
#include "polyhymnia-track.h"

typedef enum
{
  PROP_TITLE = 1,
  PROP_ALBUM,
  PROP_ARTIST,
  PROP_DURATION,
  N_PROPERTIES,
} PolyhymniaTrackProperty;

struct _PolyhymniaTrack
{
  GObject  parent_instance;

  /* Data */
  gchar *title;
  gchar *album;
  gchar *artist;
  guint duration;
};

G_DEFINE_FINAL_TYPE (PolyhymniaTrack, polyhymnia_track, G_TYPE_OBJECT)

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

static void
polyhymnia_track_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  PolyhymniaTrack *self = POLYHYMNIA_TRACK (object);

  switch ((PolyhymniaTrackProperty) property_id)
    {
    case PROP_TITLE:
      g_value_set_string (value, self->title);
      break;
    case PROP_ALBUM:
      g_value_set_string (value, self->album);
      break;
    case PROP_ARTIST:
      g_value_set_string (value, self->artist);
      break;
    case PROP_DURATION:
      g_value_set_uint (value, self->duration);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
polyhymnia_track_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  PolyhymniaTrack *self = POLYHYMNIA_TRACK (object);

  switch ((PolyhymniaTrackProperty) property_id)
    {
    case PROP_TITLE:
      g_free (self->title);
      self->title = g_strdup (g_value_get_string (value));
      break;
    case PROP_ALBUM:
      g_free (self->album);
      self->album = g_strdup (g_value_get_string (value));
      break;
    case PROP_ARTIST:
      g_free (self->artist);
      self->artist = g_strdup (g_value_get_string (value));
      break;
    case PROP_DURATION:
      self->duration = g_value_get_uint (value);
      break;


    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
polyhymnia_track_class_init (PolyhymniaTrackClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->get_property = polyhymnia_track_get_property;
  gobject_class->set_property = polyhymnia_track_set_property;

  obj_properties[PROP_TITLE] =
    g_param_spec_string ("title",
                          "Title",
                          "Track title.",
                          NULL,
                          G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  obj_properties[PROP_ALBUM] =
    g_param_spec_string ("album",
                          "Album",
                          "Title of source album.",
                          NULL,
                          G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  obj_properties[PROP_ARTIST] =
    g_param_spec_string ("artist",
                          "Artist",
                          "Name of an artist.",
                          NULL,
                          G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  obj_properties[PROP_DURATION] =
    g_param_spec_uint ("duration",
                       "Duration",
                       "Track duration (in seconds).",
                       0,
                       G_MAXUINT,
                       0,
                       G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);

  g_object_class_install_properties (gobject_class,
                                     N_PROPERTIES,
                                     obj_properties);
}

static void
polyhymnia_track_init (PolyhymniaTrack *self)
{
}
