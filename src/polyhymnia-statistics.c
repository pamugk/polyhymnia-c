
#include "polyhymnia-statistics.h"

/* Type metadata */
typedef enum
{
  PROP_ARTISTS_COUNT = 1,
  PROP_ALBUMS_COUNT,
  PROP_TRACKS_COUNT,
  PROP_MPD_UPTIME,
  PROP_DB_PLAY_TIME,
  PROP_DB_LAST_UPDATE,
  PROP_MPD_PLAY_TIME,
  N_PROPERTIES,
} PolyhymniaStatisticsProperty;

struct _PolyhymniaStatistics
{
  GObject  parent_instance;

  /* Data */
  guint  artists_count;
  guint  albums_count;
  guint  tracks_count;
  gulong mpd_uptime;
  gulong db_play_time;
  gulong db_last_update;
  gulong mpd_play_time;
};

G_DEFINE_FINAL_TYPE (PolyhymniaStatistics, polyhymnia_statistics, G_TYPE_OBJECT)

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

/* Class stuff */
static void
polyhymnia_statistics_finalize (GObject *gobject)
{
  G_OBJECT_CLASS (polyhymnia_statistics_parent_class)->finalize (gobject);
}

static void
polyhymnia_statistics_get_property (GObject    *object,
                                    guint       property_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
  PolyhymniaStatistics *self = POLYHYMNIA_STATISTICS (object);

  switch ((PolyhymniaStatisticsProperty) property_id)
    {
    case PROP_ARTISTS_COUNT:
      g_value_set_uint (value, self->artists_count);
      break;
    case PROP_ALBUMS_COUNT:
      g_value_set_uint (value, self->albums_count);
      break;
    case PROP_TRACKS_COUNT:
      g_value_set_uint (value, self->tracks_count);
      break;
    case PROP_MPD_UPTIME:
      g_value_set_ulong (value, self->mpd_uptime);
      break;
    case PROP_DB_PLAY_TIME:
      g_value_set_ulong (value, self->db_play_time);
      break;
    case PROP_DB_LAST_UPDATE:
      g_value_set_ulong (value, self->db_last_update);
      break;
    case PROP_MPD_PLAY_TIME:
      g_value_set_ulong (value, self->mpd_play_time);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
polyhymnia_statistics_set_property (GObject      *object,
                                    guint         property_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
  PolyhymniaStatistics *self = POLYHYMNIA_STATISTICS (object);

  switch ((PolyhymniaStatisticsProperty) property_id)
    {
    case PROP_ARTISTS_COUNT:
      self->artists_count = g_value_get_uint (value);
      break;
    case PROP_ALBUMS_COUNT:
      self->albums_count = g_value_get_uint (value);
      break;
    case PROP_TRACKS_COUNT:
      self->tracks_count = g_value_get_uint (value);
      break;
    case PROP_MPD_UPTIME:
      self->mpd_uptime = g_value_get_ulong (value);
      break;
    case PROP_DB_PLAY_TIME:
      self->db_play_time = g_value_get_ulong (value);
      break;
    case PROP_DB_LAST_UPDATE:
      self->db_last_update = g_value_get_ulong (value);
      break;
    case PROP_MPD_PLAY_TIME:
      self->mpd_play_time = g_value_get_ulong (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
polyhymnia_statistics_class_init (PolyhymniaStatisticsClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->finalize = polyhymnia_statistics_finalize;
  gobject_class->get_property = polyhymnia_statistics_get_property;
  gobject_class->set_property = polyhymnia_statistics_set_property;

  obj_properties[PROP_ARTISTS_COUNT] =
    g_param_spec_uint ("artists-count", "Artists count",
                       "Count of distinct artists in DB",
                       0, G_MAXUINT, 0,
                       G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  obj_properties[PROP_ALBUMS_COUNT] =
    g_param_spec_uint ("albums-count", "Albums count",
                       "Count of distinct albums in DB",
                       0, G_MAXUINT, 0,
                       G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  obj_properties[PROP_TRACKS_COUNT] =
    g_param_spec_uint ("tracks-count", "Tracks count",
                       "Count of tracks in DB",
                       0, G_MAXUINT, 0,
                       G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  obj_properties[PROP_MPD_UPTIME] =
    g_param_spec_ulong ("mpd-uptime", "MPD uptime",
                        "Current MPD server uptime",
                        0, G_MAXULONG, 0,
                        G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  obj_properties[PROP_DB_PLAY_TIME] =
    g_param_spec_ulong ("db-play-time", "DB play time",
                       "Total duration (in seconds) of known tracks",
                       0, G_MAXULONG, 0,
                       G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  obj_properties[PROP_DB_LAST_UPDATE] =
    g_param_spec_ulong ("db-last-update", "DB last update",
                        "The last time DB was updated",
                        0, G_MAXULONG, 0,
                        G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  obj_properties[PROP_MPD_PLAY_TIME] =
    g_param_spec_ulong ("mpd-play-time", "MPD play time",
                        "Total duration (in seconds) played by MPD server",
                        0, G_MAXULONG, 0,
                        G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  g_object_class_install_properties (gobject_class,
                                     N_PROPERTIES,
                                     obj_properties);
}

static void
polyhymnia_statistics_init (PolyhymniaStatistics *self)
{
}

/* Instance methods */
guint
polyhymnia_statistics_get_albums_count (PolyhymniaStatistics *self)
{
  g_return_val_if_fail (POLYHYMNIA_IS_STATISTICS (self), 0);
  return self->albums_count;
}

guint
polyhymnia_statistics_get_artists_count (PolyhymniaStatistics *self)
{
  g_return_val_if_fail (POLYHYMNIA_IS_STATISTICS (self), 0);
  return self->artists_count;
}

gulong
polyhymnia_statistics_get_db_play_time (PolyhymniaStatistics *self)
{
  g_return_val_if_fail (POLYHYMNIA_IS_STATISTICS (self), 0);
  return self->db_play_time;
}

gulong
polyhymnia_statistics_get_db_last_update (PolyhymniaStatistics *self)
{
  g_return_val_if_fail (POLYHYMNIA_IS_STATISTICS (self), 0);
  return self->db_last_update;
}

gulong
polyhymnia_statistics_get_mpd_play_time (PolyhymniaStatistics *self)
{
  g_return_val_if_fail (POLYHYMNIA_IS_STATISTICS (self), 0);
  return self->mpd_play_time;
}

gulong
polyhymnia_statistics_get_mpd_uptime (PolyhymniaStatistics *self)
{
  g_return_val_if_fail (POLYHYMNIA_IS_STATISTICS (self), 0);
  return self->mpd_uptime;
}

guint
polyhymnia_statistics_get_tracks_count (PolyhymniaStatistics *self)
{
  g_return_val_if_fail (POLYHYMNIA_IS_STATISTICS (self), 0);
  return self->tracks_count;
}
