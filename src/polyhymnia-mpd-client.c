
#include "polyhymnia-mpd-client-api.h"
#include "polyhymnia-mpd-client-core.h"
#include "polyhymnia-mpd-client-details.h"
#include "polyhymnia-mpd-client-images.h"
#include "polyhymnia-mpd-client-outputs.h"
#include "polyhymnia-mpd-client-player.h"
#include "polyhymnia-mpd-client-playlists.h"
#include "polyhymnia-mpd-client-queue.h"
#include "polyhymnia-mpd-client-statistics.h"

#include <mpd/client.h>

/* Type metadata */
typedef enum
{
  PROP_INITIALIZED = 1,
  N_PROPERTIES,
} PolyhymniaMpdClientProperty;

typedef enum
{
  SIGNAL_DATABASE_UPDATED = 1,
  SIGNAL_STORED_PLAYLIST_MODIFIED,
  SIGNAL_QUEUE_MODIFIED,
  SIGNAL_PLAYER_STATE_CHANGED,
  SIGNAL_VOLUME_MODIFIED,
  SIGNAL_AUDIO_OUTPUT_CHANGED,
  SIGNAL_PLAYBACK_OPTIONS_CHANGED,
  SIGNAL_DATABASE_UPDATE_STATE_CHANGED,
  SIGNAL_STICKER_MODIFIED,
  SIGNAL_SUBSCRIPTIONS_CHANGED,
  SIGNAL_MESSAGE_RECEIVED,
  SIGNAL_PARTITIONS_CHANGED,
  SIGNAL_NEIGHBORS_CHANGED,
  SIGNAL_MOUNT_LIST_CHANGED,
  N_SIGNALS,
} PolyhymniaMpdClientSignal;

static const guint
IMAGE_BUFFER_SIZE = 8192;

struct _PolyhymniaMpdClient
{
  GObject  parent_instance;

  /* Underlying MPD fields */
  struct mpd_connection   *main_mpd_connection;
  struct mpd_connection   *idle_mpd_connection;
  GIOChannel              *idle_channel;

  /* State fields */
  gboolean                  initialized;
};

G_DEFINE_FINAL_TYPE (PolyhymniaMpdClient, polyhymnia_mpd_client, G_TYPE_OBJECT)

G_DEFINE_QUARK (PolyhymniaMpdClient, polyhymnia_mpd_client_error);

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

static guint obj_signals[N_SIGNALS] = { 0, };

/* Class stuff - constructors, destructors, etc */
static void
polyhymnia_mpd_client_constructed (GObject *obj)
{
  G_OBJECT_CLASS (polyhymnia_mpd_client_parent_class)->constructed (obj);
}

static GObject*
polyhymnia_mpd_client_constructor (GType type,
             guint n_construct_params,
             GObjectConstructParam *construct_params)
{
  static GObject *self = NULL;

  if (self == NULL)
  {
    self = G_OBJECT_CLASS (polyhymnia_mpd_client_parent_class)->constructor (
          type, n_construct_params, construct_params);
    g_object_add_weak_pointer (self, (gpointer) &self);
    return self;
  }

  return g_object_ref (self);
}

static void
polyhymnia_mpd_client_dispose (GObject *gobject)
{
  PolyhymniaMpdClient *self = POLYHYMNIA_MPD_CLIENT (gobject);

  g_clear_pointer (&self->main_mpd_connection, mpd_connection_free);
  g_clear_pointer (&self->idle_channel, g_io_channel_unref);
  g_clear_pointer (&self->idle_mpd_connection, mpd_connection_free);

  G_OBJECT_CLASS (polyhymnia_mpd_client_parent_class)->dispose (gobject);
}

static void
polyhymnia_mpd_client_finalize (GObject *gobject)
{
  PolyhymniaMpdClient *self = POLYHYMNIA_MPD_CLIENT (gobject);

  G_OBJECT_CLASS (polyhymnia_mpd_client_parent_class)->finalize (gobject);
}

static void
polyhymnia_mpd_client_get_property (GObject    *object,
                          guint       property_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
  PolyhymniaMpdClient *self = POLYHYMNIA_MPD_CLIENT (object);

  switch ((PolyhymniaMpdClientProperty) property_id)
    {
    case PROP_INITIALIZED:
      g_value_set_boolean (value, self->initialized);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
polyhymnia_mpd_client_set_property (GObject      *object,
                          guint         property_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  PolyhymniaMpdClient *self = POLYHYMNIA_MPD_CLIENT (object);

  switch ((PolyhymniaMpdClientProperty) property_id)
    {
    case PROP_INITIALIZED:
      self->initialized = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
polyhymnia_mpd_client_class_init (PolyhymniaMpdClientClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GType type = G_TYPE_FROM_CLASS (gobject_class);

  gobject_class->constructed = polyhymnia_mpd_client_constructed;
  gobject_class->constructor = polyhymnia_mpd_client_constructor;
  gobject_class->dispose = polyhymnia_mpd_client_dispose;
  gobject_class->finalize = polyhymnia_mpd_client_finalize;
  gobject_class->get_property = polyhymnia_mpd_client_get_property;
  gobject_class->set_property = polyhymnia_mpd_client_set_property;

  obj_properties[PROP_INITIALIZED] =
    g_param_spec_boolean ("initialized",
                         "Initialized",
                         "Whether MPD connection is established.",
                         FALSE,
                         G_PARAM_READWRITE | G_PARAM_STATIC_NAME);

  g_object_class_install_properties (gobject_class,
                                     N_PROPERTIES,
                                     obj_properties);

  obj_signals[SIGNAL_DATABASE_UPDATED] =
     g_signal_newv ("database-updated", type,
                    G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                    NULL, NULL, NULL, NULL,
                    G_TYPE_NONE,
                    0, NULL);
  obj_signals[SIGNAL_STORED_PLAYLIST_MODIFIED] =
     g_signal_newv ("stored-playlist-modified", type,
                    G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                    NULL, NULL, NULL, NULL,
                    G_TYPE_NONE,
                    0, NULL);
  obj_signals[SIGNAL_QUEUE_MODIFIED] =
     g_signal_newv ("queue-modified", type,
                    G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                    NULL, NULL, NULL, NULL,
                    G_TYPE_NONE,
                    0, NULL);
  obj_signals[SIGNAL_PLAYER_STATE_CHANGED] =
     g_signal_newv ("player-state-changed", type,
                    G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                    NULL, NULL, NULL, NULL,
                    G_TYPE_NONE,
                    0, NULL);
  obj_signals[SIGNAL_VOLUME_MODIFIED] =
     g_signal_newv ("volume-modified", type,
                    G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                    NULL, NULL, NULL, NULL,
                    G_TYPE_NONE,
                    0, NULL);
  obj_signals[SIGNAL_AUDIO_OUTPUT_CHANGED] =
     g_signal_newv ("audio-output-changed", type,
                    G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                    NULL, NULL, NULL, NULL,
                    G_TYPE_NONE,
                    0, NULL);
  obj_signals[SIGNAL_PLAYBACK_OPTIONS_CHANGED] =
     g_signal_newv ("playback-options-changed", type,
                    G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                    NULL, NULL, NULL, NULL,
                    G_TYPE_NONE,
                    0, NULL);
  obj_signals[SIGNAL_DATABASE_UPDATE_STATE_CHANGED] =
     g_signal_newv ("database-update-state-changed", type,
                    G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                    NULL, NULL, NULL, NULL,
                    G_TYPE_NONE,
                    0, NULL);
  obj_signals[SIGNAL_STICKER_MODIFIED] =
     g_signal_newv ("sticker-modified", type,
                    G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                    NULL, NULL, NULL, NULL,
                    G_TYPE_NONE,
                    0, NULL);
  obj_signals[SIGNAL_SUBSCRIPTIONS_CHANGED] =
     g_signal_newv ("subscriptions-changed", type,
                    G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                    NULL, NULL, NULL, NULL,
                    G_TYPE_NONE,
                    0, NULL);
  obj_signals[SIGNAL_MESSAGE_RECEIVED] =
     g_signal_newv ("message-received", type,
                    G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                    NULL, NULL, NULL, NULL,
                    G_TYPE_NONE,
                    0, NULL);
  obj_signals[SIGNAL_PARTITIONS_CHANGED] =
     g_signal_newv ("partitions-changed", type,
                    G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                    NULL, NULL, NULL, NULL,
                    G_TYPE_NONE,
                    0, NULL);
  obj_signals[SIGNAL_NEIGHBORS_CHANGED] =
     g_signal_newv ("neighbors-changed", type,
                    G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                    NULL, NULL, NULL, NULL,
                    G_TYPE_NONE,
                    0, NULL);
  obj_signals[SIGNAL_MOUNT_LIST_CHANGED] =
     g_signal_newv ("mount-list-changed", type,
                    G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                    NULL, NULL, NULL, NULL,
                    G_TYPE_NONE,
                    0, NULL);
}

static void
polyhymnia_mpd_client_init (PolyhymniaMpdClient *self)
{
  GError *error = NULL;

  self->main_mpd_connection = NULL;
  self->idle_mpd_connection = NULL;
  self->idle_channel = NULL;

  polyhymnia_mpd_client_connect (self, &error);
  if (error != NULL)
  {
    g_warning("MPD client initialization error: %s\n",
              error->message);
    g_error_free (error);
  }
}

/* Utility functions */
static struct mpd_connection *
polyhymnia_mpd_client_connection_init(GError **error)
{
  struct mpd_connection *mpd_connection;
  enum mpd_error mpd_initialization_error;

  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  mpd_connection = mpd_connection_new(NULL, 0, 0);

  if (mpd_connection != NULL)
  {
    mpd_initialization_error = mpd_connection_get_error(mpd_connection);
    if (mpd_initialization_error == MPD_ERROR_SUCCESS)
    {
      const unsigned *mpd_version = mpd_connection_get_server_version (mpd_connection);
      g_debug("Reconnected to MPD %d.%d.%d", mpd_version[0], mpd_version[1], mpd_version[2]);
    }
    else
    {
      g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "%s",
                 mpd_connection_get_error_message(mpd_connection));
      mpd_connection_clear_error (mpd_connection);
      mpd_connection_free (mpd_connection);
      mpd_connection = NULL;
    }
  }
  else
  {
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_OOM,
                 "Out of memory");
  }

  return mpd_connection;
}

/* Private instance methods declaration */
static void
polyhymnia_mpd_client_reconnect_if_necessary (PolyhymniaMpdClient *self,
                                              GError              **error);

/* Private instance methods implementation */
static gboolean
polyhymnia_mpd_client_accept_idle_channel (GIOChannel* source,
                                           GIOCondition condition,
                                           gpointer data)
{
  PolyhymniaMpdClient *self = POLYHYMNIA_MPD_CLIENT (data);

  if (condition == G_IO_IN)
  {
    enum mpd_idle events = mpd_recv_idle (self->idle_mpd_connection, FALSE);

    // Server closed connection, any further activity is undesirable
    if (events == 0)
    {
      g_debug ("MPD server disconnected");
      self->initialized = FALSE;
      g_object_notify_by_pspec (G_OBJECT (self), obj_properties[PROP_INITIALIZED]);

      g_clear_pointer (&(self->main_mpd_connection), mpd_connection_free);
      g_clear_pointer (&(self->idle_channel), g_io_channel_unref);
      g_clear_pointer (&(self->idle_mpd_connection), mpd_connection_free);

      return FALSE;
    }

    if (events & MPD_IDLE_DATABASE)
    {
      g_debug ("MPD: song database has been updated\n");
      g_signal_emit (self, obj_signals[SIGNAL_DATABASE_UPDATED], 0);
    }
    if (events & MPD_IDLE_STORED_PLAYLIST)
    {
      g_debug ("MPD: a stored playlist has been modified, created, deleted or renamed\n");
      g_signal_emit (self, obj_signals[SIGNAL_STORED_PLAYLIST_MODIFIED], 0);
    }
    if (events & MPD_IDLE_QUEUE)
    {
      g_debug ("MPD: the queue has been modified\n");
      g_signal_emit (self, obj_signals[SIGNAL_QUEUE_MODIFIED], 0);
    }
    if (events & MPD_IDLE_PLAYER)
    {
      g_debug ("MPD: the player state has changed (play, stop, pause, seek, etc)\n");
      g_signal_emit (self, obj_signals[SIGNAL_PLAYER_STATE_CHANGED], 0);
    }
    if (events & MPD_IDLE_MIXER)
    {
      g_debug ("MPD: the volume has been modified \n");
      g_signal_emit (self, obj_signals[SIGNAL_VOLUME_MODIFIED], 0);
    }
    if (events & MPD_IDLE_OUTPUT)
    {
      g_debug ("MPD: an audio output device has been enabled or disabled\n");
      g_signal_emit (self, obj_signals[SIGNAL_AUDIO_OUTPUT_CHANGED], 0);
    }
    if (events & MPD_IDLE_OPTIONS)
    {
      g_debug ("MPD: options have changed (crossfade, random, repeat, etc)\n");
      g_signal_emit (self, obj_signals[SIGNAL_PLAYBACK_OPTIONS_CHANGED], 0);
    }
    if (events & MPD_IDLE_UPDATE)
    {
      g_debug ("MPD: a database update has started or finished\n");
      g_signal_emit (self, obj_signals[SIGNAL_DATABASE_UPDATE_STATE_CHANGED], 0);
    }
    if (events & MPD_IDLE_STICKER)
    {
      g_debug ("MPD: a sticker has been modified\n");
      g_signal_emit (self, obj_signals[SIGNAL_STICKER_MODIFIED], 0);
    }
    if (events & MPD_IDLE_SUBSCRIPTION)
    {
      g_debug ("MPD: a client has subscribed to or unsubscribed from a channel\n");
      g_signal_emit (self, obj_signals[SIGNAL_SUBSCRIPTIONS_CHANGED], 0);
    }
    if (events & MPD_IDLE_MESSAGE)
    {
      g_debug ("MPD: a message on a subscribed channel was received\n");
      g_signal_emit (self, obj_signals[SIGNAL_MESSAGE_RECEIVED], 0);
    }
    if (events & MPD_IDLE_PARTITION)
    {
      g_debug ("MPD: a partition was added or changed\n");
      g_signal_emit (self, obj_signals[SIGNAL_PARTITIONS_CHANGED], 0);
    }
    if (events & MPD_IDLE_NEIGHBOR)
    {
      g_debug ("MPD: a neighbor was found or lost\n");
      g_signal_emit (self, obj_signals[SIGNAL_NEIGHBORS_CHANGED], 0);
    }
    if (events & MPD_IDLE_MOUNT)
    {
      g_debug ("MPD: the mount list has changed\n");
      g_signal_emit (self, obj_signals[SIGNAL_MOUNT_LIST_CHANGED], 0);
    }

    mpd_send_idle (self->idle_mpd_connection);
    return TRUE;
  }

  return FALSE;
}

static void
polyhymnia_mpd_client_append_anything_to_queue(PolyhymniaMpdClient *self,
                                               const gchar         *filter,
                                               enum  mpd_tag_type  filter_tag,
                                               GError              **error)
{
  GError *inner_error = NULL;

  g_return_if_fail (POLYHYMNIA_IS_MPD_CLIENT (self));
  g_return_if_fail (error == NULL || *error == NULL);
  g_return_if_fail (self->main_mpd_connection != NULL);
  g_return_if_fail (filter != NULL && !g_str_equal (filter, ""));

  polyhymnia_mpd_client_reconnect_if_necessary (self, &inner_error);
  if (inner_error != NULL)
  {
    g_propagate_error (error, inner_error);
    return;
  }

  if (!mpd_search_add_db_songs (self->main_mpd_connection, TRUE))
  {
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "request failed - %s",
                 mpd_connection_get_error_message(self->main_mpd_connection));
    mpd_connection_clear_error (self->main_mpd_connection);
    return;
  }
  if (!mpd_search_add_tag_constraint (self->main_mpd_connection,
                                      MPD_OPERATOR_DEFAULT,
                                      filter_tag,
                                      filter))
  {
    mpd_search_cancel (self->main_mpd_connection);
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "filter failed - %s",
                 mpd_connection_get_error_message(self->main_mpd_connection));
    mpd_connection_clear_error (self->main_mpd_connection);
    return;
  }
  if (!mpd_search_commit (self->main_mpd_connection))
  {
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "start failed - %s",
                 mpd_connection_get_error_message(self->main_mpd_connection));
    mpd_connection_clear_error (self->main_mpd_connection);
    return;
  }
  if (mpd_connection_get_error(self->main_mpd_connection) != MPD_ERROR_SUCCESS
      || !mpd_response_finish(self->main_mpd_connection))
  {
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "cleanup failed - %s",
                 mpd_connection_get_error_message(self->main_mpd_connection));
    mpd_connection_clear_error (self->main_mpd_connection);
  }
}

static void
polyhymnia_mpd_client_connect_idle(PolyhymniaMpdClient *self)
{
  gint fd;
  GError *inner_error = NULL;

  self->idle_mpd_connection = polyhymnia_mpd_client_connection_init (&inner_error);
  if (inner_error != NULL)
  {
    g_warning ("MPD client event loop initialization error: %s\n",
              inner_error->message);
    g_error_free (inner_error);
    return;
  }

  if (!mpd_send_idle(self->idle_mpd_connection))
  {
    g_warning ("MPD send idle failed: %s",
             mpd_connection_get_error_message(self->idle_mpd_connection));
    mpd_connection_clear_error (self->idle_mpd_connection);
    mpd_connection_free (self->idle_mpd_connection);
    self->idle_mpd_connection = NULL;
    return;
  }

  fd = mpd_connection_get_fd(self->idle_mpd_connection);

  self->idle_channel = g_io_channel_unix_new (fd);
  g_io_channel_set_encoding (self->idle_channel, NULL, NULL);
  g_io_add_watch (self->idle_channel, G_IO_IN | G_IO_HUP,
                  polyhymnia_mpd_client_accept_idle_channel,
                  self);
}

static void
polyhymnia_mpd_client_reconnect_if_necessary (PolyhymniaMpdClient *self,
                                              GError              **error)
{
  // Check on MPD and try to reconnect when needed.
  if (!mpd_send_command (self->main_mpd_connection, "ping", NULL)
      || !mpd_response_finish(self->main_mpd_connection))
  {
    // Destroy currently invalid connection
    g_clear_pointer (&(self->main_mpd_connection), mpd_connection_free);
    // Try to open new connection
    self->main_mpd_connection = polyhymnia_mpd_client_connection_init (error);
    // Failed reconnection means that something is wrong.
    // So let's cleanup all other resources and let user figure it out.
    if (self->main_mpd_connection == NULL)
    {
      g_clear_pointer (&(self->idle_channel), g_io_channel_unref);
      g_clear_pointer (&(self->idle_mpd_connection), mpd_connection_free);
      self->initialized = FALSE;
      g_object_notify_by_pspec (G_OBJECT (self), obj_properties[PROP_INITIALIZED]);
    }
  }
}

/* Instance methods */
gint
polyhymnia_mpd_client_add_next_to_queue(PolyhymniaMpdClient *self,
                                        const gchar         *song_uri,
                                        GError              **error)
{
  gint id;
  GError *inner_error = NULL;

  g_return_val_if_fail (POLYHYMNIA_IS_MPD_CLIENT (self), 0);
  g_return_val_if_fail (error == NULL || *error == NULL, 0);
  g_return_val_if_fail (self->main_mpd_connection != NULL, 0);

  polyhymnia_mpd_client_reconnect_if_necessary (self, &inner_error);
  if (inner_error != NULL)
  {
    g_propagate_error (error, inner_error);
    return 0;
  }

  id = mpd_run_add_id_whence (self->main_mpd_connection, song_uri,
                              0, MPD_POSITION_AFTER_CURRENT);
  if (id == -1)
  {
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "failed - %s",
                 mpd_connection_get_error_message(self->main_mpd_connection));
    mpd_connection_clear_error (self->main_mpd_connection);
  }

  return id;
}

void
polyhymnia_mpd_client_append_playlist_to_queue (PolyhymniaMpdClient *self,
                                                const gchar         *name,
                                                GError              **error)
{
  GError *inner_error = NULL;

  g_return_if_fail (POLYHYMNIA_IS_MPD_CLIENT (self));
  g_return_if_fail (error == NULL || *error == NULL);
  g_return_if_fail (self->main_mpd_connection != NULL);
  g_return_if_fail (name != NULL);

  polyhymnia_mpd_client_reconnect_if_necessary (self, &inner_error);
  if (inner_error != NULL)
  {
    g_propagate_error (error, inner_error);
    return;
  }

  if (!mpd_run_load (self->main_mpd_connection, name))
  {
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "failed - %s",
                 mpd_connection_get_error_message(self->main_mpd_connection));
    mpd_connection_clear_error (self->main_mpd_connection);
  }
}

void
polyhymnia_mpd_client_add_to_playlist (PolyhymniaMpdClient *self,
                                       const gchar         *name,
                                       const gchar         *uri,
                                       GError              **error)
{
  GError *inner_error = NULL;

  g_return_if_fail (POLYHYMNIA_IS_MPD_CLIENT (self));
  g_return_if_fail (error == NULL || *error == NULL);
  g_return_if_fail (self->main_mpd_connection != NULL);
  g_return_if_fail (name != NULL);
  g_return_if_fail (uri != NULL);

  polyhymnia_mpd_client_reconnect_if_necessary (self, &inner_error);
  if (inner_error != NULL)
  {
    g_propagate_error (error, inner_error);
    return;
  }

  if (!mpd_run_playlist_add (self->main_mpd_connection, name, uri))
  {
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "failed - %s",
                 mpd_connection_get_error_message(self->main_mpd_connection));
    mpd_connection_clear_error (self->main_mpd_connection);
  }
}

void
polyhymnia_mpd_client_append_album_to_queue(PolyhymniaMpdClient *self,
                                            const gchar         *album,
                                            GError              **error)
{
  polyhymnia_mpd_client_append_anything_to_queue (self,
                                                  album, MPD_TAG_ALBUM,
                                                  error);
}

void
polyhymnia_mpd_client_append_artist_to_queue(PolyhymniaMpdClient *self,
                                             const gchar         *artist,
                                             GError              **error)
{
  polyhymnia_mpd_client_append_anything_to_queue (self,
                                                  artist, MPD_TAG_ALBUM_ARTIST,
                                                  error);
}

gint
polyhymnia_mpd_client_append_song_to_queue(PolyhymniaMpdClient *self,
                                           const gchar         *song_uri,
                                           GError              **error)
{
  gint id;
  GError *inner_error = NULL;

  g_return_val_if_fail (POLYHYMNIA_IS_MPD_CLIENT (self), -1);
  g_return_val_if_fail (error == NULL || *error == NULL, -1);
  g_return_val_if_fail (self->main_mpd_connection != NULL, -1);

  polyhymnia_mpd_client_reconnect_if_necessary (self, &inner_error);
  if (inner_error != NULL)
  {
    g_propagate_error (error, inner_error);
    return -1;
  }

  id = mpd_run_add_id(self->main_mpd_connection, song_uri);
  if (id == -1)
  {
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "failed - %s",
                 mpd_connection_get_error_message(self->main_mpd_connection));
    mpd_connection_clear_error (self->main_mpd_connection);
  }

  return id;
}

void
polyhymnia_mpd_client_append_songs_to_queue(PolyhymniaMpdClient *self,
                                            GPtrArray           *songs_uri,
                                            GError              **error)
{
  GError *inner_error = NULL;

  g_return_if_fail (POLYHYMNIA_IS_MPD_CLIENT (self));
  g_return_if_fail (error == NULL || *error == NULL);
  g_return_if_fail (self->main_mpd_connection != NULL);

  polyhymnia_mpd_client_reconnect_if_necessary (self, &inner_error);
  if (inner_error != NULL)
  {
    g_propagate_error (error, inner_error);
    return;
  }

  mpd_command_list_begin(self->main_mpd_connection, FALSE);
  for (guint i = 0; i < songs_uri->len; i++)
  {
    mpd_send_add (self->main_mpd_connection, g_ptr_array_index (songs_uri, i));
  }
  mpd_command_list_end(self->main_mpd_connection);

  if (!mpd_response_finish (self->main_mpd_connection))
  {
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "failed - %s",
                 mpd_connection_get_error_message(self->main_mpd_connection));
    mpd_connection_clear_error (self->main_mpd_connection);
  }
}

void
polyhymnia_mpd_client_change_volume(PolyhymniaMpdClient *self,
                                    gint8               volume_diff,
                                    GError              **error)
{
  GError *inner_error = NULL;

  g_return_if_fail (POLYHYMNIA_IS_MPD_CLIENT (self));
  g_return_if_fail (error == NULL || *error == NULL);
  g_return_if_fail (self->main_mpd_connection != NULL);

  polyhymnia_mpd_client_reconnect_if_necessary (self, &inner_error);
  if (inner_error != NULL)
  {
    g_propagate_error (error, inner_error);
    return;
  }

  if (!mpd_run_change_volume (self->main_mpd_connection, volume_diff))
  {
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "failed - %s",
                 mpd_connection_get_error_message(self->main_mpd_connection));
    mpd_connection_clear_error (self->main_mpd_connection);
  }
}

void
polyhymnia_mpd_client_clear_queue(PolyhymniaMpdClient *self,
                                  GError             **error)
{
  GError *inner_error = NULL;

  g_return_if_fail (POLYHYMNIA_IS_MPD_CLIENT (self));
  g_return_if_fail (error == NULL || *error == NULL);
  g_return_if_fail (self->main_mpd_connection != NULL);

  polyhymnia_mpd_client_reconnect_if_necessary (self, &inner_error);
  if (inner_error != NULL)
  {
    g_propagate_error (error, inner_error);
    return;
  }

  if (!mpd_run_clear(self->main_mpd_connection))
  {
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "failed - %s",
                 mpd_connection_get_error_message(self->main_mpd_connection));
    mpd_connection_clear_error (self->main_mpd_connection);
  }
}

void
polyhymnia_mpd_client_clear_playlist (PolyhymniaMpdClient *self,
                                      const gchar         *name,
                                      GError              **error)
{
  GError *inner_error = NULL;

  g_return_if_fail (POLYHYMNIA_IS_MPD_CLIENT (self));
  g_return_if_fail (error == NULL || *error == NULL);
  g_return_if_fail (self->main_mpd_connection != NULL);
  g_return_if_fail (name != NULL);

  polyhymnia_mpd_client_reconnect_if_necessary (self, &inner_error);
  if (inner_error != NULL)
  {
    g_propagate_error (error, inner_error);
    return;
  }

  if (!mpd_run_playlist_clear (self->main_mpd_connection, name))
  {
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "failed - %s",
                 mpd_connection_get_error_message(self->main_mpd_connection));
    mpd_connection_clear_error (self->main_mpd_connection);
  }
}

void
polyhymnia_mpd_client_connect(PolyhymniaMpdClient *self,
                              GError              **error)
{
  GError *inner_error = NULL;

  g_return_if_fail (POLYHYMNIA_IS_MPD_CLIENT (self));
  g_return_if_fail (error == NULL || *error == NULL);
  g_return_if_fail (self->main_mpd_connection == NULL);

  self->main_mpd_connection = polyhymnia_mpd_client_connection_init (&inner_error);
  if (inner_error == NULL)
  {
    polyhymnia_mpd_client_connect_idle (self);
  }
  else
  {
    g_propagate_error(error, inner_error);
  }

  self->initialized = self->main_mpd_connection != NULL;
  g_object_notify_by_pspec (G_OBJECT (self), obj_properties[PROP_INITIALIZED]);
}

void
polyhymnia_mpd_client_delete_from_queue(PolyhymniaMpdClient *self,
                                        guint               id,
                                        GError              **error)
{
  GError *inner_error = NULL;

  g_return_if_fail (POLYHYMNIA_IS_MPD_CLIENT (self));
  g_return_if_fail (error == NULL || *error == NULL);
  g_return_if_fail (self->main_mpd_connection != NULL);

  polyhymnia_mpd_client_reconnect_if_necessary (self, &inner_error);
  if (inner_error != NULL)
  {
    g_propagate_error (error, inner_error);
    return;
  }

  if (!mpd_run_delete_id (self->main_mpd_connection, id))
  {
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "failed - %s",
                 mpd_connection_get_error_message(self->main_mpd_connection));
    mpd_connection_clear_error (self->main_mpd_connection);
  }
}

void
polyhymnia_mpd_client_delete_playlist (PolyhymniaMpdClient *self,
                                       const gchar         *name,
                                       GError              **error)
{
  GError *inner_error = NULL;

  g_return_if_fail (POLYHYMNIA_IS_MPD_CLIENT (self));
  g_return_if_fail (error == NULL || *error == NULL);
  g_return_if_fail (self->main_mpd_connection != NULL);
  g_return_if_fail (name != NULL);

  polyhymnia_mpd_client_reconnect_if_necessary (self, &inner_error);
  if (inner_error != NULL)
  {
    g_propagate_error (error, inner_error);
    return;
  }

  if (!mpd_run_rm (self->main_mpd_connection, name))
  {
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "failed - %s",
                 mpd_connection_get_error_message(self->main_mpd_connection));
    mpd_connection_clear_error (self->main_mpd_connection);
  }
}

void
polyhymnia_mpd_client_delete_songs_from_queue(PolyhymniaMpdClient *self,
                                              GArray              *ids,
                                              GError              **error)
{
  GError *inner_error = NULL;

  g_return_if_fail (POLYHYMNIA_IS_MPD_CLIENT (self));
  g_return_if_fail (error == NULL || *error == NULL);
  g_return_if_fail (self->main_mpd_connection != NULL);

  polyhymnia_mpd_client_reconnect_if_necessary (self, &inner_error);
  if (inner_error != NULL)
  {
    g_propagate_error (error, inner_error);
    return;
  }

  mpd_command_list_begin(self->main_mpd_connection, FALSE);
  for (guint i = 0; i < ids->len; i++)
  {
    mpd_send_delete_id (self->main_mpd_connection, g_array_index (ids, guint, i));
  }
  mpd_command_list_end(self->main_mpd_connection);

  if (!mpd_response_finish (self->main_mpd_connection))
  {
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "failed - %s",
                 mpd_connection_get_error_message(self->main_mpd_connection));
    mpd_connection_clear_error (self->main_mpd_connection);
  }
}

GPtrArray *
polyhymnia_mpd_client_get_album_tracks(PolyhymniaMpdClient *self,
                                       const gchar         *album,
                                       GError              **error)
{
  GError *inner_error = NULL;
  struct mpd_song *track;
  GPtrArray *results;

  g_return_val_if_fail (POLYHYMNIA_IS_MPD_CLIENT (self), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);
  g_return_val_if_fail (album != NULL, NULL);
  g_return_val_if_fail (self->main_mpd_connection != NULL, NULL);

  polyhymnia_mpd_client_reconnect_if_necessary (self, &inner_error);
  if (inner_error != NULL)
  {
    g_propagate_error (error, inner_error);
    return NULL;
  }

  if (!mpd_search_db_songs (self->main_mpd_connection, TRUE))
  {
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "search initialization failed - %s",
                 mpd_connection_get_error_message(self->main_mpd_connection));
    mpd_connection_clear_error (self->main_mpd_connection);
    return NULL;
  }
  if (!mpd_search_add_tag_constraint (self->main_mpd_connection,
                                      MPD_OPERATOR_DEFAULT,
                                      MPD_TAG_ALBUM,
                                      album))
  {
    mpd_search_cancel (self->main_mpd_connection);
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "search filter failed - %s",
                 mpd_connection_get_error_message(self->main_mpd_connection));
    mpd_connection_clear_error (self->main_mpd_connection);
    return NULL;
  }
  if (!mpd_search_add_sort_tag (self->main_mpd_connection, MPD_TAG_DISC, FALSE))
  {
    mpd_search_cancel (self->main_mpd_connection);
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "search sort failed - %s",
                 mpd_connection_get_error_message(self->main_mpd_connection));
    mpd_connection_clear_error (self->main_mpd_connection);
    return NULL;
  }
  if (!mpd_search_commit (self->main_mpd_connection))
  {
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "search start failed - %s",
                 mpd_connection_get_error_message(self->main_mpd_connection));
    mpd_connection_clear_error (self->main_mpd_connection);
    return NULL;
  }

  results = g_ptr_array_new ();
  g_ptr_array_set_free_func (results, g_object_unref);
  while ((track = mpd_recv_song(self->main_mpd_connection)) != NULL)
  {
    const gchar *title = mpd_song_get_tag (track, MPD_TAG_TITLE, 0);
    if (title != NULL && !g_str_equal (title, ""))
    {
      const gchar *album_position = mpd_song_get_tag (track, MPD_TAG_TRACK, 0);
      const gchar *album_artist = mpd_song_get_tag (track, MPD_TAG_ALBUM_ARTIST, 0);
      const gchar *artist = mpd_song_get_tag (track, MPD_TAG_ARTIST, 0);
      const gchar *date = mpd_song_get_tag (track, MPD_TAG_DATE, 0);
            gchar *date_parsed = NULL;
      const gchar *disc = mpd_song_get_tag (track, MPD_TAG_DISC, 0);
      const gchar *original_date = mpd_song_get_tag (track, MPD_TAG_ORIGINAL_DATE, 0);
      guint64 disc_number = 0;
      GObject *track_object;

      if (date != NULL)
      {
        GDateTime *release_date = g_date_time_new_from_iso8601 (date, NULL);
        if (release_date != NULL)
        {
          gint year = g_date_time_get_year (release_date);
          date_parsed = g_strdup_printf ("%d", year);
          date = date_parsed;
          g_date_time_unref (release_date);
        }
      }

      if (disc != NULL)
      {
        g_ascii_string_to_unsigned (disc, 10, 0, G_MAXUINT, &disc_number, NULL);
      }

      track_object = g_object_new (POLYHYMNIA_TYPE_TRACK,
                                            "uri", mpd_song_get_uri (track),
                                            "title", title,
                                            "disc", (guint) disc_number,
                                            "album-position", album_position,
                                            "album-artist", album_artist,
                                            "artist",
                                            g_strcmp0 (album_artist, artist) == 0
                                              ? NULL : artist,
                                            "date", date,
                                            "original-date", original_date,
                                            "duration", mpd_song_get_duration (track),
                                            NULL);
      g_ptr_array_add(results, track_object);

      g_free (date_parsed);
    }
    mpd_song_free(track);
  }

  if (mpd_connection_get_error(self->main_mpd_connection) != MPD_ERROR_SUCCESS
      || !mpd_response_finish(self->main_mpd_connection))
  {
    g_ptr_array_free (results, TRUE);
    results = NULL;
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "cleanup failed - %s",
                 mpd_connection_get_error_message(self->main_mpd_connection));
    mpd_connection_clear_error (self->main_mpd_connection);
  }

  return results;
}

GPtrArray *
polyhymnia_mpd_client_get_artist_discography(PolyhymniaMpdClient *self,
                                             const gchar         *artist,
                                             GError              **error)
{
  GError *inner_error = NULL;
  struct mpd_song *track;
  GPtrArray *results;

  polyhymnia_mpd_client_reconnect_if_necessary (self, &inner_error);
  if (inner_error != NULL)
  {
    g_propagate_error (error, inner_error);
    return NULL;
  }

  g_return_val_if_fail (POLYHYMNIA_IS_MPD_CLIENT (self), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);
  g_return_val_if_fail (artist != NULL, NULL);
  g_return_val_if_fail (self->main_mpd_connection != NULL, NULL);

  if (!mpd_search_db_songs (self->main_mpd_connection, TRUE))
  {
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "search initialization failed - %s",
                 mpd_connection_get_error_message(self->main_mpd_connection));
    mpd_connection_clear_error (self->main_mpd_connection);
    return NULL;
  }
  if (!mpd_search_add_tag_constraint (self->main_mpd_connection,
                                      MPD_OPERATOR_DEFAULT,
                                      MPD_TAG_ALBUM_ARTIST,
                                      artist))
  {
    mpd_search_cancel (self->main_mpd_connection);
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "search filter failed - %s",
                 mpd_connection_get_error_message(self->main_mpd_connection));
    mpd_connection_clear_error (self->main_mpd_connection);
    return NULL;
  }
  if (!mpd_search_add_sort_tag (self->main_mpd_connection, MPD_TAG_ALBUM_SORT, FALSE))
  {
    mpd_search_cancel (self->main_mpd_connection);
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "search sort failed - %s",
                 mpd_connection_get_error_message(self->main_mpd_connection));
    mpd_connection_clear_error (self->main_mpd_connection);
    return NULL;
  }
  if (!mpd_search_commit (self->main_mpd_connection))
  {
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "search start failed - %s",
                 mpd_connection_get_error_message(self->main_mpd_connection));
    mpd_connection_clear_error (self->main_mpd_connection);
    return NULL;
  }

  results = g_ptr_array_new ();
  g_ptr_array_set_free_func (results, g_object_unref);
  while ((track = mpd_recv_song(self->main_mpd_connection)) != NULL)
  {
    const gchar *title = mpd_song_get_tag (track, MPD_TAG_TITLE, 0);
    if (title != NULL && !g_str_equal (title, ""))
    {
      const gchar *album = mpd_song_get_tag (track, MPD_TAG_ALBUM, 0);
      const gchar *album_sort = mpd_song_get_tag (track, MPD_TAG_ALBUM_SORT, 0);
      const gchar *album_position = mpd_song_get_tag (track, MPD_TAG_TRACK, 0);
      const gchar *album_artist = mpd_song_get_tag (track, MPD_TAG_ALBUM_ARTIST, 0);
      const gchar *track_artist = mpd_song_get_tag (track, MPD_TAG_ARTIST, 0);
      const gchar *date = mpd_song_get_tag (track, MPD_TAG_DATE, 0);
            gchar *date_parsed = NULL;
      const gchar *disc = mpd_song_get_tag (track, MPD_TAG_DISC, 0);
      const gchar *original_date = mpd_song_get_tag (track, MPD_TAG_ORIGINAL_DATE, 0);
      guint64 disc_number = 0;
      GObject *track_object;

      if (date != NULL)
      {
        GDateTime *release_date = g_date_time_new_from_iso8601 (date, NULL);
        if (release_date != NULL)
        {
          gint year = g_date_time_get_year (release_date);
          date_parsed = g_strdup_printf ("%d", year);
          date = date_parsed;
          g_date_time_unref (release_date);
        }
      }

      if (disc != NULL)
      {
        g_ascii_string_to_unsigned (disc, 10, 0, G_MAXUINT, &disc_number, NULL);
      }

      track_object = g_object_new (POLYHYMNIA_TYPE_TRACK,
                                   "uri", mpd_song_get_uri (track),
                                   "title", title,
                                   "album", album,
                                   "album-sort", album_sort,
                                   "disc", (guint) disc_number,
                                   "album-position", album_position,
                                   "album-artist", album_artist,
                                   "artist",
                                   g_strcmp0 (album_artist, track_artist) == 0
                                   ? NULL : track_artist,
                                   "date", date,
                                   "original-date", original_date,
                                   "duration", mpd_song_get_duration (track),
                                   NULL);
      g_ptr_array_add(results, track_object);

      g_free (date_parsed);
    }
    mpd_song_free(track);
  }

  if (mpd_connection_get_error(self->main_mpd_connection) != MPD_ERROR_SUCCESS
      || !mpd_response_finish(self->main_mpd_connection))
  {
    g_ptr_array_free (results, TRUE);
    results = NULL;
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "cleanup failed - %s",
                 mpd_connection_get_error_message(self->main_mpd_connection));
    mpd_connection_clear_error (self->main_mpd_connection);
  }

  return results;
}

GPtrArray *
polyhymnia_mpd_client_get_outputs(PolyhymniaMpdClient *self,
                                  GError              **error)
{
  struct mpd_output *output;
  GError *inner_error = NULL;
  GPtrArray *results;

  g_return_val_if_fail (POLYHYMNIA_IS_MPD_CLIENT (self), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);
  g_return_val_if_fail (self->main_mpd_connection != NULL, NULL);

  polyhymnia_mpd_client_reconnect_if_necessary (self, &inner_error);
  if (inner_error != NULL)
  {
    g_propagate_error (error, inner_error);
    return NULL;
  }

  if (!mpd_send_outputs (self->main_mpd_connection))
  {
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "request failed - %s",
                 mpd_connection_get_error_message(self->main_mpd_connection));
    mpd_connection_clear_error (self->main_mpd_connection);
    return NULL;
  }

  results = g_ptr_array_new ();
  g_ptr_array_set_free_func (results, g_object_unref);

  while ((output = mpd_recv_output (self->main_mpd_connection)) != NULL)
  {
    GObject *track_object = g_object_new (POLYHYMNIA_TYPE_OUTPUT,
                                          "id", mpd_output_get_id (output),
                                          "name", mpd_output_get_name (output),
                                          "plugin", mpd_output_get_plugin (output),
                                          "enabled", mpd_output_get_enabled (output),
                                          NULL);
    g_ptr_array_add(results, track_object);
    mpd_output_free (output);
  }

  if (mpd_connection_get_error(self->main_mpd_connection) != MPD_ERROR_SUCCESS
      || !mpd_response_finish(self->main_mpd_connection))
  {
    g_ptr_array_free (results, TRUE);
    results = NULL;
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "cleanup failed - %s",
                 mpd_connection_get_error_message(self->main_mpd_connection));
    mpd_connection_clear_error (self->main_mpd_connection);
  }

  return results;
}

PolyhymniaPlayerPlaybackOptions
polyhymnia_mpd_client_get_playback_options(PolyhymniaMpdClient *self,
                                           GError              **error)
{
  GError *inner_error = NULL;
  PolyhymniaPlayerPlaybackOptions state = {};
  struct mpd_status     *status;

  g_return_val_if_fail (POLYHYMNIA_IS_MPD_CLIENT (self), state);
  g_return_val_if_fail (error == NULL || *error == NULL, state);
  g_return_val_if_fail (self->main_mpd_connection != NULL, state);

  polyhymnia_mpd_client_reconnect_if_necessary (self, &inner_error);
  if (inner_error != NULL)
  {
    g_propagate_error (error, inner_error);
    return state;
  }

  status = mpd_run_status (self->main_mpd_connection);
  if (status == NULL)
  {
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "request failed - %s",
                 mpd_connection_get_error_message (self->main_mpd_connection));
    mpd_connection_clear_error (self->main_mpd_connection);
    return state;
  }

  state.random = mpd_status_get_random (status);
  state.repeat = mpd_status_get_repeat (status);

  mpd_status_free (status);

  return state;
}

PolyhymniaPlayerPlaybackState
polyhymnia_mpd_client_get_playback_state(PolyhymniaMpdClient *self,
                                         GError              **error)
{
  GError *inner_error = NULL;
  PolyhymniaPlayerPlaybackState state = {};
  struct mpd_status     *status;

  g_return_val_if_fail (POLYHYMNIA_IS_MPD_CLIENT (self), state);
  g_return_val_if_fail (error == NULL || *error == NULL, state);
  g_return_val_if_fail (self->main_mpd_connection != NULL, state);

  polyhymnia_mpd_client_reconnect_if_necessary (self, &inner_error);
  if (inner_error != NULL)
  {
    g_propagate_error (error, inner_error);
    return state;
  }

  status = mpd_run_status (self->main_mpd_connection);
  if (status == NULL)
  {
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "request failed - %s",
                 mpd_connection_get_error_message (self->main_mpd_connection));
    mpd_connection_clear_error (self->main_mpd_connection);
    return state;
  }

  state.current_track_id = mpd_status_get_song_id (status);
  state.elapsed_seconds  = mpd_status_get_elapsed_ms (status) / 1000;
  state.has_next         = mpd_status_get_next_song_id (status) != -1;
  state.has_previous     = mpd_status_get_song_pos (status) > 0;
  state.playback_status  = (PolyhymniaPlayerPlaybackStatus) mpd_status_get_state (status);

  mpd_status_free (status);

  return state;
}

GPtrArray *
polyhymnia_mpd_client_get_playlist_tracks (PolyhymniaMpdClient *self,
                                           const gchar         *name,
                                           GError              **error)
{
  struct mpd_entity *entity;
  GError *inner_error = NULL;
  GPtrArray *results;

  g_return_val_if_fail (POLYHYMNIA_IS_MPD_CLIENT (self), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);
  g_return_val_if_fail (self->main_mpd_connection != NULL, NULL);
  g_return_val_if_fail (name != NULL, NULL);

  polyhymnia_mpd_client_reconnect_if_necessary (self, &inner_error);
  if (inner_error != NULL)
  {
    g_propagate_error (error, inner_error);
    return NULL;
  }

  if (!mpd_send_list_playlist_meta (self->main_mpd_connection, name))
  {
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "request failed - %s",
                 mpd_connection_get_error_message(self->main_mpd_connection));
    mpd_connection_clear_error (self->main_mpd_connection);
    return NULL;
  }

  results = g_ptr_array_new ();
  g_ptr_array_set_free_func (results, g_object_unref);

  while ((entity = mpd_recv_entity (self->main_mpd_connection)) != NULL)
  {
    if (mpd_entity_get_type (entity) == MPD_ENTITY_TYPE_SONG)
    {
      const struct mpd_song *track = mpd_entity_get_song (entity);
      const gchar *title = mpd_song_get_tag (track, MPD_TAG_TITLE, 0);
      const gchar *album = mpd_song_get_tag (track, MPD_TAG_ALBUM, 0);
      const gchar *artist = mpd_song_get_tag (track, MPD_TAG_ARTIST, 0);
      GObject *track_object = g_object_new (POLYHYMNIA_TYPE_TRACK,
                                            "uri", mpd_song_get_uri (track),
                                            "title", title,
                                            "album", album,
                                            "artist", artist,
                                            "duration", mpd_song_get_duration (track),
                                            NULL);
      g_ptr_array_add(results, track_object);
    }
    mpd_entity_free (entity);
  }

  if (mpd_connection_get_error(self->main_mpd_connection) != MPD_ERROR_SUCCESS
      || !mpd_response_finish(self->main_mpd_connection))
  {
    g_ptr_array_free (results, TRUE);
    results = NULL;
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "cleanup failed - %s",
                 mpd_connection_get_error_message(self->main_mpd_connection));
    mpd_connection_clear_error (self->main_mpd_connection);
  }

  return results;
}

GPtrArray *
polyhymnia_mpd_client_get_queue(PolyhymniaMpdClient *self,
                                GError              **error)
{
  struct mpd_entity *entity;
  GError *inner_error = NULL;
  GPtrArray *results;

  g_return_val_if_fail (POLYHYMNIA_IS_MPD_CLIENT (self), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);
  g_return_val_if_fail (self->main_mpd_connection != NULL, NULL);

  polyhymnia_mpd_client_reconnect_if_necessary (self, &inner_error);
  if (inner_error != NULL)
  {
    g_propagate_error (error, inner_error);
    return NULL;
  }

  if (!mpd_send_list_queue_meta(self->main_mpd_connection))
  {
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "request failed - %s",
                 mpd_connection_get_error_message(self->main_mpd_connection));
    mpd_connection_clear_error (self->main_mpd_connection);
    return NULL;
  }

  results = g_ptr_array_new ();
  g_ptr_array_set_free_func (results, g_object_unref);

  while ((entity = mpd_recv_entity (self->main_mpd_connection)) != NULL)
  {
    if (mpd_entity_get_type (entity) == MPD_ENTITY_TYPE_SONG)
    {
      const struct mpd_song *track = mpd_entity_get_song (entity);
      const gchar *title = mpd_song_get_tag (track, MPD_TAG_TITLE, 0);
      const gchar *album = mpd_song_get_tag (track, MPD_TAG_ALBUM, 0);
      const gchar *artist = mpd_song_get_tag (track, MPD_TAG_ARTIST, 0);
      GObject *track_object = g_object_new (POLYHYMNIA_TYPE_TRACK,
                                            "id", mpd_song_get_id (track),
                                            "queue-position", mpd_song_get_pos (track),
                                            "uri", mpd_song_get_uri (track),
                                            "title", title,
                                            "album", album,
                                            "artist", artist,
                                            "duration", mpd_song_get_duration (track),
                                            NULL);
      g_ptr_array_add(results, track_object);
    }
    mpd_entity_free (entity);
  }

  if (mpd_connection_get_error(self->main_mpd_connection) != MPD_ERROR_SUCCESS
      || !mpd_response_finish(self->main_mpd_connection))
  {
    g_ptr_array_free (results, TRUE);
    results = NULL;
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "cleanup failed - %s",
                 mpd_connection_get_error_message(self->main_mpd_connection));
    mpd_connection_clear_error (self->main_mpd_connection);
  }

  return results;
}

GBytes *
polyhymnia_mpd_client_get_song_album_cover(PolyhymniaMpdClient *self,
                                           const gchar         *song_uri,
                                           GError              **error)
{
  GByteArray *cover_array;
  GError *inner_error = NULL;

  g_return_val_if_fail (POLYHYMNIA_IS_MPD_CLIENT (self), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);
  g_return_val_if_fail (self->main_mpd_connection != NULL, NULL);

  polyhymnia_mpd_client_reconnect_if_necessary (self, &inner_error);
  if (inner_error != NULL)
  {
    g_propagate_error (error, inner_error);
    return NULL;
  }

  cover_array = g_byte_array_sized_new (IMAGE_BUFFER_SIZE * 115);
  for (guint offset = 0; ; offset += IMAGE_BUFFER_SIZE)
  {
    guint8 buffer[IMAGE_BUFFER_SIZE];
    int read_size = mpd_run_readpicture (self->main_mpd_connection,
                                         song_uri,
                                         offset,
                                         buffer,
                                         IMAGE_BUFFER_SIZE);
    if (read_size > 0)
    {
      g_byte_array_append (cover_array, buffer, read_size);
      if (read_size < IMAGE_BUFFER_SIZE)
      {
        break;
      }
    }
    else if (read_size == 0)
    {
      break;
    }
    else
    {
      enum mpd_error read_error = mpd_connection_get_error (self->main_mpd_connection);
      if (read_error != MPD_ERROR_SUCCESS)
      {
        // If a server error occurred, let's pretend that
        // cover size in bytes is divisible by buffer size,
        // so client didn't stop sending requests in time.
        if (read_error != MPD_ERROR_SERVER
            || mpd_connection_get_server_error (self->main_mpd_connection) != MPD_SERVER_ERROR_ARG)
        {
          g_set_error (error,
                       POLYHYMNIA_MPD_CLIENT_ERROR,
                       POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                       "failed to read portion of cover image - %s",
                       mpd_connection_get_error_message(self->main_mpd_connection));
          g_byte_array_unref (cover_array);
          cover_array = NULL;
        }
        mpd_connection_clear_error (self->main_mpd_connection);
      }
      break;
    }
  }

  return cover_array == NULL ? NULL : g_byte_array_free_to_bytes (cover_array);
}

PolyhymniaTrackFullInfo *
polyhymnia_mpd_client_get_song_details (PolyhymniaMpdClient *self,
                                        const gchar         *song_uri,
                                        GError              **error)
{
  GError                  *inner_error = NULL;
  PolyhymniaTrackFullInfo *song_object;
  struct mpd_song         *song;

  g_return_val_if_fail (POLYHYMNIA_IS_MPD_CLIENT (self), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);
  g_return_val_if_fail (self->main_mpd_connection != NULL, NULL);

  polyhymnia_mpd_client_reconnect_if_necessary (self, &inner_error);
  if (inner_error != NULL)
  {
    g_propagate_error (error, inner_error);
    return NULL;
  }

  if (!mpd_search_db_songs (self->main_mpd_connection, TRUE))
  {
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "search initialization failed - %s",
                 mpd_connection_get_error_message(self->main_mpd_connection));
    mpd_connection_clear_error (self->main_mpd_connection);
    return NULL;
  }
  if (!mpd_search_add_uri_constraint (self->main_mpd_connection,
                                      MPD_OPERATOR_DEFAULT,
                                      song_uri))
  {
    mpd_search_cancel (self->main_mpd_connection);
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "search filter failed - %s",
                 mpd_connection_get_error_message(self->main_mpd_connection));
    mpd_connection_clear_error (self->main_mpd_connection);
    return NULL;
  }
  if (!mpd_search_commit (self->main_mpd_connection))
  {
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "search start failed - %s",
                 mpd_connection_get_error_message(self->main_mpd_connection));
    mpd_connection_clear_error (self->main_mpd_connection);
    return NULL;
  }
  while ((song = mpd_recv_song(self->main_mpd_connection)) != NULL
          && song_object == NULL)
  {
    song_object = g_object_new (POLYHYMNIA_TYPE_TRACK_FULL_INFO,
                                "album",
                                mpd_song_get_tag (song, MPD_TAG_ALBUM, 0),
                                "album-artist",
                                mpd_song_get_tag (song, MPD_TAG_ALBUM_ARTIST, 0),
                                "title",
                                mpd_song_get_tag (song, MPD_TAG_TITLE, 0),
                                "uri", mpd_song_get_uri (song),
                                NULL);
    mpd_song_free(song);
  }

  if (mpd_connection_get_error(self->main_mpd_connection) != MPD_ERROR_SUCCESS
      || !mpd_response_finish(self->main_mpd_connection))
  {
    g_clear_object (&song_object);
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "cleanup failed - %s",
                 mpd_connection_get_error_message(self->main_mpd_connection));
    mpd_connection_clear_error (self->main_mpd_connection);
  }

  return NULL;
}

PolyhymniaTrack *
polyhymnia_mpd_client_get_song_from_queue(PolyhymniaMpdClient *self,
                                          guint               id,
                                          GError              **error)
{
  GError *inner_error = NULL;
  PolyhymniaTrack *song_object;
  struct mpd_song *song;

  g_return_val_if_fail (POLYHYMNIA_IS_MPD_CLIENT (self), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);
  g_return_val_if_fail (self->main_mpd_connection != NULL, NULL);

  polyhymnia_mpd_client_reconnect_if_necessary (self, &inner_error);
  if (inner_error != NULL)
  {
    g_propagate_error (error, inner_error);
    return NULL;
  }

  song = mpd_run_get_queue_song_id (self->main_mpd_connection, id);

  if (song == NULL)
  {
    g_set_error (error,
                  POLYHYMNIA_MPD_CLIENT_ERROR,
                  POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                  "current track request failed - %s",
                  mpd_connection_get_error_message (self->main_mpd_connection));
    mpd_connection_clear_error (self->main_mpd_connection);
    return NULL;
  }
  else
  {
    const gchar *album = mpd_song_get_tag (song, MPD_TAG_ALBUM, 0);
    const gchar *artist = mpd_song_get_tag (song, MPD_TAG_ARTIST, 0);
    const gchar *title = mpd_song_get_tag (song, MPD_TAG_TITLE, 0);
    song_object = g_object_new (POLYHYMNIA_TYPE_TRACK,
                                "id", mpd_song_get_id (song),
                                "queue-position", mpd_song_get_pos (song),
                                "uri", mpd_song_get_uri (song),
                                "title", title,
                                "album", album,
                                "artist", artist,
                                "duration", mpd_song_get_duration (song),
                                NULL);
    mpd_song_free (song);
  }

  return song_object;
}

PolyhymniaPlayerState
polyhymnia_mpd_client_get_state(PolyhymniaMpdClient *self,
                                 GError              **error)
{
  GError *inner_error = NULL;
  PolyhymniaPlayerState state = {};
  struct mpd_status     *status;

  g_return_val_if_fail (POLYHYMNIA_IS_MPD_CLIENT (self), state);
  g_return_val_if_fail (error == NULL || *error == NULL, state);
  g_return_val_if_fail (self->main_mpd_connection != NULL, state);

  polyhymnia_mpd_client_reconnect_if_necessary (self, &inner_error);
  if (inner_error != NULL)
  {
    g_propagate_error (error, inner_error);
    return state;
  }

  status = mpd_run_status (self->main_mpd_connection);
  if (status == NULL)
  {
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "main request failed - %s",
                 mpd_connection_get_error_message (self->main_mpd_connection));
    mpd_connection_clear_error (self->main_mpd_connection);
  }
  else
  {
    PolyhymniaTrack *current_track = NULL;
    int             current_track_id = mpd_status_get_song_id (status);
    int             volume = mpd_status_get_volume (status);

    if (current_track_id != -1)
    {
      current_track = polyhymnia_mpd_client_get_song_from_queue (self, current_track_id, error);
    }

    state.playback_state.current_track_id = current_track_id;
    state.current_track                   = current_track;
    state.playback_state.elapsed_seconds  = mpd_status_get_elapsed_ms (status) / 1000;
    state.playback_state.has_next         = mpd_status_get_next_song_id (status) != -1;
    state.playback_state.has_previous     = mpd_status_get_song_pos (status) > 0;
    state.playback_state.playback_status  = (PolyhymniaPlayerPlaybackStatus) mpd_status_get_state (status);

    state.playback_options.random        = mpd_status_get_random (status);
    state.playback_options.repeat        = mpd_status_get_repeat (status);

    if (volume >= 0)
    {
      state.audio_available = TRUE;
      state.volume          = volume;
    }
    else
    {
      state.audio_available = FALSE;
      state.volume          = 0;
    }

    mpd_status_free (status);
  }

  return state;
}

PolyhymniaStatistics *
polyhymnia_mpd_client_get_statistics (PolyhymniaMpdClient *self,
                                      GError              **error)
{
  GError               *inner_error = NULL;
  struct mpd_stats     *mpd_statistics;
  PolyhymniaStatistics *statistics = NULL;

  g_return_val_if_fail (POLYHYMNIA_IS_MPD_CLIENT (self), statistics);
  g_return_val_if_fail (error == NULL || *error == NULL, statistics);
  g_return_val_if_fail (self->main_mpd_connection != NULL, statistics);

  polyhymnia_mpd_client_reconnect_if_necessary (self, &inner_error);
  if (inner_error != NULL)
  {
    g_propagate_error (error, inner_error);
    return statistics;
  }

  mpd_statistics = mpd_run_stats (self->main_mpd_connection);

  if (mpd_statistics == NULL)
  {
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "failed - %s",
                 mpd_connection_get_error_message(self->main_mpd_connection));
    mpd_connection_clear_error (self->main_mpd_connection);
  }
  else
  {
    statistics = g_object_new (POLYHYMNIA_TYPE_STATISTICS,
                               "artists-count",
                               mpd_stats_get_number_of_artists (mpd_statistics),
                               "albums-count",
                               mpd_stats_get_number_of_albums (mpd_statistics),
                               "tracks-count",
                               mpd_stats_get_number_of_songs (mpd_statistics),
                               "mpd-uptime",
                               mpd_stats_get_uptime (mpd_statistics),
                               "db-play-time",
                               mpd_stats_get_db_play_time (mpd_statistics),
                               "db-last-update",
                               mpd_stats_get_db_update_time (mpd_statistics),
                               "mpd-play-time",
                               mpd_stats_get_play_time (mpd_statistics),
                               NULL);
    mpd_stats_free (mpd_statistics);
  }

  return statistics;
}

guint
polyhymnia_mpd_client_get_volume(PolyhymniaMpdClient *self,
                                 GError              **error)
{
  GError *inner_error = NULL;
  int response;
  guint volume = 0;

  g_return_val_if_fail (POLYHYMNIA_IS_MPD_CLIENT (self), 0);
  g_return_val_if_fail (error == NULL || *error == NULL, 0);
  g_return_val_if_fail (self->main_mpd_connection != NULL, 0);

  polyhymnia_mpd_client_reconnect_if_necessary (self, &inner_error);
  if (inner_error != NULL)
  {
    g_propagate_error (error, inner_error);
    return 0;
  }

  response = mpd_run_get_volume (self->main_mpd_connection);
  /* Somehow -1 does also mean disabled output, and no error is registered */
  if (response == -1 && mpd_connection_get_error (self->main_mpd_connection) != MPD_ERROR_SUCCESS)
  {
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "failed - %s",
                 mpd_connection_get_error_message(self->main_mpd_connection));
    mpd_connection_clear_error (self->main_mpd_connection);
  }
  else
  {
    volume = response >= 0 ? response : 0;
  }

  return volume;
}

gboolean
polyhymnia_mpd_client_is_initialized (PolyhymniaMpdClient *self)
{
  g_return_val_if_fail (POLYHYMNIA_IS_MPD_CLIENT (self), FALSE);
  return self->initialized;
}

void
polyhymnia_mpd_client_pause_playback(PolyhymniaMpdClient *self,
                                     GError              **error)
{
  GError *inner_error = NULL;

  g_return_if_fail (POLYHYMNIA_IS_MPD_CLIENT (self));
  g_return_if_fail (error == NULL || *error == NULL);
  g_return_if_fail (self->main_mpd_connection != NULL);

  polyhymnia_mpd_client_reconnect_if_necessary (self, &inner_error);
  if (inner_error != NULL)
  {
    g_propagate_error (error, inner_error);
    return;
  }

  if (!mpd_run_pause(self->main_mpd_connection, TRUE))
  {
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "failed - %s",
                 mpd_connection_get_error_message(self->main_mpd_connection));
    mpd_connection_clear_error (self->main_mpd_connection);
  }
}

void
polyhymnia_mpd_client_play (PolyhymniaMpdClient *self,
                            GError              **error)
{
  GError *inner_error = NULL;

  g_return_if_fail (POLYHYMNIA_IS_MPD_CLIENT (self));
  g_return_if_fail (error == NULL || *error == NULL);
  g_return_if_fail (self->main_mpd_connection != NULL);

  polyhymnia_mpd_client_reconnect_if_necessary (self, &inner_error);
  if (inner_error != NULL)
  {
    g_propagate_error (error, inner_error);
    return;
  }

  if (!mpd_run_play (self->main_mpd_connection))
  {
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "failed - %s",
                 mpd_connection_get_error_message(self->main_mpd_connection));
    mpd_connection_clear_error (self->main_mpd_connection);
  }
}

void
polyhymnia_mpd_client_play_album(PolyhymniaMpdClient *self,
                                 const gchar         *album,
                                 GError              **error)
{
  GError *inner_error = NULL;

  polyhymnia_mpd_client_reconnect_if_necessary (self, &inner_error);
  if (inner_error != NULL)
  {
    g_propagate_error (error, inner_error);
    return;
  }

  polyhymnia_mpd_client_clear_queue (self, &inner_error);
  if (inner_error != NULL)
  {
    g_propagate_error(error, inner_error);
    return;
  }

  polyhymnia_mpd_client_append_album_to_queue (self, album, &inner_error);
  if (inner_error == NULL)
  {
    polyhymnia_mpd_client_play (self, &inner_error);
  }

  if (inner_error != NULL)
  {
    g_propagate_error(error, inner_error);
  }
}

void
polyhymnia_mpd_client_play_artist(PolyhymniaMpdClient *self,
                                  const gchar         *artist,
                                  GError              **error)
{
  GError *inner_error = NULL;

  polyhymnia_mpd_client_clear_queue (self, &inner_error);
  if (inner_error != NULL)
  {
    g_propagate_error(error, inner_error);
    return;
  }

  polyhymnia_mpd_client_append_artist_to_queue (self, artist, &inner_error);
  if (inner_error == NULL)
  {
    polyhymnia_mpd_client_play (self, &inner_error);
  }

  if (inner_error != NULL)
  {
    g_propagate_error(error, inner_error);
  }
}

void
polyhymnia_mpd_client_play_next(PolyhymniaMpdClient *self,
                                GError              **error)
{
  GError *inner_error = NULL;

  g_return_if_fail (POLYHYMNIA_IS_MPD_CLIENT (self));
  g_return_if_fail (error == NULL || *error == NULL);
  g_return_if_fail (self->main_mpd_connection != NULL);

  polyhymnia_mpd_client_reconnect_if_necessary (self, &inner_error);
  if (inner_error != NULL)
  {
    g_propagate_error (error, inner_error);
    return;
  }

  if (!mpd_run_next (self->main_mpd_connection))
  {
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "failed - %s",
                 mpd_connection_get_error_message(self->main_mpd_connection));
    mpd_connection_clear_error (self->main_mpd_connection);
  }
}

void
polyhymnia_mpd_client_play_playlist (PolyhymniaMpdClient *self,
                                     const gchar         *name,
                                     GError              **error)
{
  GError *inner_error = NULL;

  polyhymnia_mpd_client_reconnect_if_necessary (self, &inner_error);
  if (inner_error != NULL)
  {
    g_propagate_error (error, inner_error);
    return;
  }

  polyhymnia_mpd_client_clear_queue (self, &inner_error);
  if (inner_error != NULL)
  {
    g_propagate_error(error, inner_error);
    return;
  }

  polyhymnia_mpd_client_append_playlist_to_queue (self, name, &inner_error);
  if (inner_error == NULL)
  {
    polyhymnia_mpd_client_play (self, &inner_error);
  }

  if (inner_error != NULL)
  {
    g_propagate_error(error, inner_error);
  }
}

void
polyhymnia_mpd_client_play_previous(PolyhymniaMpdClient *self,
                                    GError              **error)
{
  GError *inner_error = NULL;

  g_return_if_fail (POLYHYMNIA_IS_MPD_CLIENT (self));
  g_return_if_fail (error == NULL || *error == NULL);
  g_return_if_fail (self->main_mpd_connection != NULL);

  polyhymnia_mpd_client_reconnect_if_necessary (self, &inner_error);
  if (inner_error != NULL)
  {
    g_propagate_error (error, inner_error);
    return;
  }

  if (!mpd_run_previous (self->main_mpd_connection))
  {
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "failed - %s",
                 mpd_connection_get_error_message(self->main_mpd_connection));
    mpd_connection_clear_error (self->main_mpd_connection);
  }
}

gint
polyhymnia_mpd_client_play_song(PolyhymniaMpdClient *self,
                                const gchar         *song_uri,
                                GError              **error)
{
  GError *inner_error = NULL;
  int id = -1;

  polyhymnia_mpd_client_clear_queue (self, &inner_error);
  if (inner_error != NULL)
  {
    g_propagate_error(error, inner_error);
    return id;
  }

  id = polyhymnia_mpd_client_append_song_to_queue (self, song_uri, &inner_error);
  if (inner_error == NULL)
  {
    polyhymnia_mpd_client_play (self, &inner_error);
  }

  if (inner_error != NULL)
  {
    g_propagate_error(error, inner_error);
  }

  return id;
}

void
polyhymnia_mpd_client_play_songs(PolyhymniaMpdClient *self,
                                 GPtrArray           *songs_uri,
                                 GError              **error)
{
  GError *inner_error = NULL;

  polyhymnia_mpd_client_clear_queue (self, &inner_error);
  if (inner_error != NULL)
  {
    g_propagate_error(error, inner_error);
    return;
  }

  polyhymnia_mpd_client_append_songs_to_queue (self, songs_uri, &inner_error);
  if (inner_error == NULL)
  {
    polyhymnia_mpd_client_play (self, &inner_error);
  }

  if (inner_error != NULL)
  {
    g_propagate_error(error, inner_error);
  }
}

void
polyhymnia_mpd_client_play_song_from_queue(PolyhymniaMpdClient *self,
                                           guint               id,
                                           GError              **error)
{
  GError *inner_error = NULL;

  g_return_if_fail (POLYHYMNIA_IS_MPD_CLIENT (self));
  g_return_if_fail (error == NULL || *error == NULL);
  g_return_if_fail (self->main_mpd_connection != NULL);

  polyhymnia_mpd_client_reconnect_if_necessary (self, &inner_error);
  if (inner_error != NULL)
  {
    g_propagate_error (error, inner_error);
    return;
  }

  if (!mpd_run_play_id (self->main_mpd_connection, id))
  {
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "failed - %s",
                 mpd_connection_get_error_message(self->main_mpd_connection));
    mpd_connection_clear_error (self->main_mpd_connection);
  }
}

void
polyhymnia_mpd_client_rename_playlist (PolyhymniaMpdClient *self,
                                       const gchar         *old_name,
                                       const gchar         *new_name,
                                       GError              **error)
{
  GError *inner_error = NULL;

  g_return_if_fail (POLYHYMNIA_IS_MPD_CLIENT (self));
  g_return_if_fail (error == NULL || *error == NULL);
  g_return_if_fail (self->main_mpd_connection != NULL);
  g_return_if_fail (old_name != NULL);
  g_return_if_fail (new_name != NULL);

  polyhymnia_mpd_client_reconnect_if_necessary (self, &inner_error);
  if (inner_error != NULL)
  {
    g_propagate_error (error, inner_error);
    return;
  }

  if (!mpd_run_rename (self->main_mpd_connection, old_name, new_name))
  {
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "failed - %s",
                 mpd_connection_get_error_message(self->main_mpd_connection));
    mpd_connection_clear_error (self->main_mpd_connection);
  }
}

void
polyhymnia_mpd_client_resume_playback(PolyhymniaMpdClient *self,
                                      GError              **error)
{
  GError *inner_error = NULL;

  g_return_if_fail (POLYHYMNIA_IS_MPD_CLIENT (self));
  g_return_if_fail (error == NULL || *error == NULL);
  g_return_if_fail (self->main_mpd_connection != NULL);

  polyhymnia_mpd_client_reconnect_if_necessary (self, &inner_error);
  if (inner_error != NULL)
  {
    g_propagate_error (error, inner_error);
    return;
  }

  if (!mpd_run_pause (self->main_mpd_connection, FALSE))
  {
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "failed - %s",
                 mpd_connection_get_error_message(self->main_mpd_connection));
    mpd_connection_clear_error (self->main_mpd_connection);
  }
}

void
polyhymnia_mpd_client_save_queue_as_playlist (PolyhymniaMpdClient *self,
                                              const gchar         *name,
                                              GError              **error)
{
  GError *inner_error = NULL;

  g_return_if_fail (POLYHYMNIA_IS_MPD_CLIENT (self));
  g_return_if_fail (error == NULL || *error == NULL);
  g_return_if_fail (self->main_mpd_connection != NULL);

  polyhymnia_mpd_client_reconnect_if_necessary (self, &inner_error);
  if (inner_error != NULL)
  {
    g_propagate_error (error, inner_error);
    return;
  }

  if (!mpd_run_save (self->main_mpd_connection, name))
  {
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "failed - %s",
                 mpd_connection_get_error_message(self->main_mpd_connection));
    mpd_connection_clear_error (self->main_mpd_connection);
  }
}

void
polyhymnia_mpd_client_rescan(PolyhymniaMpdClient *self,
                             GError              **error)
{
  GError *inner_error = NULL;
  guint scan_job_id;

  g_return_if_fail (POLYHYMNIA_IS_MPD_CLIENT (self));
  g_return_if_fail (error == NULL || *error == NULL);
  g_return_if_fail (self->main_mpd_connection != NULL);

  polyhymnia_mpd_client_reconnect_if_necessary (self, &inner_error);
  if (inner_error != NULL)
  {
    g_propagate_error (error, inner_error);
    return;
  }

  scan_job_id = mpd_run_rescan (self->main_mpd_connection, NULL);
  if (scan_job_id == 0)
  {
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "failed - %s",
                 mpd_connection_get_error_message(self->main_mpd_connection));
    mpd_connection_clear_error (self->main_mpd_connection);
  }
}

void
polyhymnia_mpd_client_scan(PolyhymniaMpdClient *self,
                           GError              **error)
{
  GError *inner_error = NULL;
  guint scan_job_id;

  g_return_if_fail (POLYHYMNIA_IS_MPD_CLIENT (self));
  g_return_if_fail (error == NULL || *error == NULL);
  g_return_if_fail (self->main_mpd_connection != NULL);

  polyhymnia_mpd_client_reconnect_if_necessary (self, &inner_error);
  if (inner_error != NULL)
  {
    g_propagate_error (error, inner_error);
    return;
  }

  scan_job_id = mpd_run_update (self->main_mpd_connection, NULL);
  if (scan_job_id == 0)
  {
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "failed - %s",
                 mpd_connection_get_error_message(self->main_mpd_connection));
    mpd_connection_clear_error (self->main_mpd_connection);
  }
}

GPtrArray *
polyhymnia_mpd_client_search_albums(PolyhymniaMpdClient *self,
                                    GError              **error)
{
  GError *inner_error = NULL;
  struct mpd_pair *pair;
  GPtrArray * results;

  g_return_val_if_fail (POLYHYMNIA_IS_MPD_CLIENT (self), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);
  g_return_val_if_fail (self->main_mpd_connection != NULL, NULL);

  polyhymnia_mpd_client_reconnect_if_necessary (self, &inner_error);
  if (inner_error != NULL)
  {
    g_propagate_error (error, inner_error);
    return NULL;
  }

  if (!mpd_search_db_tags (self->main_mpd_connection, MPD_TAG_ALBUM))
  {
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "search initialization failed - %s",
                 mpd_connection_get_error_message(self->main_mpd_connection));
    mpd_connection_clear_error (self->main_mpd_connection);
    return NULL;
  }
  if (!mpd_search_commit (self->main_mpd_connection))
  {
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "search start failed - %s",
                 mpd_connection_get_error_message(self->main_mpd_connection));
    mpd_connection_clear_error (self->main_mpd_connection);
    return NULL;
  }

  results = g_ptr_array_new ();
  g_ptr_array_set_free_func (results, g_object_unref);
  while ((pair = mpd_recv_pair_tag(self->main_mpd_connection,
				    MPD_TAG_ALBUM)) != NULL)
  {
    if (pair->value != NULL && !g_str_equal (pair->value, ""))
    {
      GObject *album_object = g_object_new (POLYHYMNIA_TYPE_ALBUM,
                                            "title", pair->value,
                                             NULL);
      g_ptr_array_add (results, album_object);
    }
    mpd_return_pair(self->main_mpd_connection, pair);
  }

  if (mpd_connection_get_error(self->main_mpd_connection) != MPD_ERROR_SUCCESS
      || !mpd_response_finish(self->main_mpd_connection))
  {
    g_ptr_array_free (results, TRUE);
    results = NULL;
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "cleanup failed - %s",
                 mpd_connection_get_error_message(self->main_mpd_connection));
    mpd_connection_clear_error (self->main_mpd_connection);
  }

  return results;
}

GPtrArray *
polyhymnia_mpd_client_search_artists(PolyhymniaMpdClient *self,
                                      GError              **error)
{
  GError *inner_error = NULL;
  struct mpd_pair *pair;
  GPtrArray * results;

  g_return_val_if_fail (POLYHYMNIA_IS_MPD_CLIENT (self), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);
  g_return_val_if_fail (self->main_mpd_connection != NULL, NULL);

  polyhymnia_mpd_client_reconnect_if_necessary (self, &inner_error);
  if (inner_error != NULL)
  {
    g_propagate_error (error, inner_error);
    return NULL;
  }

  if (!mpd_search_db_tags (self->main_mpd_connection, MPD_TAG_ALBUM_ARTIST))
  {
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "search initialization failed - %s",
                 mpd_connection_get_error_message(self->main_mpd_connection));
    mpd_connection_clear_error (self->main_mpd_connection);
    return NULL;
  }
  if (!mpd_search_commit (self->main_mpd_connection))
  {
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "search start failed - %s",
                 mpd_connection_get_error_message(self->main_mpd_connection));
    mpd_connection_clear_error (self->main_mpd_connection);
    return NULL;
  }

  results = g_ptr_array_new ();
  g_ptr_array_set_free_func (results, g_object_unref);
  while ((pair = mpd_recv_pair_tag(self->main_mpd_connection,
				    MPD_TAG_ALBUM_ARTIST)) != NULL)
  {
    if (pair->value != NULL && !g_str_equal (pair->value, ""))
    {
      GObject *artist_object = g_object_new (POLYHYMNIA_TYPE_ARTIST,
                                            "name", pair->value,
                                             NULL);
      g_ptr_array_add (results, artist_object);
    }
    mpd_return_pair(self->main_mpd_connection, pair);
  }

  if (mpd_connection_get_error(self->main_mpd_connection) != MPD_ERROR_SUCCESS
      || !mpd_response_finish(self->main_mpd_connection))
  {
    g_ptr_array_free (results, TRUE);
    results = NULL;
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "cleanup failed - %s",
                 mpd_connection_get_error_message(self->main_mpd_connection));
    mpd_connection_clear_error (self->main_mpd_connection);
  }

  return results;
}

GPtrArray *
polyhymnia_mpd_client_search_genres(PolyhymniaMpdClient *self,
                                    GError              **error)
{
  GError *inner_error = NULL;
  struct mpd_pair *pair;
  GPtrArray * results;

  g_return_val_if_fail (POLYHYMNIA_IS_MPD_CLIENT (self), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);
  g_return_val_if_fail (self->main_mpd_connection != NULL, NULL);

  polyhymnia_mpd_client_reconnect_if_necessary (self, &inner_error);
  if (inner_error != NULL)
  {
    g_propagate_error (error, inner_error);
    return NULL;
  }

  if (!mpd_search_db_tags (self->main_mpd_connection, MPD_TAG_GENRE))
  {
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "search initialization failed - %s",
                 mpd_connection_get_error_message(self->main_mpd_connection));
    mpd_connection_clear_error (self->main_mpd_connection);
    return NULL;
  }
  if (!mpd_search_commit (self->main_mpd_connection))
  {
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "search start failed - %s",
                 mpd_connection_get_error_message(self->main_mpd_connection));
    mpd_connection_clear_error (self->main_mpd_connection);
    return NULL;
  }

  results = g_ptr_array_new_null_terminated (0, g_free, TRUE);
  while ((pair = mpd_recv_pair_tag(self->main_mpd_connection,
				    MPD_TAG_GENRE)) != NULL)
  {
    if (pair->value != NULL && !g_str_equal (pair->value, ""))
    {
      gchar *genre = g_strdup (pair->value);
      g_ptr_array_add (results, genre);
    }
    mpd_return_pair(self->main_mpd_connection, pair);
  }

  if (mpd_connection_get_error(self->main_mpd_connection) != MPD_ERROR_SUCCESS
      || !mpd_response_finish(self->main_mpd_connection))
  {
    g_ptr_array_free (results, TRUE);
    results = NULL;
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "cleanup failed - %s",
                 mpd_connection_get_error_message(self->main_mpd_connection));
    mpd_connection_clear_error (self->main_mpd_connection);
  }

  return results;
}

GPtrArray *
polyhymnia_mpd_client_search_playlists(PolyhymniaMpdClient *self,
                                       GError              **error)
{
  GError *inner_error = NULL;
  struct mpd_playlist *playlist;
  GPtrArray * results;

  g_return_val_if_fail (POLYHYMNIA_IS_MPD_CLIENT (self), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);
  g_return_val_if_fail (self->main_mpd_connection != NULL, NULL);

  polyhymnia_mpd_client_reconnect_if_necessary (self, &inner_error);
  if (inner_error != NULL)
  {
    g_propagate_error (error, inner_error);
    return NULL;
  }

  if (!mpd_send_list_playlists (self->main_mpd_connection))
  {
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "search start failed - %s",
                 mpd_connection_get_error_message(self->main_mpd_connection));
    mpd_connection_clear_error (self->main_mpd_connection);
    return NULL;
  }

  results = g_ptr_array_new_null_terminated (0, g_free, TRUE);
  while ((playlist = mpd_recv_playlist (self->main_mpd_connection)) != NULL)
  {
    g_ptr_array_add (results, g_strdup (mpd_playlist_get_path (playlist)));
    mpd_playlist_free (playlist);
  }

  if (mpd_connection_get_error(self->main_mpd_connection) != MPD_ERROR_SUCCESS
      || !mpd_response_finish(self->main_mpd_connection))
  {
    g_ptr_array_free (results, TRUE);
    results = NULL;
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "cleanup failed - %s",
                 mpd_connection_get_error_message(self->main_mpd_connection));
    mpd_connection_clear_error (self->main_mpd_connection);
  }

  return results;
}

GPtrArray *
polyhymnia_mpd_client_search_tracks(PolyhymniaMpdClient *self,
                                    const gchar         *query,
                                    GError              **error)
{
  GError *inner_error = NULL;
  struct mpd_song *track;
  GPtrArray *results;

  g_return_val_if_fail (POLYHYMNIA_IS_MPD_CLIENT (self), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);
  g_return_val_if_fail (self->main_mpd_connection != NULL, NULL);

  polyhymnia_mpd_client_reconnect_if_necessary (self, &inner_error);
  if (inner_error != NULL)
  {
    g_propagate_error (error, inner_error);
    return NULL;
  }

  if (!mpd_search_db_songs (self->main_mpd_connection, FALSE))
  {
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "search initialization failed - %s",
                 mpd_connection_get_error_message(self->main_mpd_connection));
    mpd_connection_clear_error (self->main_mpd_connection);
    return NULL;
  }
  if (!mpd_search_add_tag_constraint (self->main_mpd_connection,
                                      MPD_OPERATOR_DEFAULT,
                                      MPD_TAG_TITLE,
                                      query))
  {
    mpd_search_cancel (self->main_mpd_connection);
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "search filter failed - %s",
                 mpd_connection_get_error_message(self->main_mpd_connection));
    mpd_connection_clear_error (self->main_mpd_connection);
    return NULL;
  }
  if (!mpd_search_add_sort_tag (self->main_mpd_connection, MPD_TAG_TITLE, FALSE))
  {
    mpd_search_cancel (self->main_mpd_connection);
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "search sort failed - %s",
                 mpd_connection_get_error_message(self->main_mpd_connection));
    mpd_connection_clear_error (self->main_mpd_connection);
    return NULL;
  }
  if (!mpd_search_commit (self->main_mpd_connection))
  {
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "search start failed - %s",
                 mpd_connection_get_error_message(self->main_mpd_connection));
    mpd_connection_clear_error (self->main_mpd_connection);
    return NULL;
  }

  results = g_ptr_array_new ();
  g_ptr_array_set_free_func (results, g_object_unref);
  while ((track = mpd_recv_song(self->main_mpd_connection)) != NULL)
  {
    const gchar *title = mpd_song_get_tag (track, MPD_TAG_TITLE, 0);
    if (title != NULL && !g_str_equal (title, ""))
    {
      const gchar *album = mpd_song_get_tag (track, MPD_TAG_ALBUM, 0);
      const gchar *artist = mpd_song_get_tag (track, MPD_TAG_ARTIST, 0);
      GObject *track_object = g_object_new (POLYHYMNIA_TYPE_TRACK,
                                            "uri", mpd_song_get_uri (track),
                                            "title", title,
                                            "album", album,
                                            "artist", artist,
                                            "duration", mpd_song_get_duration (track),
                                            NULL);
      g_ptr_array_add(results, track_object);
    }
    mpd_song_free(track);
  }

  if (mpd_connection_get_error(self->main_mpd_connection) != MPD_ERROR_SUCCESS
      || !mpd_response_finish(self->main_mpd_connection))
  {
    g_ptr_array_free (results, TRUE);
    results = NULL;
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "cleanup failed - %s",
                 mpd_connection_get_error_message(self->main_mpd_connection));
    mpd_connection_clear_error (self->main_mpd_connection);
  }

  return results;
}

void
polyhymnia_mpd_client_seek_playback(PolyhymniaMpdClient *self,
                                    guint               id,
                                    guint               position,
                                    GError              **error)
{
  GError *inner_error = NULL;

  g_return_if_fail (POLYHYMNIA_IS_MPD_CLIENT (self));
  g_return_if_fail (error == NULL || *error == NULL);
  g_return_if_fail (self->main_mpd_connection != NULL);

  polyhymnia_mpd_client_reconnect_if_necessary (self, &inner_error);
  if (inner_error != NULL)
  {
    g_propagate_error (error, inner_error);
    return;
  }

  if (!mpd_run_seek_id(self->main_mpd_connection, id, position))
  {
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "failed - %s",
                 mpd_connection_get_error_message(self->main_mpd_connection));
    mpd_connection_clear_error (self->main_mpd_connection);
  }
}

void
polyhymnia_mpd_client_set_volume(PolyhymniaMpdClient *self,
                                 guint               volume,
                                 GError              **error)
{
  GError *inner_error = NULL;

  g_return_if_fail (POLYHYMNIA_IS_MPD_CLIENT (self));
  g_return_if_fail (error == NULL || *error == NULL);
  g_return_if_fail (self->main_mpd_connection != NULL);

  polyhymnia_mpd_client_reconnect_if_necessary (self, &inner_error);
  if (inner_error != NULL)
  {
    g_propagate_error (error, inner_error);
    return;
  }

  if (!mpd_run_set_volume (self->main_mpd_connection, volume))
  {
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "failed - %s",
                 mpd_connection_get_error_message(self->main_mpd_connection));
    mpd_connection_clear_error (self->main_mpd_connection);
  }
}

void
polyhymnia_mpd_client_swap_songs_in_queue(PolyhymniaMpdClient *self,
                                          guint               id1,
                                          guint               id2,
                                          GError              **error)
{
  GError *inner_error = NULL;

  g_return_if_fail (POLYHYMNIA_IS_MPD_CLIENT (self));
  g_return_if_fail (error == NULL || *error == NULL);
  g_return_if_fail (self->main_mpd_connection != NULL);

  polyhymnia_mpd_client_reconnect_if_necessary (self, &inner_error);
  if (inner_error != NULL)
  {
    g_propagate_error (error, inner_error);
    return;
  }

  if (!mpd_run_swap_id(self->main_mpd_connection, id1, id2))
  {
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "failed - %s",
                 mpd_connection_get_error_message(self->main_mpd_connection));
    mpd_connection_clear_error (self->main_mpd_connection);
  }
}

void
polyhymnia_mpd_client_toggle_output(PolyhymniaMpdClient *self,
                                    guint               output_id,
                                    GError              **error)
{
  GError *inner_error = NULL;

  g_return_if_fail (POLYHYMNIA_IS_MPD_CLIENT (self));
  g_return_if_fail (error == NULL || *error == NULL);
  g_return_if_fail (self->main_mpd_connection != NULL);

  polyhymnia_mpd_client_reconnect_if_necessary (self, &inner_error);
  if (inner_error != NULL)
  {
    g_propagate_error (error, inner_error);
    return;
  }

  if (!mpd_run_toggle_output (self->main_mpd_connection, output_id))
  {
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "failed - %s",
                 mpd_connection_get_error_message (self->main_mpd_connection));
    mpd_connection_clear_error (self->main_mpd_connection);
  }
}

void
polyhymnia_mpd_client_toggle_random_order(PolyhymniaMpdClient *self,
                                          gboolean            new_value,
                                          GError              **error)
{
  GError *inner_error = NULL;

  g_return_if_fail (POLYHYMNIA_IS_MPD_CLIENT (self));
  g_return_if_fail (error == NULL || *error == NULL);
  g_return_if_fail (self->main_mpd_connection != NULL);

  polyhymnia_mpd_client_reconnect_if_necessary (self, &inner_error);
  if (inner_error != NULL)
  {
    g_propagate_error (error, inner_error);
    return;
  }

  if (!mpd_run_random(self->main_mpd_connection, new_value))
  {
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "failed - %s",
                 mpd_connection_get_error_message(self->main_mpd_connection));
    mpd_connection_clear_error (self->main_mpd_connection);
  }
}

void
polyhymnia_mpd_client_toggle_repeat(PolyhymniaMpdClient *self,
                                    gboolean            new_value,
                                    GError              **error)
{
  GError *inner_error = NULL;

  g_return_if_fail (POLYHYMNIA_IS_MPD_CLIENT (self));
  g_return_if_fail (error == NULL || *error == NULL);
  g_return_if_fail (self->main_mpd_connection != NULL);

  polyhymnia_mpd_client_reconnect_if_necessary (self, &inner_error);
  if (inner_error != NULL)
  {
    g_propagate_error (error, inner_error);
    return;
  }

  if (!mpd_run_repeat (self->main_mpd_connection, new_value))
  {
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "failed - %s",
                 mpd_connection_get_error_message(self->main_mpd_connection));
    mpd_connection_clear_error (self->main_mpd_connection);
  }
}
