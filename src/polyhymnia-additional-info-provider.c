
#include "secrets.h"

#include "polyhymnia-additional-info-provider.h"

#include <json-glib-1.0/json-glib/json-glib.h>
#include <libsoup/soup.h>

#define LASTFM_API_HOST "http://ws.audioscrobbler.com/2.0/"

void
polyhymnia_search_album_info_response_free (PolyhymniaSearchAlbumInfoResponse *self)
{
  if (self != NULL)
  {
    g_free (self->description_summary);
    g_free (self->description_full);

    g_free (self);
  }
}

void
polyhymnia_search_artist_info_response_free (PolyhymniaSearchArtistInfoResponse *self)
{
  if (self != NULL)
  {
    g_free (self->bio_summary);
    g_free (self->bio_full);

    g_free (self);
  }
}

void
polyhymnia_search_track_info_response_free (PolyhymniaSearchTrackInfoResponse *self)
{
  if (self != NULL)
  {
    g_free (self->description_summary);
    g_free (self->description_full);

    g_free (self);
  }
}

/* Type metadata */
struct _PolyhymniaAdditionalInfoProvider
{
  GObject  parent_instance;

  GSettings   *application_settings;
  SoupSession *common_session;
};

G_DEFINE_FINAL_TYPE (PolyhymniaAdditionalInfoProvider, polyhymnia_additional_info_provider, G_TYPE_OBJECT)

/* Private instance methods declaration */
static void
polyhymnia_additional_info_provider_lastfm_get_album_callback (GObject      *source,
                                                               GAsyncResult *result,
                                                               void         *user_data);

static void
polyhymnia_additional_info_provider_lastfm_get_artist_callback (GObject      *source,
                                                                GAsyncResult *result,
                                                                void         *user_data);

static void
polyhymnia_additional_info_provider_lastfm_get_track_callback (GObject      *source,
                                                               GAsyncResult *result,
                                                               void         *user_data);

/* Class stuff - constructors, destructors, etc */
static void
polyhymnia_additional_info_provider_constructed (GObject *obj)
{
  G_OBJECT_CLASS (polyhymnia_additional_info_provider_parent_class)->constructed (obj);
}

static GObject*
polyhymnia_additional_info_provider_constructor (GType                  type,
                                                 unsigned int           n_construct_params,
                                                 GObjectConstructParam *construct_params)
{
  static GObject *self = NULL;

  if (self == NULL)
  {
    self = G_OBJECT_CLASS (polyhymnia_additional_info_provider_parent_class)->constructor (
          type, n_construct_params, construct_params);
    g_object_add_weak_pointer (self, (gpointer) &self);
    return self;
  }

  return g_object_ref (self);
}

static void
polyhymnia_additional_info_provider_dispose (GObject *gobject)
{
  PolyhymniaAdditionalInfoProvider *self = POLYHYMNIA_ADDITIONAL_INFO_PROVIDER (gobject);

  g_clear_object (&(self->application_settings));
  g_clear_object (&(self->common_session));

  G_OBJECT_CLASS (polyhymnia_additional_info_provider_parent_class)->dispose (gobject);
}

static void
polyhymnia_additional_info_provider_class_init (PolyhymniaAdditionalInfoProviderClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->constructed = polyhymnia_additional_info_provider_constructed;
  gobject_class->constructor = polyhymnia_additional_info_provider_constructor;
  gobject_class->dispose = polyhymnia_additional_info_provider_dispose;
}

static void
polyhymnia_additional_info_provider_init (PolyhymniaAdditionalInfoProvider *self)
{
  self->application_settings = g_settings_new ("com.github.pamugk.polyhymnia");
  self->common_session = soup_session_new();
}

/* Instance methods */
void
polyhymnia_additional_info_provider_search_album_info_async (PolyhymniaAdditionalInfoProvider       *self,
                                                             const PolyhymniaSearchAlbumInfoRequest *request,
                                                             GCancellable                           *cancellable,
                                                             GAsyncReadyCallback                     callback,
                                                             void                                   *user_data)
{
  gboolean started_search = FALSE;
  GTask   *task;

  g_assert (POLYHYMNIA_IS_ADDITIONAL_INFO_PROVIDER (self));
  g_assert_nonnull (request);

  task = g_task_new (self, cancellable, callback, user_data);
  g_task_set_return_on_cancel (task, TRUE);
  g_task_set_source_tag (task, polyhymnia_additional_info_provider_search_album_info_async);

  if (g_settings_get_boolean (self->application_settings, "app-external-data-additional-info-lastfm")
      && ((request->album_name != NULL && request->artist_name != NULL)
          || request->album_musicbrainz_id != NULL))
  {
    SoupMessage *message;
    GString     *uri_query = g_string_new ("api_key=" POLYHYMNIA_LASTFM_API_KEY "&method=album.getinfo&format=json");

    if (request->album_musicbrainz_id != NULL)
    {
      g_string_append (uri_query, "&mbid=");
      g_string_append_uri_escaped (uri_query, request->album_musicbrainz_id,
                                   NULL, TRUE);
    }
    if (request->album_name != NULL)
    {
      g_string_append (uri_query, "&artist=");
      g_string_append_uri_escaped (uri_query, request->artist_name,
                                   NULL, TRUE);
      g_string_append (uri_query, "&album=");
      g_string_append_uri_escaped (uri_query, request->album_name,
                                   NULL, TRUE);
    }
    message = soup_message_new_from_encoded_form (SOUP_METHOD_GET,
                                                  LASTFM_API_HOST,
                                                  g_string_free_and_steal (uri_query));
    soup_message_headers_append (soup_message_get_request_headers (message),
                                 "User-Agent", "Polyhymnia");

    started_search = TRUE;
    soup_session_send_async (self->common_session, message,
                             G_PRIORITY_DEFAULT, g_task_get_cancellable (task),
                             polyhymnia_additional_info_provider_lastfm_get_album_callback,
                             g_object_ref (task));

    g_object_unref (message);
    g_object_unref (task);
  }

  if (!started_search)
  {
    g_task_return_pointer (task, NULL, NULL);
  }
}

PolyhymniaSearchAlbumInfoResponse *
polyhymnia_additional_info_provider_search_album_info_finish (PolyhymniaAdditionalInfoProvider *self,
                                                              GAsyncResult                     *result,
                                                              GError                          **error)
{
  g_return_val_if_fail (g_task_is_valid (result, self), NULL);
  return g_task_propagate_pointer (G_TASK (result), error);
}

void
polyhymnia_additional_info_provider_search_artist_info_async (PolyhymniaAdditionalInfoProvider        *self,
                                                              const PolyhymniaSearchArtistInfoRequest *request,
                                                              GCancellable                            *cancellable,
                                                              GAsyncReadyCallback                      callback,
                                                              void                                    *user_data)
{
  gboolean started_search = FALSE;
  GTask   *task;

  g_assert (POLYHYMNIA_IS_ADDITIONAL_INFO_PROVIDER (self));
  g_assert_nonnull (request);

  task = g_task_new (self, cancellable, callback, user_data);
  g_task_set_return_on_cancel (task, TRUE);
  g_task_set_source_tag (task, polyhymnia_additional_info_provider_search_artist_info_async);

  if (g_settings_get_boolean (self->application_settings, "app-external-data-additional-info-lastfm")
      && (request->artist_name != NULL || request->artist_musicbrainz_id != NULL))
  {
    SoupMessage *message;
    GString     *uri_query = g_string_new ("api_key=" POLYHYMNIA_LASTFM_API_KEY "&method=artist.getinfo&format=json");

    if (request->artist_musicbrainz_id != NULL)
    {
      g_string_append (uri_query, "&mbid=");
      g_string_append_uri_escaped (uri_query, request->artist_musicbrainz_id,
                                   NULL, TRUE);
    }
    if (request->artist_name != NULL)
    {
      g_string_append (uri_query, "&artist=");
      g_string_append_uri_escaped (uri_query, request->artist_name,
                                   NULL, TRUE);
    }
    message = soup_message_new_from_encoded_form (SOUP_METHOD_GET,
                                                  LASTFM_API_HOST,
                                                  g_string_free_and_steal (uri_query));
    soup_message_headers_append (soup_message_get_request_headers (message),
                                 "User-Agent", "Polyhymnia");

    started_search = TRUE;
    soup_session_send_async (self->common_session, message,
                             G_PRIORITY_DEFAULT, g_task_get_cancellable (task),
                             polyhymnia_additional_info_provider_lastfm_get_artist_callback,
                             g_object_ref (task));

    g_object_unref (message);
    g_object_unref (task);
  }

  if (!started_search)
  {
    g_task_return_pointer (task, NULL, NULL);
  }
}

PolyhymniaSearchArtistInfoResponse *
polyhymnia_additional_info_provider_search_artist_info_finish (PolyhymniaAdditionalInfoProvider *self,
                                                               GAsyncResult                     *result,
                                                               GError                          **error)
{
  g_return_val_if_fail (g_task_is_valid (result, self), NULL);
  return g_task_propagate_pointer (G_TASK (result), error);
}

void
polyhymnia_additional_info_provider_search_track_info_async (PolyhymniaAdditionalInfoProvider       *self,
                                                             const PolyhymniaSearchTrackInfoRequest *request,
                                                             GCancellable                           *cancellable,
                                                             GAsyncReadyCallback                     callback,
                                                             void                                   *user_data)
{
  gboolean started_search = FALSE;
  GTask   *task;

  g_assert (POLYHYMNIA_IS_ADDITIONAL_INFO_PROVIDER (self));
  g_assert_nonnull (request);

  task = g_task_new (self, cancellable, callback, user_data);
  g_task_set_return_on_cancel (task, TRUE);
  g_task_set_source_tag (task, polyhymnia_additional_info_provider_search_album_info_async);

  if (g_settings_get_boolean (self->application_settings, "app-external-data-additional-info-lastfm")
      && ((request->track_name != NULL && request->artist_name != NULL)
          || request->track_musicbrainz_id != NULL))
  {
    SoupMessage *message;
    GString     *uri_query = g_string_new ("api_key=" POLYHYMNIA_LASTFM_API_KEY "&method=track.getinfo&format=json");

    if (request->track_musicbrainz_id != NULL)
    {
      g_string_append (uri_query, "&mbid=");
      g_string_append_uri_escaped (uri_query, request->track_musicbrainz_id,
                                   NULL, TRUE);
    }
    if (request->track_name != NULL)
    {
      g_string_append (uri_query, "&artist=");
      g_string_append_uri_escaped (uri_query, request->artist_name,
                                   NULL, TRUE);
      g_string_append (uri_query, "&track=");
      g_string_append_uri_escaped (uri_query, request->track_name,
                                   NULL, TRUE);
    }
    message = soup_message_new_from_encoded_form (SOUP_METHOD_GET,
                                                  LASTFM_API_HOST,
                                                  g_string_free_and_steal (uri_query));
    soup_message_headers_append (soup_message_get_request_headers (message),
                                 "User-Agent", "Polyhymnia");

    started_search = TRUE;
    soup_session_send_async (self->common_session, message,
                             G_PRIORITY_DEFAULT, g_task_get_cancellable (task),
                             polyhymnia_additional_info_provider_lastfm_get_track_callback,
                             g_object_ref (task));

    g_object_unref (message);
    g_object_unref (task);
  }

  if (!started_search)
  {
    g_task_return_pointer (task, NULL, NULL);
  }
}

PolyhymniaSearchTrackInfoResponse *
polyhymnia_additional_info_provider_search_track_info_finish (PolyhymniaAdditionalInfoProvider *self,
                                                              GAsyncResult                     *result,
                                                              GError                          **error)
{
  g_return_val_if_fail (g_task_is_valid (result, self), NULL);
  return g_task_propagate_pointer (G_TASK (result), error);
}

/* Private instance methods implementation */
static void
polyhymnia_additional_info_provider_lastfm_get_album_callback (GObject      *source,
                                                               GAsyncResult *result,
                                                               void         *user_data)
{
  GError       *error = NULL;
  GInputStream *input_stream;

  input_stream = soup_session_send_finish (SOUP_SESSION (source), result, &error);
  if (error != NULL)
  {
    if (!g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
    {
      g_warning ("Failed to get album info on Last.fm: %s", error->message);
    }
    g_task_return_error (G_TASK (user_data), error);
  }
  else
  {
    JsonParser       *response_parser = json_parser_new_immutable ();
    GTask            *task = G_TASK (user_data);

    json_parser_load_from_stream (response_parser, input_stream,
                                  g_task_get_cancellable (task), &error);
    if (error != NULL)
    {
      if (!g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
      {
        g_warning ("Failed to parse Last.fm response for album info: %s", error->message);
      }
      g_task_return_error (task, error);
    }
    else
    {
      JsonReader *response_reader = json_reader_new (json_parser_get_root (response_parser));

      json_reader_read_member (response_reader, "message");
      if (json_reader_get_error (response_reader) == NULL)
      {
        g_warning ("Failed to get album info on Last.fm. Reason: %s",
                   json_reader_get_string_value (response_reader));
        json_reader_end_member (response_reader);
        g_task_return_pointer (task, NULL, NULL);
      }
      else
      {
        PolyhymniaSearchAlbumInfoResponse *response;
        response = g_malloc0 (sizeof (PolyhymniaSearchAlbumInfoResponse));

        json_reader_end_member (response_reader);

        json_reader_read_member (response_reader, "album");

        json_reader_read_member (response_reader, "wiki");
        if (json_reader_get_error (response_reader) == NULL)
        {
          json_reader_read_member (response_reader, "summary");
          response->description_summary = g_strdup (json_reader_get_string_value (response_reader));
          json_reader_end_member (response_reader);

          json_reader_read_member (response_reader, "content");
          response->description_full = g_strdup (json_reader_get_string_value (response_reader));
          json_reader_end_member (response_reader);
        }
        json_reader_end_member (response_reader);

        json_reader_end_member (response_reader);

        g_task_return_pointer (task, response,
                               (GDestroyNotify) polyhymnia_search_album_info_response_free);
      }

      g_object_unref (response_reader);
    }

    g_object_unref (input_stream);
    g_object_unref (response_parser);
  }
}

static void
polyhymnia_additional_info_provider_lastfm_get_artist_callback (GObject      *source,
                                                                GAsyncResult *result,
                                                                void         *user_data)
{
  GError       *error = NULL;
  GInputStream *input_stream;

  input_stream = soup_session_send_finish (SOUP_SESSION (source), result, &error);
  if (error != NULL)
  {
    if (!g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
    {
      g_warning ("Failed to get artist info on Last.fm: %s", error->message);
    }
    g_task_return_error (G_TASK (user_data), error);
  }
  else
  {
    JsonParser       *response_parser = json_parser_new_immutable ();
    GTask            *task = G_TASK (user_data);

    json_parser_load_from_stream (response_parser, input_stream,
                                  g_task_get_cancellable (task), &error);
    if (error != NULL)
    {
      if (!g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
      {
        g_warning ("Failed to parse Last.fm response for artist info: %s", error->message);
      }
      g_task_return_error (task, error);
    }
    else
    {
      JsonReader *response_reader = json_reader_new (json_parser_get_root (response_parser));

      json_reader_read_member (response_reader, "message");
      if (json_reader_get_error (response_reader) == NULL)
      {
        g_warning ("Failed to get artist info on Last.fm. Reason: %s",
                   json_reader_get_string_value (response_reader));
        json_reader_end_member (response_reader);
        g_task_return_pointer (task, NULL, NULL);
      }
      else
      {
        PolyhymniaSearchArtistInfoResponse *response;
        response = g_malloc0 (sizeof (PolyhymniaSearchArtistInfoResponse));

        json_reader_end_member (response_reader);

        json_reader_read_member (response_reader, "artist");

        json_reader_read_member (response_reader, "bio");
        if (json_reader_get_error (response_reader) == NULL)
        {
          json_reader_read_member (response_reader, "summary");
          response->bio_summary = g_strdup (json_reader_get_string_value (response_reader));
          json_reader_end_member (response_reader);

          json_reader_read_member (response_reader, "content");
          response->bio_full = g_strdup (json_reader_get_string_value (response_reader));
          json_reader_end_member (response_reader);
        }
        json_reader_end_member (response_reader);

        json_reader_end_member (response_reader);

        g_task_return_pointer (task, response,
                               (GDestroyNotify) polyhymnia_search_artist_info_response_free);
      }

      g_object_unref (response_reader);
    }

    g_object_unref (input_stream);
    g_object_unref (response_parser);
  }
}

static void
polyhymnia_additional_info_provider_lastfm_get_track_callback (GObject      *source,
                                                               GAsyncResult *result,
                                                               void         *user_data)
{
  GError       *error = NULL;
  GInputStream *input_stream;

  input_stream = soup_session_send_finish (SOUP_SESSION (source), result, &error);
  if (error != NULL)
  {
    if (!g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
    {
      g_warning ("Failed to get track info on Last.fm: %s", error->message);
    }
    g_task_return_error (G_TASK (user_data), error);
  }
  else
  {
    JsonParser       *response_parser = json_parser_new_immutable ();
    GTask            *task = G_TASK (user_data);

    json_parser_load_from_stream (response_parser, input_stream,
                                  g_task_get_cancellable (task), &error);
    if (error != NULL)
    {
      if (!g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
      {
        g_warning ("Failed to parse Last.fm response for track info: %s", error->message);
      }
      g_task_return_error (task, error);
    }
    else
    {
      JsonReader *response_reader = json_reader_new (json_parser_get_root (response_parser));

      json_reader_read_member (response_reader, "message");
      if (json_reader_get_error (response_reader) == NULL)
      {
        g_warning ("Failed to get track info on Last.fm. Reason: %s",
                   json_reader_get_string_value (response_reader));
        json_reader_end_member (response_reader);
        g_task_return_pointer (task, NULL, NULL);
      }
      else
      {
        PolyhymniaSearchTrackInfoResponse *response;
        response = g_malloc0 (sizeof (PolyhymniaSearchTrackInfoResponse));

        json_reader_end_member (response_reader);

        json_reader_read_member (response_reader, "track");

        json_reader_read_member (response_reader, "wiki");
        if (json_reader_get_error (response_reader) == NULL)
        {
          json_reader_read_member (response_reader, "summary");
          response->description_summary = g_strdup (json_reader_get_string_value (response_reader));
          json_reader_end_member (response_reader);

          json_reader_read_member (response_reader, "content");
          response->description_full = g_strdup (json_reader_get_string_value (response_reader));
          json_reader_end_member (response_reader);
        }
        json_reader_end_member (response_reader);

        json_reader_end_member (response_reader);

        g_task_return_pointer (task, response,
                               (GDestroyNotify) polyhymnia_search_track_info_response_free);
      }

      g_object_unref (response_reader);
    }

    g_object_unref (input_stream);
    g_object_unref (response_parser);
  }
}
