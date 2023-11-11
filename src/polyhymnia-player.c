
#include "polyhymnia-enums.h"
#include "polyhymnia-mpd-client-images.h"
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
  PROP_RANDOM_ORDER,
  PROP_REPEAT_PLAYBACK,
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
  gboolean                        random;
  gboolean                        repeat;
  guint                           volume;
};

G_DEFINE_FINAL_TYPE (PolyhymniaPlayer, polyhymnia_player, G_TYPE_OBJECT)

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

/* Event handlers declaration */
static void
polyhymnia_player_mpd_audio_output_changed (PolyhymniaPlayer    * self,
                                            PolyhymniaMpdClient *user_data);

static void
polyhymnia_player_mpd_initialized (PolyhymniaPlayer    * self,
                                   GParamSpec* pspec,
                                   PolyhymniaMpdClient *user_data);

static void
polyhymnia_player_mpd_playback_state_changed (PolyhymniaPlayer    * self,
                                              PolyhymniaMpdClient *user_data);

static void
polyhymnia_player_mpd_options_changed (PolyhymniaPlayer    * self,
                                       PolyhymniaMpdClient *user_data);

static void
polyhymnia_player_mpd_volume_changed (PolyhymniaPlayer    * self,
                                      PolyhymniaMpdClient *user_data);

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
    case PROP_RANDOM_ORDER:
      g_value_set_boolean (value, self->random);
      break;
    case PROP_REPEAT_PLAYBACK:
      g_value_set_boolean (value, self->repeat);
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

  obj_properties[PROP_RANDOM_ORDER] =
    g_param_spec_boolean ("random-order",
                          "Random order",
                          "Marks whether playback is playing in random order.",
                          FALSE,
                          G_PARAM_READABLE | G_PARAM_STATIC_NAME);

  obj_properties[PROP_REPEAT_PLAYBACK] =
    g_param_spec_boolean ("repeat-playback",
                          "Repeat playback",
                          "Marks whether playback is on repeat.",
                          FALSE,
                          G_PARAM_READABLE | G_PARAM_STATIC_NAME);

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

  polyhymnia_player_mpd_initialized (self, NULL, self->mpd_client);

  g_signal_connect_swapped (self->mpd_client, "notify::initialized",
                            G_CALLBACK (polyhymnia_player_mpd_initialized),
                            self);
  g_signal_connect_swapped (self->mpd_client, "player-state-changed",
                            G_CALLBACK (polyhymnia_player_mpd_playback_state_changed),
                            self);
  g_signal_connect_swapped (self->mpd_client, "volume-modified",
                            G_CALLBACK (polyhymnia_player_mpd_volume_changed),
                            self);
  g_signal_connect_swapped (self->mpd_client, "audio-output-changed",
                            G_CALLBACK (polyhymnia_player_mpd_audio_output_changed),
                            self);
  g_signal_connect_swapped (self->mpd_client, "playback-options-changed",
                            G_CALLBACK (polyhymnia_player_mpd_options_changed),
                            self);
}

/* Instance methods */
const PolyhymniaTrack *
polyhymnia_player_get_current_track (const PolyhymniaPlayer *self)
{
  return self->current_track;
}

GBytes *
polyhymnia_player_get_current_track_album_cover (PolyhymniaPlayer *self,
                                                 GError           **error)
{
  g_return_val_if_fail (POLYHYMNIA_IS_PLAYER (self), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);
  g_return_val_if_fail (self->current_track != NULL, NULL);

  return polyhymnia_mpd_client_get_song_album_cover (self->mpd_client,
                                                     polyhymnia_track_get_uri (self->current_track),
                                                     error);
}

guint
polyhymnia_player_get_elapsed (const PolyhymniaPlayer *self)
{
  return self->elapsed_seconds;
}

PolyhymniaPlayerPlaybackStatus
polyhymnia_player_get_playback_status (const PolyhymniaPlayer *self)
{
  return self->playback_status;
}

void
polyhymnia_player_play_next (PolyhymniaPlayer *self,
                             GError           **error)
{
  g_return_if_fail (POLYHYMNIA_IS_PLAYER (self));
  g_return_if_fail (error == NULL || *error == NULL);

  polyhymnia_mpd_client_play_next (self->mpd_client, error);
}

void
polyhymnia_player_play_previous (PolyhymniaPlayer *self,
                                 GError           **error)
{
  g_return_if_fail (POLYHYMNIA_IS_PLAYER (self));
  g_return_if_fail (error == NULL || *error == NULL);

  polyhymnia_mpd_client_play_previous (self->mpd_client, error);
}

void
polyhymnia_player_toggle_playback_state (PolyhymniaPlayer *self,
                                         GError           **error)
{
  g_return_if_fail (POLYHYMNIA_IS_PLAYER (self));
  g_return_if_fail (error == NULL || *error == NULL);

  switch (self->playback_status)
  {
  case POLYHYMNIA_PLAYER_PLAYBACK_STATUS_PAUSED:
    polyhymnia_mpd_client_resume_playback (self->mpd_client, error);
    break;
  case POLYHYMNIA_PLAYER_PLAYBACK_STATUS_PLAYING:
    polyhymnia_mpd_client_pause_playback (self->mpd_client, error);
    break;
  }
}

/* Event handlers implementation */
static void
polyhymnia_player_mpd_audio_output_changed (PolyhymniaPlayer    * self,
                                            PolyhymniaMpdClient *user_data)
{
  g_return_if_fail (POLYHYMNIA_IS_PLAYER (self));
}

static void
polyhymnia_player_mpd_initialized (PolyhymniaPlayer    * self,
                                   GParamSpec* pspec,
                                   PolyhymniaMpdClient *user_data)
{

  g_return_if_fail (POLYHYMNIA_IS_PLAYER (self));

  if (polyhymnia_mpd_client_is_initialized (self->mpd_client))
  {
    GError *error = NULL;
    PolyhymniaPlayerState state;
    state = polyhymnia_mpd_client_get_state (self->mpd_client, &error);
    if (error == NULL)
    {
      self->audio_available = state.audio_available;
      self->current_track   = state.current_track;
      self->elapsed_seconds = state.playback_state.elapsed_seconds;
      self->has_next        = state.playback_state.has_next;
      self->has_previous    = state.playback_state.has_previous;
      self->playback_status = state.playback_state.playback_status;
      self->random          = state.playback_options.random;
      self->repeat          = state.playback_options.repeat;
      self->volume          = state.volume;
    }
    else
    {
      g_warning ("Failed to initialize player state: %s\n", error->message);
      g_error_free (error);
      error = NULL;
    }
  }
  else
  {
    self->audio_available = FALSE;
    g_clear_object (&self->current_track);
    self->elapsed_seconds = 0;
    self->has_next        = FALSE;
    self->has_previous    = FALSE;
    self->playback_status = POLYHYMNIA_PLAYER_PLAYBACK_STATUS_UNKNOWN;
    self->random          = FALSE;
    self->repeat          = FALSE;
    self->volume          = 0;
  }

  if (pspec != NULL)
  {
    for (guint i = 1; i < N_PROPERTIES; i++)
    {
      g_object_notify_by_pspec (G_OBJECT (self), obj_properties[i]);
    }
  }
}

static void
polyhymnia_player_mpd_playback_state_changed (PolyhymniaPlayer    * self,
                                              PolyhymniaMpdClient *user_data)
{
  GError *error = NULL;
  PolyhymniaPlayerPlaybackState new_state;

  g_return_if_fail (POLYHYMNIA_IS_PLAYER (self));

  new_state = polyhymnia_mpd_client_get_playback_state (user_data, &error);
  if (error != NULL)
  {
    g_warning ("Failed to get player playback state update: %s\n", error->message);
    g_error_free (error);
    error = NULL;
    return;
  }

  if (new_state.current_track_id < 0 && self->current_track != NULL)
  {
    g_clear_object (&self->current_track);
    self->elapsed_seconds = 0;
    g_object_notify_by_pspec (G_OBJECT (self), obj_properties[PROP_CURRENT_TRACK]);
  }
  else if (new_state.current_track_id >= 0 && self->current_track == NULL)
  {
    PolyhymniaTrack *new_track;
    new_track = polyhymnia_mpd_client_get_song_from_queue (user_data,
                                                           new_state.current_track_id,
                                                           &error);
    if (error != NULL)
    {
      g_warning ("Failed to get player new current track: %s\n", error->message);
      g_error_free (error);
      error = NULL;
      return;
    }

    self->current_track = new_track;
    self->elapsed_seconds = new_state.elapsed_seconds;
    g_object_notify_by_pspec (G_OBJECT (self), obj_properties[PROP_CURRENT_TRACK]);
  }
  else if (new_state.current_track_id !=
           polyhymnia_track_get_id (self->current_track))
  {
    PolyhymniaTrack *new_track;
    new_track = polyhymnia_mpd_client_get_song_from_queue (user_data,
                                                           new_state.current_track_id,
                                                           &error);
    if (error != NULL)
    {
      g_warning ("Failed to get player new current track: %s\n", error->message);
      g_error_free (error);
      error = NULL;
      return;
    }

    g_clear_object (&self->current_track);
    self->current_track = new_track;

    g_object_notify_by_pspec (G_OBJECT (self), obj_properties[PROP_CURRENT_TRACK]);
  }

  if (new_state.elapsed_seconds != self->elapsed_seconds)
  {
    self->elapsed_seconds = new_state.elapsed_seconds;
    g_object_notify_by_pspec (G_OBJECT (self), obj_properties[PROP_ELAPSED_SECONDS]);
  }
  if (new_state.has_next != self->has_next)
  {
    self->has_next = new_state.has_next;
    g_object_notify_by_pspec (G_OBJECT (self), obj_properties[PROP_HAS_NEXT]);
  }
  if (new_state.has_previous != self->has_previous)
  {
    self->has_previous = new_state.has_previous;
    g_object_notify_by_pspec (G_OBJECT (self), obj_properties[PROP_HAS_PREVIOUS]);
  }
  if (new_state.playback_status != self->playback_status)
  {
    self->playback_status = new_state.playback_status;
    g_object_notify_by_pspec (G_OBJECT (self), obj_properties[PROP_PLAYBACK_STATUS]);
    g_object_notify_by_pspec (G_OBJECT (self), obj_properties[PROP_ACTIVE]);
  }
}

static void
polyhymnia_player_mpd_options_changed (PolyhymniaPlayer    * self,
                                       PolyhymniaMpdClient *user_data)
{
  GError *error = NULL;
  PolyhymniaPlayerPlaybackOptions new_options;

  g_return_if_fail (POLYHYMNIA_IS_PLAYER (self));

  new_options = polyhymnia_mpd_client_get_playback_options (user_data, &error);
  if (error != NULL)
  {
    g_warning ("Failed to get player playback options update: %s\n", error->message);
    g_error_free (error);
    error = NULL;
    return;
  }

  if (new_options.random != self->random)
  {
    self->random = new_options.random;
    g_object_notify_by_pspec (G_OBJECT (self), obj_properties[PROP_RANDOM_ORDER]);
  }
  if (new_options.repeat != self->repeat)
  {
    self->repeat = new_options.repeat;
    g_object_notify_by_pspec (G_OBJECT (self), obj_properties[PROP_REPEAT_PLAYBACK]);
  }
}

static void
polyhymnia_player_mpd_volume_changed (PolyhymniaPlayer    * self,
                                      PolyhymniaMpdClient *user_data)
{
  GError *error = NULL;
  guint new_volume;

  g_return_if_fail (POLYHYMNIA_IS_PLAYER (self));

  new_volume = polyhymnia_mpd_client_get_volume (user_data, &error);
  if (error != NULL)
  {
    g_warning ("Failed to get player volume update: %s\n", error->message);
    g_error_free (error);
    error = NULL;
    return;
  }

  if (new_volume != self->volume)
  {
    self->volume = new_volume;
    g_object_notify_by_pspec (G_OBJECT (self), obj_properties[PROP_VOLUME]);
  }
}
