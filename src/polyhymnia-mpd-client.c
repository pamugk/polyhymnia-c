
#include "polyhymnia-mpd-client.h"

#include <mpd/client.h>

typedef enum
{
  PROP_SCAN_AVAILABLE = 1,
  N_PROPERTIES,
} PolyhymniaMpdClientProperty;

struct _PolyhymniaMpdClient
{
  GObject  parent_instance;

  /* Underlying MPD fields */
  struct mpd_connection   *mpd_connection;

  /* State fields */
  gboolean                  initialized;
};

G_DEFINE_FINAL_TYPE (PolyhymniaMpdClient, polyhymnia_mpd_client, G_TYPE_OBJECT)

G_DEFINE_QUARK (PolyhymniaMpdClient, polyhymnia_mpd_client_error);

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

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
polyhymnia_mpd_client_finalize (GObject *gobject)
{
  PolyhymniaMpdClient *self = POLYHYMNIA_MPD_CLIENT (gobject);
  if (self->mpd_connection != NULL)
  {
    mpd_connection_free(self->mpd_connection);
  }

  G_OBJECT_CLASS (polyhymnia_mpd_client_parent_class)->finalize (gobject);
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
    case PROP_SCAN_AVAILABLE:
      self->initialized = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
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
    case PROP_SCAN_AVAILABLE:
      g_value_set_boolean (value, self->initialized);
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

  gobject_class->constructed = polyhymnia_mpd_client_constructed;
  gobject_class->constructor = polyhymnia_mpd_client_constructor;
  gobject_class->finalize = polyhymnia_mpd_client_finalize;
  gobject_class->get_property = polyhymnia_mpd_client_get_property;
  gobject_class->set_property = polyhymnia_mpd_client_set_property;

  obj_properties[PROP_SCAN_AVAILABLE] =
    g_param_spec_boolean ("initialized",
                         "Initialized",
                         "Whether MPD connection is established.",
                         FALSE,
                         G_PARAM_READWRITE | G_PARAM_STATIC_NAME);

  g_object_class_install_properties (gobject_class,
                                     N_PROPERTIES,
                                     obj_properties);
}

static void
polyhymnia_mpd_client_init (PolyhymniaMpdClient *self)
{
  GError *error = NULL;
  polyhymnia_mpd_client_connect (self, &error);
  if (error != NULL)
  {
    g_warning("MPD client initialization error: %s\n",
              error->message);
    g_error_free (error);
  }
}

gint
polyhymnia_mpd_client_add_next_to_queue(PolyhymniaMpdClient *self,
                                        const gchar         *song_uri,
                                        GError              **error)
{
  gint id;
  g_return_val_if_fail (POLYHYMNIA_IS_MPD_CLIENT (self), 0);
  g_return_val_if_fail (error == NULL || *error == NULL, 0);
  g_return_val_if_fail (self->mpd_connection != NULL, 0);

  id = mpd_run_add_id_whence (self->mpd_connection, song_uri,
                              0, MPD_POSITION_AFTER_CURRENT);
  if (id == -1)
  {
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "failed - %s",
                 mpd_connection_get_error_message(self->mpd_connection));
  }

  return id;
}

/* Instance methods */
gint
polyhymnia_mpd_client_append_to_queue(PolyhymniaMpdClient *self,
                                      const gchar         *song_uri,
                                      GError              **error)
{
  gint id;
  g_return_val_if_fail (POLYHYMNIA_IS_MPD_CLIENT (self), 0);
  g_return_val_if_fail (error == NULL || *error == NULL, 0);
  g_return_val_if_fail (self->mpd_connection != NULL, 0);

  id = mpd_run_add_id(self->mpd_connection, song_uri);
  if (id == -1)
  {
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "failed - %s",
                 mpd_connection_get_error_message(self->mpd_connection));
  }

  return id;
}

void
polyhymnia_mpd_client_clear_queue(PolyhymniaMpdClient *self,
                                  GError             **error)
{
  g_return_if_fail (POLYHYMNIA_IS_MPD_CLIENT (self));
  g_return_if_fail (error == NULL || *error == NULL);
  g_return_if_fail (self->mpd_connection != NULL);

  if (!mpd_run_clear(self->mpd_connection))
  {
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "failed - %s",
                 mpd_connection_get_error_message(self->mpd_connection));
  }
}

void
polyhymnia_mpd_client_connect(PolyhymniaMpdClient *self,
                              GError              **error)
{
  struct mpd_connection *mpd_connection;
  enum mpd_error mpd_initialization_error;

  g_return_if_fail (POLYHYMNIA_IS_MPD_CLIENT (self));
  g_return_if_fail (error == NULL || *error == NULL);
  g_return_if_fail (self->mpd_connection == NULL);

  mpd_connection = mpd_connection_new(NULL, 0, 0);

  if (mpd_connection != NULL)
  {
    mpd_initialization_error = mpd_connection_get_error(mpd_connection);
    if (mpd_initialization_error == MPD_ERROR_SUCCESS)
    {
      const unsigned *mpd_version = mpd_connection_get_server_version (mpd_connection);
      g_debug("Connected to MPD %d.%d.%d", mpd_version[0], mpd_version[1], mpd_version[2]);
    }
    else
    {
      g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "%s",
                 mpd_connection_get_error_message(mpd_connection));
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

  self->mpd_connection = mpd_connection;
  if (mpd_connection != NULL)
  {
    g_object_set(G_OBJECT (self), "initialized", TRUE, NULL);
  }
}

void
polyhymnia_mpd_client_delete_from_queue(PolyhymniaMpdClient *self,
                                        guint               id,
                                        GError              **error)
{
  g_return_if_fail (POLYHYMNIA_IS_MPD_CLIENT (self));
  g_return_if_fail (error == NULL || *error == NULL);
  g_return_if_fail (self->mpd_connection != NULL);

  if (!mpd_run_delete_id (self->mpd_connection, id))
  {
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "failed - %s",
                 mpd_connection_get_error_message(self->mpd_connection));
  }
}

GPtrArray *
polyhymnia_mpd_client_get_queue(PolyhymniaMpdClient *self,
                                GError              **error)
{
  struct mpd_entity *entity;
  GPtrArray *results;

  g_return_val_if_fail (POLYHYMNIA_IS_MPD_CLIENT (self), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);
  g_return_val_if_fail (self->mpd_connection != NULL, NULL);

  if (!mpd_send_list_queue_meta(self->mpd_connection))
  {
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "request failed - %s",
                 mpd_connection_get_error_message(self->mpd_connection));
    return NULL;
  }

  results = g_ptr_array_new ();
  g_ptr_array_set_free_func (results, g_object_unref);

  while ((entity = mpd_recv_entity (self->mpd_connection)) != NULL)
  {
    const struct mpd_song *track = mpd_entity_get_song (entity);
    const gchar *title = mpd_song_get_tag (track, MPD_TAG_TITLE, 0);
    if (title != NULL && !g_str_equal (title, ""))
    {
      const gchar *album = mpd_song_get_tag (track, MPD_TAG_ALBUM, 0);
      const gchar *artist = mpd_song_get_tag (track, MPD_TAG_ARTIST, 0);
      GObject *track_object = g_object_new (POLYHYMNIA_TYPE_TRACK,
                                            "id", mpd_song_get_id (track),
                                            "position", mpd_song_get_pos (track),
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

  if (mpd_connection_get_error(self->mpd_connection) != MPD_ERROR_SUCCESS
      || !mpd_response_finish(self->mpd_connection))
  {
    g_ptr_array_free (results, TRUE);
    results = NULL;
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "cleanup failed - %s",
                 mpd_connection_get_error_message(self->mpd_connection));
  }

  return results;
}

int
polyhymnia_mpd_client_get_volume(PolyhymniaMpdClient *self,
                                 GError              **error)
{
  int volume;
  g_return_val_if_fail (POLYHYMNIA_IS_MPD_CLIENT (self), -1);
  g_return_val_if_fail (error == NULL || *error == NULL, -1);
  g_return_val_if_fail (self->mpd_connection != NULL, -1);

  volume = mpd_run_get_volume (self->mpd_connection);
  if (volume == -1)
  {
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "failed - %s",
                 mpd_connection_get_error_message(self->mpd_connection));
  }

  return volume;
}

void
polyhymnia_mpd_client_pause_playback(PolyhymniaMpdClient *self,
                                     GError              **error)
{
  g_return_if_fail (POLYHYMNIA_IS_MPD_CLIENT (self));
  g_return_if_fail (error == NULL || *error == NULL);
  g_return_if_fail (self->mpd_connection != NULL);

  if (!mpd_run_pause(self->mpd_connection, TRUE))
  {
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "failed - %s",
                 mpd_connection_get_error_message(self->mpd_connection));
  }
}

void
polyhymnia_mpd_client_play_next(PolyhymniaMpdClient *self,
                                GError              **error)
{
  g_return_if_fail (POLYHYMNIA_IS_MPD_CLIENT (self));
  g_return_if_fail (error == NULL || *error == NULL);
  g_return_if_fail (self->mpd_connection != NULL);

  if (!mpd_run_next (self->mpd_connection))
  {
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "failed - %s",
                 mpd_connection_get_error_message(self->mpd_connection));
  }
}

void
polyhymnia_mpd_client_play_previous(PolyhymniaMpdClient *self,
                                    GError              **error)
{
  g_return_if_fail (POLYHYMNIA_IS_MPD_CLIENT (self));
  g_return_if_fail (error == NULL || *error == NULL);
  g_return_if_fail (self->mpd_connection != NULL);

  if (!mpd_run_previous (self->mpd_connection))
  {
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "failed - %s",
                 mpd_connection_get_error_message(self->mpd_connection));
  }
}

void
polyhymnia_mpd_client_resume_playback(PolyhymniaMpdClient *self,
                                      GError              **error)
{
  g_return_if_fail (POLYHYMNIA_IS_MPD_CLIENT (self));
  g_return_if_fail (error == NULL || *error == NULL);
  g_return_if_fail (self->mpd_connection != NULL);

  if (!mpd_run_pause (self->mpd_connection, FALSE))
  {
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "failed - %s",
                 mpd_connection_get_error_message(self->mpd_connection));
  }
}

void
polyhymnia_mpd_client_scan(PolyhymniaMpdClient *self,
                           GError              **error)
{
  guint scan_job_id;

  g_return_if_fail (POLYHYMNIA_IS_MPD_CLIENT (self));
  g_return_if_fail (error == NULL || *error == NULL);

  scan_job_id = mpd_run_update (self->mpd_connection, NULL);
  if (scan_job_id > 0)
  {
    g_debug ("Scanning...");
  }
  else
  {
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "%s",
                 mpd_connection_get_error_message(self->mpd_connection));
  }
}

GPtrArray *
polyhymnia_mpd_client_search_albums(PolyhymniaMpdClient *self,
                                    GError              **error)
{
  struct mpd_pair *pair;
  GPtrArray * results;

  g_return_val_if_fail (POLYHYMNIA_IS_MPD_CLIENT (self), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  if (!mpd_search_db_tags (self->mpd_connection, MPD_TAG_ALBUM))
  {
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "search initialization failed - %s",
                 mpd_connection_get_error_message(self->mpd_connection));
    return NULL;
  }
  if (!mpd_search_commit (self->mpd_connection))
  {
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "search start failed - %s",
                 mpd_connection_get_error_message(self->mpd_connection));
    return NULL;
  }

  results = g_ptr_array_new ();
  g_ptr_array_set_free_func (results, g_object_unref);
  while ((pair = mpd_recv_pair_tag(self->mpd_connection,
				    MPD_TAG_ALBUM)) != NULL)
  {
    if (pair->value != NULL && !g_str_equal (pair->value, ""))
    {
      GObject *album_object = g_object_new (POLYHYMNIA_TYPE_ALBUM,
                                            "title", pair->value,
                                             NULL);
      g_ptr_array_add (results, album_object);
    }
    mpd_return_pair(self->mpd_connection, pair);
  }

  if (mpd_connection_get_error(self->mpd_connection) != MPD_ERROR_SUCCESS
      || !mpd_response_finish(self->mpd_connection))
  {
    g_ptr_array_free (results, TRUE);
    results = NULL;
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "cleanup failed - %s",
                 mpd_connection_get_error_message(self->mpd_connection));
  }

  return results;
}

GPtrArray *
polyhymnia_mpd_client_search_artists(PolyhymniaMpdClient *self,
                                      GError              **error)
{
  struct mpd_pair *pair;
  GPtrArray * results;

  g_return_val_if_fail (POLYHYMNIA_IS_MPD_CLIENT (self), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  if (!mpd_search_db_tags (self->mpd_connection, MPD_TAG_ALBUM_ARTIST))
  {
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "search initialization failed - %s",
                 mpd_connection_get_error_message(self->mpd_connection));
    return NULL;
  }
  if (!mpd_search_commit (self->mpd_connection))
  {
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "search start failed - %s",
                 mpd_connection_get_error_message(self->mpd_connection));
    return NULL;
  }

  results = g_ptr_array_new ();
  g_ptr_array_set_free_func (results, g_object_unref);
  while ((pair = mpd_recv_pair_tag(self->mpd_connection,
				    MPD_TAG_ALBUM_ARTIST)) != NULL)
  {
    if (pair->value != NULL && !g_str_equal (pair->value, ""))
    {
      GObject *artist_object = g_object_new (POLYHYMNIA_TYPE_ARTIST,
                                            "name", pair->value,
                                             NULL);
      g_ptr_array_add (results, artist_object);
    }
    mpd_return_pair(self->mpd_connection, pair);
  }

  if (mpd_connection_get_error(self->mpd_connection) != MPD_ERROR_SUCCESS
      || !mpd_response_finish(self->mpd_connection))
  {
    g_ptr_array_free (results, TRUE);
    results = NULL;
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "cleanup failed - %s",
                 mpd_connection_get_error_message(self->mpd_connection));
  }

  return results;
}

GPtrArray *
polyhymnia_mpd_client_search_genres(PolyhymniaMpdClient *self,
                                    GError              **error)
{
  struct mpd_pair *pair;
  GPtrArray * results;

  g_return_val_if_fail (POLYHYMNIA_IS_MPD_CLIENT (self), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  if (!mpd_search_db_tags (self->mpd_connection, MPD_TAG_GENRE))
  {
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "search initialization failed - %s",
                 mpd_connection_get_error_message(self->mpd_connection));
    return NULL;
  }
  if (!mpd_search_commit (self->mpd_connection))
  {
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "search start failed - %s",
                 mpd_connection_get_error_message(self->mpd_connection));
    return NULL;
  }

  results = g_ptr_array_new ();
  g_ptr_array_set_free_func (results, g_free);
  while ((pair = mpd_recv_pair_tag(self->mpd_connection,
				    MPD_TAG_GENRE)) != NULL)
  {
    if (pair->value != NULL && !g_str_equal (pair->value, ""))
    {
      gchar *genre = g_strdup (pair->value);
      g_ptr_array_add (results, genre);
    }
    mpd_return_pair(self->mpd_connection, pair);
  }

  if (mpd_connection_get_error(self->mpd_connection) != MPD_ERROR_SUCCESS
      || !mpd_response_finish(self->mpd_connection))
  {
    g_ptr_array_free (results, TRUE);
    results = NULL;
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "cleanup failed - %s",
                 mpd_connection_get_error_message(self->mpd_connection));
  }

  return results;
}

GPtrArray *
polyhymnia_mpd_client_search_tracks(PolyhymniaMpdClient *self,
                                    const gchar         *query,
                                    GError              **error)
{
  struct mpd_song *track;
  GPtrArray *results;

  g_return_val_if_fail (POLYHYMNIA_IS_MPD_CLIENT (self), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  if (!mpd_search_db_songs (self->mpd_connection, FALSE))
  {
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "search initialization failed - %s",
                 mpd_connection_get_error_message(self->mpd_connection));
    return NULL;
  }
  if (!mpd_search_add_tag_constraint (self->mpd_connection,
                                      MPD_OPERATOR_DEFAULT,
                                      MPD_TAG_TITLE,
                                      query))
  {
    mpd_search_cancel (self->mpd_connection);
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "search filter failed - %s",
                 mpd_connection_get_error_message(self->mpd_connection));
    return NULL;
  }
  if (!mpd_search_add_sort_tag (self->mpd_connection, MPD_TAG_TITLE, FALSE))
  {
    mpd_search_cancel (self->mpd_connection);
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "search sort failed - %s",
                 mpd_connection_get_error_message(self->mpd_connection));
    return NULL;
  }
  if (!mpd_search_commit (self->mpd_connection))
  {
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "search start failed - %s",
                 mpd_connection_get_error_message(self->mpd_connection));
    return NULL;
  }

  results = g_ptr_array_new ();
  g_ptr_array_set_free_func (results, g_object_unref);
  while ((track = mpd_recv_song(self->mpd_connection)) != NULL)
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

  if (mpd_connection_get_error(self->mpd_connection) != MPD_ERROR_SUCCESS
      || !mpd_response_finish(self->mpd_connection))
  {
    g_ptr_array_free (results, TRUE);
    results = NULL;
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "cleanup failed - %s",
                 mpd_connection_get_error_message(self->mpd_connection));
  }

  return results;
}

void
polyhymnia_mpd_client_seek_playback(PolyhymniaMpdClient *self,
                                    guint               id,
                                    time_t              position,
                                    GError              **error)
{
  g_return_if_fail (POLYHYMNIA_IS_MPD_CLIENT (self));
  g_return_if_fail (error == NULL || *error == NULL);
  g_return_if_fail (self->mpd_connection != NULL);

  if (!mpd_run_seek_id(self->mpd_connection, id, position))
  {
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "failed - %s",
                 mpd_connection_get_error_message(self->mpd_connection));
  }
}

void
polyhymnia_mpd_client_set_volume(PolyhymniaMpdClient *self,
                                 guint               volume,
                                 GError              **error)
{
  g_return_if_fail (POLYHYMNIA_IS_MPD_CLIENT (self));
  g_return_if_fail (error == NULL || *error == NULL);
  g_return_if_fail (self->mpd_connection != NULL);

  if (!mpd_run_set_volume (self->mpd_connection, volume))
  {
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "failed - %s",
                 mpd_connection_get_error_message(self->mpd_connection));
  }
}

void
polyhymnia_mpd_client_stop_playback(PolyhymniaMpdClient *self,
                                    GError              **error)
{
  g_return_if_fail (POLYHYMNIA_IS_MPD_CLIENT (self));
  g_return_if_fail (error == NULL || *error == NULL);
  g_return_if_fail (self->mpd_connection != NULL);

  if (!mpd_run_stop(self->mpd_connection))
  {
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "failed - %s",
                 mpd_connection_get_error_message(self->mpd_connection));
  }
}

void
polyhymnia_mpd_client_swap_songs_in_queue(PolyhymniaMpdClient *self,
                                          guint               id1,
                                          guint               id2,
                                          GError              **error)
{
  g_return_if_fail (POLYHYMNIA_IS_MPD_CLIENT (self));
  g_return_if_fail (error == NULL || *error == NULL);
  g_return_if_fail (self->mpd_connection != NULL);

  if (!mpd_run_swap_id(self->mpd_connection, id1, id2))
  {
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
                 "failed - %s",
                 mpd_connection_get_error_message(self->mpd_connection));
  }
}
