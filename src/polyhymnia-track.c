
#include "polyhymnia-format-utils.h"
#include "polyhymnia-track.h"

/* Type metadata */
typedef enum
{
  PROP_ID = 1,
  PROP_QUEUE_POSITION,
  PROP_URI,
  PROP_TITLE,
  PROP_DISC,
  PROP_ALBUM_POSITION,
  PROP_ALBUM,
  PROP_ALBUM_SORT,
  PROP_ALBUM_ARTIST,
  PROP_ARTIST,
  PROP_DATE,
  PROP_ORIGINAL_DATE,
  PROP_DURATION,
  PROP_DURATION_READABLE,
  N_PROPERTIES,
} PolyhymniaTrackProperty;

struct _PolyhymniaTrack
{
  GObject  parent_instance;

  /* Data */
  guint id;
  guint queue_position;
  gchar *uri;
  gchar *title;
  guint disc;
  gchar *album_position;
  gchar *album;
  gchar *album_sort;
  gchar *album_artist;
  gchar *artist;
  gchar *date;
  gchar *original_date;
  guint duration;
  gchar *duration_readable;
};

G_DEFINE_FINAL_TYPE (PolyhymniaTrack, polyhymnia_track, G_TYPE_OBJECT)

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

/* Class stuff */
static void
polyhymnia_track_finalize (GObject *gobject)
{
  PolyhymniaTrack *self = POLYHYMNIA_TRACK (gobject);

  g_free (self->uri);
  g_free (self->title);
  g_free (self->album_position);
  g_free (self->album);
  g_free (self->album_sort);
  g_free (self->album_artist);
  g_free (self->artist);
  g_free (self->date);
  g_free (self->original_date);
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
    case PROP_QUEUE_POSITION:
      g_value_set_uint (value, self->queue_position);
      break;
    case PROP_URI:
      g_value_set_string (value, self->uri);
      break;
    case PROP_TITLE:
      g_value_set_string (value, self->title);
      break;
    case PROP_DISC:
      g_value_set_uint (value, self->disc);
      break;
    case PROP_ALBUM_POSITION:
      g_value_set_string (value, self->album_position);
      break;
    case PROP_ALBUM:
      g_value_set_string (value, self->album);
      break;
    case PROP_ALBUM_SORT:
      g_value_set_string (value, self->album_sort);
      break;
    case PROP_ALBUM_ARTIST:
      g_value_set_string (value, self->album_artist);
      break;
    case PROP_ARTIST:
      g_value_set_string (value, self->artist);
      break;
    case PROP_DATE:
      g_value_set_string (value, self->date);
      break;
    case PROP_ORIGINAL_DATE:
      g_value_set_string (value, self->original_date);
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
    case PROP_QUEUE_POSITION:
      self->queue_position = g_value_get_uint (value);
      break;
    case PROP_URI:
      g_set_str (&(self->uri), g_value_get_string (value));
      break;
    case PROP_TITLE:
      g_set_str (&(self->title), g_value_get_string (value));
      break;
    case PROP_DISC:
      self->disc = g_value_get_uint (value);
      break;
    case PROP_ALBUM_POSITION:
      g_set_str (&(self->album_position), g_value_get_string (value));
      break;
    case PROP_ALBUM:
      g_set_str (&(self->album), g_value_get_string (value));
      break;
    case PROP_ALBUM_SORT:
      g_set_str (&(self->album_sort), g_value_get_string (value));
      break;
    case PROP_ALBUM_ARTIST:
      g_set_str (&(self->album_artist), g_value_get_string (value));
      break;
    case PROP_ARTIST:
      g_set_str (&(self->artist), g_value_get_string (value));
      break;
    case PROP_DATE:
      g_set_str (&(self->date), g_value_get_string (value));
      break;
    case PROP_ORIGINAL_DATE:
      g_set_str (&(self->original_date), g_value_get_string (value));
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
  obj_properties[PROP_QUEUE_POSITION] =
    g_param_spec_uint ("queue-position",
                       "Queue position",
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
  obj_properties[PROP_DISC] =
    g_param_spec_uint ("disc",
                       "Disc",
                       "Disc number of an album that track belongs to",
                       0,
                       G_MAXUINT,
                       0,
                       G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  obj_properties[PROP_ALBUM_POSITION] =
    g_param_spec_string ("album-position",
                         "Album position",
                         "Track position (in an album)",
                         NULL,
                         G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  obj_properties[PROP_ALBUM] =
    g_param_spec_string ("album",
                          "Album",
                          "Title of source album",
                          NULL,
                          G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  obj_properties[PROP_ALBUM_SORT] =
    g_param_spec_string ("album-sort",
                          "Album (sort)",
                          "Title of source album, suitable for sort",
                          NULL,
                          G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  obj_properties[PROP_ALBUM_ARTIST] =
    g_param_spec_string ("album-artist",
                          "Album artist",
                          "Name of a main album artist",
                          NULL,
                          G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  obj_properties[PROP_ARTIST] =
    g_param_spec_string ("artist",
                          "Artist",
                          "Name of an artist performing track",
                          NULL,
                          G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  obj_properties[PROP_DATE] =
    g_param_spec_string ("date",
                          "Date",
                          "Date of track release",
                          NULL,
                          G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  obj_properties[PROP_ORIGINAL_DATE] =
    g_param_spec_string ("original-date",
                          "Original date",
                          "Date of original track version release",
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
polyhymnia_track_get_artist (const PolyhymniaTrack *self)
{
  return self->artist;
}

guint
polyhymnia_track_get_disc (const PolyhymniaTrack *self)
{
  return self->disc;
}

guint
polyhymnia_track_get_duration (const PolyhymniaTrack *self)
{
  return self->duration;
}

const gchar *
polyhymnia_track_get_duration_readable (const PolyhymniaTrack *self)
{
  return self->duration_readable;
}

guint
polyhymnia_track_get_id (const PolyhymniaTrack *self)
{
  return self->id;
}

const gchar *
polyhymnia_track_get_title (const PolyhymniaTrack *self)
{
  return self->title;
}

const gchar *
polyhymnia_track_get_uri (const PolyhymniaTrack *self)
{
  return self->uri;
}
