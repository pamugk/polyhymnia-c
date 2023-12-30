
#include "polyhymnia-track-full-info.h"

/* Type metadata */
typedef enum
{
  PROP_ALBUM = 1,
  PROP_ALBUM_ARTIST,
  PROP_DISC,
  PROP_TITLE,
  PROP_URI,
  N_PROPERTIES,
} PolyhymniaTrackFullInfoProperty;

struct _PolyhymniaTrackFullInfo
{
  GObject  parent_instance;

  /* Data */
  gchar *album;
  gchar *album_artist;
  guint disc;
  gchar *title;
  gchar *uri;
};

G_DEFINE_FINAL_TYPE (PolyhymniaTrackFullInfo, polyhymnia_track_full_info, G_TYPE_OBJECT)

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

/* Class stuff */
static void
polyhymnia_track_full_info_finalize (GObject *gobject)
{
  PolyhymniaTrackFullInfo *self = POLYHYMNIA_TRACK_FULL_INFO (gobject);

  g_free (self->album);
  g_free (self->album_artist);
  g_free (self->title);
  g_free (self->uri);

  G_OBJECT_CLASS (polyhymnia_track_full_info_parent_class)->finalize (gobject);
}

static void
polyhymnia_track_full_info_get_property (GObject    *object,
                                         guint       property_id,
                                         GValue     *value,
                                         GParamSpec *pspec)
{
  PolyhymniaTrackFullInfo *self = POLYHYMNIA_TRACK_FULL_INFO (object);

  switch ((PolyhymniaTrackFullInfoProperty) property_id)
    {
    case PROP_ALBUM:
      g_value_set_string (value, self->album);
      break;
    case PROP_ALBUM_ARTIST:
      g_value_set_string (value, self->album_artist);
      break;
    case PROP_DISC:
      g_value_set_uint (value, self->disc);
      break;
    case PROP_TITLE:
      g_value_set_string (value, self->title);
      break;
    case PROP_URI:
      g_value_set_string (value, self->uri);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
polyhymnia_track_full_info_set_property (GObject      *object,
                                         guint         property_id,
                                         const GValue *value,
                                         GParamSpec   *pspec)
{
  PolyhymniaTrackFullInfo *self = POLYHYMNIA_TRACK_FULL_INFO (object);

  switch ((PolyhymniaTrackFullInfoProperty) property_id)
    {
    case PROP_ALBUM:
      g_set_str (&(self->album), g_value_get_string (value));
      break;
    case PROP_ALBUM_ARTIST:
      g_set_str (&(self->album_artist), g_value_get_string (value));
      break;
    case PROP_DISC:
      self->disc = g_value_get_uint (value);
      break;
    case PROP_TITLE:
      g_set_str (&(self->title), g_value_get_string (value));
      break;
    case PROP_URI:
      g_set_str (&(self->uri), g_value_get_string (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
polyhymnia_track_full_info_class_init (PolyhymniaTrackFullInfoClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->finalize = polyhymnia_track_full_info_finalize;
  gobject_class->get_property = polyhymnia_track_full_info_get_property;
  gobject_class->set_property = polyhymnia_track_full_info_set_property;

  obj_properties[PROP_ALBUM] =
    g_param_spec_string ("album",
                          "Album",
                          "Title of source album",
                          NULL,
                          G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  obj_properties[PROP_ALBUM_ARTIST] =
    g_param_spec_string ("album-artist",
                          "Album artist",
                          "Name of a main album artist",
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
  obj_properties[PROP_TITLE] =
    g_param_spec_string ("title",
                          "Title",
                          "Track title",
                          NULL,
                          G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  obj_properties[PROP_URI] =
    g_param_spec_string ("uri",
                         "URI",
                         "Track filepath",
                         NULL,
                         G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);

  g_object_class_install_properties (gobject_class,
                                     N_PROPERTIES,
                                     obj_properties);
}

static void
polyhymnia_track_full_info_init (PolyhymniaTrackFullInfo *self)
{
}

/* Instance methods */
const gchar *
polyhymnia_track_full_info_get_album (PolyhymniaTrackFullInfo *self)
{
  g_return_val_if_fail (POLYHYMNIA_IS_TRACK_FULL_INFO (self), NULL);
  return self->album;
}

const gchar *
polyhymnia_track_full_info_get_album_artist (PolyhymniaTrackFullInfo *self)
{
  g_return_val_if_fail (POLYHYMNIA_IS_TRACK_FULL_INFO (self), NULL);
  return self->album_artist;
}

guint
polyhymnia_track_full_info_get_disc (PolyhymniaTrackFullInfo *self)
{
  g_return_val_if_fail (POLYHYMNIA_IS_TRACK_FULL_INFO (self), 1);
  return self->disc;
}

const gchar *
polyhymnia_track_full_info_get_title (PolyhymniaTrackFullInfo *self)
{
  g_return_val_if_fail (POLYHYMNIA_IS_TRACK_FULL_INFO (self), NULL);
  return self->title;
}

const gchar *
polyhymnia_track_full_info_get_uri (PolyhymniaTrackFullInfo *self)
{
  g_return_val_if_fail (POLYHYMNIA_IS_TRACK_FULL_INFO (self), NULL);
  return self->uri;
}
