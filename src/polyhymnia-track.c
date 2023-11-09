
#include "polyhymnia-track.h"

/* Type metadata */
typedef enum
{
  PROP_ID = 1,
  PROP_POSITION,
  PROP_URI,
  PROP_TITLE,
  PROP_ALBUM,
  PROP_ARTIST,
  PROP_DURATION,
  PROP_DURATION_READABLE,
  N_PROPERTIES,
} PolyhymniaTrackProperty;

struct _PolyhymniaTrack
{
  GObject  parent_instance;

  /* Data */
  guint id;
  guint position;
  gchar *uri;
  gchar *title;
  gchar *album;
  gchar *artist;
  guint duration;
  gchar *duration_readable;
};

G_DEFINE_FINAL_TYPE (PolyhymniaTrack, polyhymnia_track, G_TYPE_OBJECT)

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

/* Utility functions */
static gchar*
seconds_to_readable(guint duration)
{
  gushort hours = duration / 3600;
  gushort minutes = (duration % 3600) / 60;
  gushort seconds = duration %  60;
  return hours == 0
    ? g_strdup_printf ("%02d:%02d", minutes, seconds)
    : g_strdup_printf ("%d:%02d:%02d", hours, minutes, seconds);
}

/* Class stuff */
static void
polyhymnia_track_finalize (GObject *gobject)
{
  PolyhymniaTrack *self = POLYHYMNIA_TRACK (gobject);

  g_free (self->uri);
  g_free (self->title);
  g_free (self->album);
  g_free (self->artist);
  g_free (self->duration_readable);

  G_OBJECT_CLASS (polyhymnia_track_parent_class)->finalize (gobject);
}

static void
polyhymnia_track_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  PolyhymniaTrack *self = POLYHYMNIA_TRACK (object);

  switch ((PolyhymniaTrackProperty) property_id)
    {
    case PROP_ID:
      g_value_set_uint (value, self->id);
      break;
    case PROP_POSITION:
      g_value_set_uint (value, self->position);
      break;
    case PROP_URI:
      g_value_set_string (value, self->uri);
      break;
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
    case PROP_DURATION_READABLE:
      g_value_set_string (value, self->duration_readable);
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
    case PROP_ID:
      self->id = g_value_get_uint (value);
      break;
    case PROP_POSITION:
      self->position = g_value_get_uint (value);
      break;
    case PROP_URI:
      g_set_str (&(self->uri), g_value_get_string (value));
      break;
    case PROP_TITLE:
      g_set_str (&(self->title), g_value_get_string (value));
      break;
    case PROP_ALBUM:
      g_set_str (&(self->album), g_value_get_string (value));
      break;
    case PROP_ARTIST:
      g_set_str (&(self->artist), g_value_get_string (value));
      break;
    case PROP_DURATION:
      g_free (self->duration_readable);
      self->duration = g_value_get_uint (value);
      self->duration_readable = seconds_to_readable(self->duration);
      g_object_notify (object, "duration-readable");
      break;
    case PROP_DURATION_READABLE:
      g_set_str (&(self->duration_readable), g_value_get_string (value));
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

  gobject_class->finalize = polyhymnia_track_finalize;
  gobject_class->get_property = polyhymnia_track_get_property;
  gobject_class->set_property = polyhymnia_track_set_property;

  obj_properties[PROP_ID] =
    g_param_spec_uint ("id",
                       "ID",
                       "Track id (in a queue)",
                       0,
                       G_MAXUINT,
                       0,
                       G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  obj_properties[PROP_POSITION] =
    g_param_spec_uint ("position",
                       "Position",
                       "Track position (in a queue)",
                       0,
                       G_MAXUINT,
                       0,
                       G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  obj_properties[PROP_URI] =
    g_param_spec_string ("uri",
                         "URI",
                         "Track filepath",
                         NULL,
                         G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  obj_properties[PROP_TITLE] =
    g_param_spec_string ("title",
                          "Title",
                          "Track title",
                          NULL,
                          G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  obj_properties[PROP_ALBUM] =
    g_param_spec_string ("album",
                          "Album",
                          "Title of source album",
                          NULL,
                          G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  obj_properties[PROP_ARTIST] =
    g_param_spec_string ("artist",
                          "Artist",
                          "Name of an artist",
                          NULL,
                          G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  obj_properties[PROP_DURATION] =
    g_param_spec_uint ("duration",
                       "Duration",
                       "Track duration (in seconds)",
                       0,
                       G_MAXUINT,
                       0,
                       G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  obj_properties[PROP_DURATION_READABLE] =
    g_param_spec_string ("duration-readable",
                         "Duration (human-readable representation)",
                         "Track duration (string in UI format)",
                         NULL,
                         G_PARAM_READABLE);

  g_object_class_install_properties (gobject_class,
                                     N_PROPERTIES,
                                     obj_properties);
}

static void
polyhymnia_track_init (PolyhymniaTrack *self)
{
}

/* Instance methods */
const gchar *
polyhymnia_track_get_uri (const PolyhymniaTrack *track)
{
  return track->uri;
}
