
#include "secrets.h"

#include "polyhymnia-lyrics-provider.h"

#include <json-glib-1.0/json-glib/json-glib.h>
#include <libsoup/soup.h>

void
polyhymnia_search_lyrics_request_free (PolyhymniaSearchLyricsRequest *self)
{
  g_return_if_fail (self != NULL);

  g_free (self->artist);
  g_free (self->title);
  g_free (self);
}

/* Type metadata */
struct _PolyhymniaLyricsProvider
{
  GObject  parent_instance;

  GSettings   *application_settings;
  SoupSession *common_session;
};

G_DEFINE_FINAL_TYPE (PolyhymniaLyricsProvider, polyhymnia_lyrics_provider, G_TYPE_OBJECT)

/* Private instance methods declaration */
static void
polyhymnia_lyris_provider_genius_get_song_callback (GObject      *source,
                                                    GAsyncResult *result,
                                                    void         *user_data);

static void
polyhymnia_lyrics_provider_genius_search_callback (GObject      *source,
                                                   GAsyncResult *result,
                                                   void         *user_data);

/* Class stuff - constructors, destructors, etc */
static void
polyhymnia_lyrics_provider_constructed (GObject *obj)
{
  G_OBJECT_CLASS (polyhymnia_lyrics_provider_parent_class)->constructed (obj);
}

static GObject*
polyhymnia_lyrics_provider_constructor (GType                  type,
                                      unsigned int           n_construct_params,
                                      GObjectConstructParam *construct_params)
{
  static GObject *self = NULL;

  if (self == NULL)
  {
    self = G_OBJECT_CLASS (polyhymnia_lyrics_provider_parent_class)->constructor (
          type, n_construct_params, construct_params);
    g_object_add_weak_pointer (self, (gpointer) &self);
    return self;
  }

  return g_object_ref (self);
}

static void
polyhymnia_lyrics_provider_dispose (GObject *gobject)
{
  PolyhymniaLyricsProvider *self = POLYHYMNIA_LYRICS_PROVIDER (gobject);

  g_clear_object (&(self->application_settings));
  g_clear_object (&(self->common_session));

  G_OBJECT_CLASS (polyhymnia_lyrics_provider_parent_class)->dispose (gobject);
}

static void
polyhymnia_lyrics_provider_finalize (GObject *gobject)
{
  PolyhymniaLyricsProvider *self = POLYHYMNIA_LYRICS_PROVIDER (gobject);

  G_OBJECT_CLASS (polyhymnia_lyrics_provider_parent_class)->finalize (gobject);
}

static void
polyhymnia_lyrics_provider_class_init (PolyhymniaLyricsProviderClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->constructed = polyhymnia_lyrics_provider_constructed;
  gobject_class->constructor = polyhymnia_lyrics_provider_constructor;
  gobject_class->dispose = polyhymnia_lyrics_provider_dispose;
  gobject_class->finalize = polyhymnia_lyrics_provider_finalize;
}

static void
polyhymnia_lyrics_provider_init (PolyhymniaLyricsProvider *self)
{
  self->application_settings = g_settings_new ("com.github.pamugk.polyhymnia");
  self->common_session = soup_session_new();
}

/* Instance methods */
void
polyhymnia_lyrics_provider_search_lyrics_async (PolyhymniaLyricsProvider      *self,
                                                PolyhymniaSearchLyricsRequest *track_info,
                                                GCancellable                  *cancellable,
                                                GAsyncReadyCallback            callback,
                                                void                          *user_data)
{
  gboolean          started_search = FALSE;
  GTask            *task;

  g_assert (POLYHYMNIA_IS_LYRICS_PROVIDER (self));
  g_assert_nonnull (track_info);

  task = g_task_new (self, cancellable, callback, user_data);
  g_task_set_return_on_cancel (task, TRUE);
  g_task_set_source_tag (task, polyhymnia_lyrics_provider_search_lyrics_async);
  g_task_set_task_data (task, track_info,
                        (GDestroyNotify) polyhymnia_search_lyrics_request_free);

  if (g_settings_get_boolean (self->application_settings, "app-external-data-lyrics-genius")
      && track_info->title != NULL && track_info->artist != NULL)
  {
    char             *genius_encoded_query;
    char             *genius_query;
    SoupMessage      *message;

    genius_query = g_strjoin(" ", track_info->title, track_info->artist, NULL);
    genius_encoded_query = g_uri_escape_string (genius_query, NULL, FALSE);
    message = soup_message_new_from_encoded_form (SOUP_METHOD_GET,
                                                  "https://api.genius.com/search",
                                                  g_strconcat ("q=", genius_encoded_query, NULL));
    soup_message_headers_append (soup_message_get_request_headers (message),
                                  "Authorization", "Bearer " POLYHYMNIA_GENIUS_CLIENT_ACCESS_TOKEN);

    started_search = TRUE;
    soup_session_send_async (self->common_session, message,
                             G_PRIORITY_DEFAULT, g_task_get_cancellable (task),
                             polyhymnia_lyrics_provider_genius_search_callback,
                             g_object_ref (task));

    g_free (genius_encoded_query);
    g_free (genius_query);
    g_object_unref (message);
    g_object_unref (task);
  }

  if (!started_search)
  {
    g_task_return_pointer (task, NULL, g_free);
  }
}

char *
polyhymnia_lyrics_provider_search_lyrics_finish (PolyhymniaLyricsProvider *self,
                                                 GAsyncResult             *result,
                                                 GError                  **error)
{
  g_return_val_if_fail (g_task_is_valid (result, self), FALSE);
  return g_task_propagate_pointer (G_TASK (result), error);
}

/* Private instance methods implementation */
static void
polyhymnia_lyris_provider_genius_get_song_callback (GObject      *source,
                                                    GAsyncResult *result,
                                                    void         *user_data)
{
  GError       *error = NULL;
  GInputStream *input_stream;

  input_stream = soup_session_send_finish (SOUP_SESSION (source), result, &error);
  if (error != NULL)
  {
    g_warning ("Failed to get song on Genius: %s", error->message);
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
      g_warning ("Failed to parse Genius response for song: %s", error->message);
      g_task_return_error (task, error);
    }
    else
    {
      JsonReader *response_reader = json_reader_new (json_parser_get_root (response_parser));
      json_reader_read_member (response_reader, "meta");
      json_reader_read_member (response_reader, "status");
      if (json_reader_get_int_value (response_reader) != 200)
      {
        const char *error_message;
        json_reader_end_member (response_reader);
        json_reader_read_member (response_reader, "message");
        error_message = json_reader_get_string_value (response_reader);
        g_warning ("Received an error from Genius on song: %s",
                   error_message == NULL ? "Unknown error" : error_message);
        json_reader_end_member (response_reader);

        g_task_return_pointer (task, NULL, g_free);
      }
      else
      {
        char *song_lyrics_embeddable_content;

        json_reader_end_member (response_reader);
        json_reader_end_member (response_reader);

        json_reader_read_member (response_reader, "response");
        json_reader_read_member (response_reader, "song");
        json_reader_read_member (response_reader, "embed_content");
        song_lyrics_embeddable_content = g_strdup (json_reader_get_string_value (response_reader));
        json_reader_end_member (response_reader);
        json_reader_end_member (response_reader);
        json_reader_end_member (response_reader);

        g_task_return_pointer (task, song_lyrics_embeddable_content, g_free);
      }
      g_object_unref (response_reader);
    }

    g_object_unref (input_stream);
    g_object_unref (response_parser);
  }
}

static void
polyhymnia_lyrics_provider_genius_search_callback (GObject      *source,
                                                   GAsyncResult *result,
                                                   void         *user_data)
{
  GError       *error = NULL;
  GInputStream *input_stream;

  input_stream = soup_session_send_finish (SOUP_SESSION (source), result, &error);
  if (error != NULL)
  {
    g_warning ("Failed to search for song on Genius: %s", error->message);
    g_task_return_error (G_TASK (user_data), error);
  }
  else
  {
    JsonParser *response_parser = json_parser_new_immutable ();
    GTask      *task = G_TASK (user_data);

    json_parser_load_from_stream (response_parser, input_stream,
                                   g_task_get_cancellable (task), &error);
    if (error != NULL)
    {
      g_warning ("Failed to parse Genius response for song search: %s", error->message);
      g_task_return_error (task, error);
    }
    else
    {
      JsonReader *response_reader = json_reader_new (json_parser_get_root (response_parser));
      json_reader_read_member (response_reader, "meta");
      json_reader_read_member (response_reader, "status");
      if (json_reader_get_int_value (response_reader) != 200)
      {
        const char *error_message;
        json_reader_end_member (response_reader);
        json_reader_read_member (response_reader, "message");
        error_message = json_reader_get_string_value (response_reader);
        g_warning ("Received an error from Genius on song search: %s",
                   error_message == NULL ? "Unknown error" : error_message);
        json_reader_end_member (response_reader);

        g_task_return_pointer (task, NULL, g_free);
      }
      else
      {
        gboolean                       found_song = FALSE;
        long                           found_song_id;
        int                            hit_index = 0;
        PolyhymniaSearchLyricsRequest *processed_track = g_task_get_task_data (task);
        gchar                         *target_case_insensitive_artist;
        gchar                         *target_case_insensitive_title;

        target_case_insensitive_artist = g_utf8_casefold (processed_track->artist, -1);
        target_case_insensitive_title = g_utf8_casefold (processed_track->title, -1);

        json_reader_end_member (response_reader);
        json_reader_end_member (response_reader);

        json_reader_read_member (response_reader, "response");
        json_reader_read_member (response_reader, "hits");

        while (json_reader_read_element (response_reader, hit_index))
        {
          const char *hit_type;

          json_reader_read_member (response_reader, "type");
          hit_type = json_reader_get_string_value (response_reader);
          if (g_strcmp0 ("song", hit_type) == 0)
          {
            gboolean     same_artist;
            gboolean     same_title;

            json_reader_end_member (response_reader);

            json_reader_read_member (response_reader, "result");

            // TODO: do fuzzy comparison?
            json_reader_read_member (response_reader, "primary_artist_names");
            if (json_reader_get_string_value (response_reader) != NULL)
            {
              gchar *case_insensitive_artist = g_utf8_casefold (json_reader_get_string_value (response_reader), -1);
              same_artist = g_utf8_collate (target_case_insensitive_artist, case_insensitive_artist) == 0;
              g_free (case_insensitive_artist);
            }
            else
            {
              same_artist = FALSE;
            }
            json_reader_end_member (response_reader);

            json_reader_read_member (response_reader, "title");
            if (json_reader_get_string_value (response_reader) != NULL)
            {
              gchar *case_insensitive_title = g_utf8_casefold (json_reader_get_string_value (response_reader), -1);
              same_title = g_utf8_collate (target_case_insensitive_title, case_insensitive_title) == 0;
              g_free (case_insensitive_title);
            }
            else
            {
              same_title = FALSE;
            }
            json_reader_end_member (response_reader);

            if (same_artist && same_title)
            {
              json_reader_read_member (response_reader, "id");
              found_song_id = json_reader_get_int_value (response_reader);
              found_song = json_reader_get_error (response_reader) == NULL;
              json_reader_end_member (response_reader);
            }
            json_reader_end_member (response_reader);

            if (found_song)
            {
              break;
            }
          }
          else
          {
            json_reader_end_member (response_reader);
          }

          json_reader_end_element (response_reader);
          hit_index++;
        }
        json_reader_end_element (response_reader);

        json_reader_end_member (response_reader);
        json_reader_end_member (response_reader);

        if (found_song)
        {
          SoupMessage              *message;
          PolyhymniaLyricsProvider *self;
          char                     *song_id_str;
          char                     *song_uri;

          self = POLYHYMNIA_LYRICS_PROVIDER (g_task_get_source_object (task));
          song_id_str = g_strdup_printf ("%ld", found_song_id);
          song_uri = g_strconcat ("https://api.genius.com/songs/", song_id_str,
                                  NULL);
          message = soup_message_new_from_encoded_form (SOUP_METHOD_GET,
                                                        song_uri,
                                                        "text_format=plain");
          soup_message_headers_append (soup_message_get_request_headers (message),
                                       "Authorization", "Bearer " POLYHYMNIA_GENIUS_CLIENT_ACCESS_TOKEN);
          soup_session_send_async (self->common_session, message,
                                   G_PRIORITY_DEFAULT, g_task_get_cancellable (task),
                                   polyhymnia_lyris_provider_genius_get_song_callback,
                                   task);

          g_object_unref (message);
          g_free (song_id_str);
          g_free (song_uri);
        }
        else
        {
          g_task_return_pointer (task, NULL, g_free);
        }

        g_free (target_case_insensitive_artist);
        g_free (target_case_insensitive_title);
      }
      g_object_unref (response_reader);
    }

    g_object_unref (input_stream);
    g_object_unref (response_parser);
  }
}
