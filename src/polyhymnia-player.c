
#include "polyhymnia-enums.h"
#include "polyhymnia-mpd-client-player.h"
#include "polyhymnia-player.h"

/* Type metadata */
typedef enum
{
  PROP_ACTIVE = 1,
  PROP_AUDIO_AVAILABLE,
  PROP_CURRENT_TRACK,
  PROP_ELAPSED_SECONDS,
  PROP_HAS_NEXT,
  PROP_HAS_PREVIOUS,
  PROP_PLAYBACK_STATUS,
  PROP_TOTAL_SECONDS,
  PROP_VOLUME,
  N_PROPERTIES,
} PolyhymniaPlayerProperty;

struct _PolyhymniaPlayer
{
  GObject  parent_instance;

  /* Utility fields */
  PolyhymniaMpdClient *mpd_client;

  /* State fields */
  gboolean                        audio_available;
  PolyhymniaTrack                 *current_track;
  guint                           elapsed_seconds;
  gboolean                        has_next;
  gboolean                        has_previous;
  PolyhymniaPlayerPlaybackStatus  playback_status;
  guint                           volume;
};

G_DEFINE_FINAL_TYPE (PolyhymniaPlayer, polyhymnia_player, G_TYPE_OBJECT)

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

/* Class stuff - constructors, destructors, etc */
static void
polyhymnia_player_constructed (GObject *obj)
{
  G_OBJECT_CLASS (polyhymnia_player_parent_class)->constructed (obj);
}

static GObject*
polyhymnia_player_constructor (GType type,
             guint n_construct_params,
             GObjectConstructParam *construct_params)
{
  static GObject *self = NULL;

  if (self == NULL)
  {
    self = G_OBJECT_CLASS (polyhymnia_player_parent_class)->constructor (
          type, n_construct_params, construct_params);
    g_object_add_weak_pointer (self, (gpointer) &self);
    return self;
  }

  return g_object_ref (self);
}

static void
polyhymnia_player_dispose (GObject *gobject)
{
  PolyhymniaPlayer *self = POLYHYMNIA_PLAYER (gobject);

  g_clear_object (&self->current_track);
  g_clear_object (&self->mpd_client);

  G_OBJECT_CLASS (polyhymnia_player_parent_class)->dispose (gobject);
}

static void
polyhymnia_player_finalize (GObject *gobject)
{
  PolyhymniaPlayer *self = POLYHYMNIA_PLAYER (gobject);

  G_OBJECT_CLASS (polyhymnia_player_parent_class)->finalize (gobject);
}

static void
polyhymnia_player_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  PolyhymniaPlayer *self = POLYHYMNIA_PLAYER (object);

  switch ((PolyhymniaPlayerProperty) property_id)
    {
    case PROP_ACTIVE:
      g_value_set_boolean (value,
                           self->playback_status == POLYHYMNIA_PLAYER_PLAYBACK_STATUS_PLAYING
                           || self->playback_status == POLYHYMNIA_PLAYER_PLAYBACK_STATUS_PAUSED);
      break;
    case PROP_AUDIO_AVAILABLE:
      g_value_set_boolean (value, self->audio_available);
      break;
    case PROP_CURRENT_TRACK:
      g_value_set_object (value, self->current_track);
      break;
    case PROP_ELAPSED_SECONDS:
      g_value_set_uint (value, self->elapsed_seconds);
      break;
    case PROP_HAS_NEXT:
      g_value_set_boolean (value, self->has_next);
      break;
    case PROP_HAS_PREVIOUS:
      g_value_set_boolean (value, self->has_previous);
      break;
    case PROP_PLAYBACK_STATUS:
      g_value_set_enum (value, self->playback_status);
      break;
    case PROP_TOTAL_SECONDS:
      g_value_set_uint (value,
                        self->current_track == NULL
                        ? 0
                        : polyhymnia_track_get_duration (self->current_track));
      break;
    case PROP_VOLUME:
      g_value_set_double (value, self->volume);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
polyhymnia_player_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  PolyhymniaPlayer *self = POLYHYMNIA_PLAYER (object);

  switch ((PolyhymniaPlayerProperty) property_id)
    {
    case PROP_VOLUME:
      self->volume = g_value_get_double (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
polyhymnia_player_class_init (PolyhymniaPlayerClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->constructed  = polyhymnia_player_constructed;
  gobject_class->constructor  = polyhymnia_player_constructor;
  gobject_class->dispose      = polyhymnia_player_dispose;
  gobject_class->finalize     = polyhymnia_player_finalize;
  gobject_class->get_property = polyhymnia_player_get_property;
  gobject_class->set_property = polyhymnia_player_set_property;

  obj_properties[PROP_ACTIVE] =
    g_param_spec_boolean ("active",
                          "Active",
                          "Marks whether player is active.",
                          FALSE,
                          G_PARAM_READABLE | G_PARAM_STATIC_NAME);

  obj_properties[PROP_AUDIO_AVAILABLE] =
    g_param_spec_boolean ("audio-available",
                          "Audio available",
                          "Marks whether audio output is available to MPD server.",
                          FALSE,
                          G_PARAM_READABLE | G_PARAM_STATIC_NAME);

  obj_properties[PROP_CURRENT_TRACK] =
    g_param_spec_object ("current-track",
                         "Current track",
                         "Track that is currently being played.",
                         POLYHYMNIA_TYPE_TRACK,
                         G_PARAM_READABLE | G_PARAM_STATIC_NAME);

  obj_properties[PROP_ELAPSED_SECONDS] =
    g_param_spec_uint ("elapsed-seconds",
                       "Elapsed seconds",
                       "Count of seconds that passed since current track start.",
                       0, G_MAXUINT, 0,
                       G_PARAM_READABLE | G_PARAM_STATIC_NAME);

  obj_properties[PROP_HAS_NEXT] =
    g_param_spec_boolean ("has-next",
                          "Has next",
                          "Marks whether there is next track in a queue.",
                          FALSE,
                          G_PARAM_READABLE | G_PARAM_STATIC_NAME);

  obj_properties[PROP_HAS_PREVIOUS] =
    g_param_spec_boolean ("has-previous",
                          "Has previous",
                          "Marks whether there is previous track in a queue.",
                          FALSE,
                          G_PARAM_READABLE | G_PARAM_STATIC_NAME);

  obj_properties[PROP_PLAYBACK_STATUS] =
    g_param_spec_enum ("playback-status",
                       "Playback status",
                       "State of underlaying MPD player.",
                       POLYHYMNIA_TYPE_PLAYER_PLAYBACK_STATUS,
                       POLYHYMNIA_PLAYER_PLAYBACK_STATUS_UNKNOWN,
                       G_PARAM_READABLE | G_PARAM_STATIC_NAME);

  obj_properties[PROP_TOTAL_SECONDS] =
    g_param_spec_uint ("total-seconds",
                       "Total seconds",
                       "Full duration of a current track (in seconds).",
                       0, G_MAXUINT, 0,
                       G_PARAM_READWRITE | G_PARAM_STATIC_NAME);

  obj_properties[PROP_VOLUME] =
    g_param_spec_double ("volume",
                         "Volume",
                         "Volume of a playback.",
                         0, 100, 100,
                         G_PARAM_READWRITE | G_PARAM_STATIC_NAME);

  g_object_class_install_properties (gobject_class,
                                     N_PROPERTIES,
                                     obj_properties);
}

static void
polyhymnia_player_init (PolyhymniaPlayer *self)
{
  self->mpd_client = g_object_new (POLYHYMNIA_TYPE_MPD_CLIENT, NULL);
  if (polyhymnia_mpd_client_is_initialized (self->mpd_client))
  {
    GError *error = NULL;
    PolyhymniaPlayerState state;
    state = polyhymnia_mpd_client_get_state (self->mpd_client, &error);
    if (error == NULL)
    {
      self->audio_available = state.audio_available;
      self->current_track   = state.current_track;
      self->elapsed_seconds = state.elapsed_seconds;
      self->has_next        = state.has_next;
      self->has_previous    = state.has_previous;
      self->playback_status = state.playback_status;
      self->volume          = state.volume;
    }
    else
    {
      g_warning ("Failed to initialize player state: %s\n", error->message);
      g_error_free (error);
      error = NULL;
    }
  }
}

/* Instance methods */
const PolyhymniaTrack *
polyhymnia_player_get_current_track (const PolyhymniaPlayer *self)
{
  return self->current_track;
}

PolyhymniaPlayerPlaybackStatus
polyhymnia_player_get_playback_status (const PolyhymniaPlayer *self)
{
  return self->playback_status;
}
