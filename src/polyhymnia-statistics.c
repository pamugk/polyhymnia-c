
#include "polyhymnia-statistics.h"

/* Type metadata */
typedef enum
{
  PROP_ARTISTS_COUNT = 1,
  PROP_ALBUMS_COUNT,
  PROP_TRACKS_COUNT,
  PROP_MPD_UPTIME,
  PROP_DB_PLAYTIME,
  PROP_DB_LAST_UPDATE,
  PROP_MPD_PLAYTIME,
  N_PROPERTIES,
} PolyhymniaStatisticsProperty;

struct _PolyhymniaStatistics
{
  GObject  parent_instance;

  /* Data */
  guint     artists_count;
  guint     albums_count;
  guint     tracks_count;
  gulong    mpd_uptime;
  gulong    db_playtime;
  GDateTime *db_last_update;
  gulong    mpd_playtime;
};

G_DEFINE_FINAL_TYPE (PolyhymniaStatistics, polyhymnia_statistics, G_TYPE_OBJECT)

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

/* Class stuff */
static void
polyhymnia_statistics_finalize (GObject *gobject)
{
  PolyhymniaStatistics *self = POLYHYMNIA_STATISTICS (gobject);

  g_clear_pointer (&(self->db_last_update), g_date_time_unref);

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
    case PROP_DB_PLAYTIME:
      g_value_set_ulong (value, self->db_playtime);
      break;
    case PROP_DB_LAST_UPDATE:
      g_value_set_object (value, self->db_last_update);
      break;
    case PROP_MPD_PLAYTIME:
      g_value_set_ulong (value, self->mpd_playtime);
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
    case PROP_DB_PLAYTIME:
      self->db_playtime = g_value_get_ulong (value);
      break;
    case PROP_DB_LAST_UPDATE:
      self->db_last_update = g_value_get_object (value);
      break;
    case PROP_MPD_PLAYTIME:
      self->mpd_playtime = g_value_get_ulong (value);
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
    g_param_spec_uint ("albums-count",
                       "Albums count",
                       "Count of distinct albums in DB",
                       0, G_MAXUINT, 0,
                       G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  obj_properties[PROP_TRACKS_COUNT] =
    g_param_spec_uint ("tracks-count", "Tracks count",
                       "Count of tracks in DB",
                       0, G_MAXUINT, 0,
                       G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  obj_properties[PROP_MPD_UPTIME] =
    g_param_spec_ulong ("tracks-count", "Tracks count",
                        "Count of tracks in DB",
                        0, G_MAXULONG, 0,
                        G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  obj_properties[PROP_DB_PLAYTIME] =
    g_param_spec_ulong ("tracks-count", "Tracks count",
                       "Count of tracks in DB",
                       0, G_MAXULONG, 0,
                       G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  obj_properties[PROP_DB_LAST_UPDATE] =
    g_param_spec_object ("db-last-update", "DB last update",
                         "The last time DB was updated",
                         G_TYPE_DATE_TIME,
                         G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  obj_properties[PROP_MPD_PLAYTIME] =
    g_param_spec_ulong ("tracks-count", "Tracks count",
                        "Count of tracks in DB",
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
