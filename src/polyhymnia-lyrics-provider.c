
#include "polyhymnia-lyrics-provider.h"

#include <json-glib-1.0/json-glib/json-glib.h>
#include <libsoup/soup.h>

/* Type metadata */
struct _PolyhymniaLyricsProvider
{
  GObject  parent_instance;

  SoupSession *common_session;
};

G_DEFINE_FINAL_TYPE (PolyhymniaLyricsProvider, polyhymnia_lyrics_provider, G_TYPE_OBJECT)

/* Data structures for internal logic */
typedef struct
{
  GInputStream    *current_genius_input_stream;

  PolyhymniaTrack *processed_track;
} SearchLyricsData;

/* Private instance methods declaration */
static void
polyhymnia_lyrics_provider_genius_parse_search_result_callback (GObject      *source,
                                                                GAsyncResult *result,
                                                                void         *user_data);

static void
polyhymnia_lyrics_provider_genius_search_callback (GObject      *source,
                                                   GAsyncResult *result,
                                                   void         *user_data);

static void
polyhymnia_search_lyrics_data_free (SearchLyricsData *data);

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
  self->common_session = soup_session_new();
}

/* Instance methods */
void
polyhymnia_lyrics_provider_search_track_lyrics_async (PolyhymniaLyricsProvider *self,
                                                      PolyhymniaTrack          *track,
                                                      GCancellable             *cancellable,
                                                      GAsyncReadyCallback       callback,
                                                      void                     *user_data)
{
  SearchLyricsData *data;
  char             *genius_encoded_query;
  char             *genius_query;
  SoupMessage      *message;
  GTask            *task;

  g_assert (POLYHYMNIA_IS_LYRICS_PROVIDER (self));
  genius_query = g_strjoin(" ",
                           polyhymnia_track_get_title (track),
                           polyhymnia_track_get_artist (track),
                           NULL);
  genius_encoded_query = g_uri_escape_string (genius_query, NULL, FALSE);
  message = soup_message_new_from_encoded_form (SOUP_METHOD_GET,
                                                "https://api.genius.com/search",
                                                g_strconcat ("q=", genius_encoded_query, NULL));
  soup_message_headers_append (soup_message_get_request_headers (message),
                               "Authorization", "Bearer <token goes here>");

  task = g_task_new (self, cancellable, callback, user_data);
  g_task_set_return_on_cancel (task, TRUE);
  g_task_set_source_tag (task, polyhymnia_lyrics_provider_search_track_lyrics_async);
  data = g_malloc0 (sizeof (SearchLyricsData));
  data->processed_track = g_object_ref (track);
  g_task_set_task_data (task, data,
                        (GDestroyNotify) polyhymnia_search_lyrics_data_free);

  soup_session_send_async (self->common_session, message,
                           G_PRIORITY_DEFAULT, g_task_get_cancellable (task),
                           polyhymnia_lyrics_provider_genius_search_callback,
                           g_object_ref (task));

  g_free (genius_encoded_query);
  g_free (genius_query);
  g_object_unref (task);
}

gboolean
polyhymnia_lyrics_provider_search_track_lyrics_finish (PolyhymniaLyricsProvider *self,
                                                       GAsyncResult             *result,
                                                       GError                  **error)
{
  g_return_val_if_fail (g_task_is_valid (result, self), FALSE);
  return g_task_propagate_boolean (G_TASK (result), error);
}

/* Private instance methods implementation */
static void
polyhymnia_lyrics_provider_genius_parse_search_result_callback (GObject      *source,
                                                                GAsyncResult *result,
                                                                void         *user_data)
{
  GError     *error = NULL;
  JsonParser *response_parser = JSON_PARSER (source);
  GTask            *task = G_TASK (user_data);
  SearchLyricsData *task_data = g_task_get_task_data (task);

  json_parser_load_from_stream_finish (response_parser, result, &error);
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
      json_reader_end_member (response_reader);

      g_warning ("Received an error from Genius on song search: %s",
                 error_message == NULL ? "Unknown error" : error_message);

      g_task_return_boolean (task, FALSE);
    }
    else
    {
      json_reader_end_member (response_reader);
      json_reader_end_member (response_reader);
      // TODO: process received songs to find the best match & fetch lyrics
      g_task_return_boolean (task, TRUE);
    }
    g_object_unref (response_reader);
  }

  g_object_unref (source);
  g_object_unref (task_data->current_genius_input_stream);
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
    JsonParser       *response_parser = json_parser_new_immutable ();
    GTask            *task = G_TASK (user_data);
    SearchLyricsData *task_data = g_task_get_task_data (task);
    task_data->current_genius_input_stream = g_object_ref (input_stream);
    json_parser_load_from_stream_async (g_object_ref (response_parser),
                                        task_data->current_genius_input_stream,
                                        g_task_get_cancellable (task),
                                        polyhymnia_lyrics_provider_genius_parse_search_result_callback,
                                        user_data);

    g_object_unref (input_stream);
    g_object_unref (response_parser);
  }
}

static void
polyhymnia_search_lyrics_data_free (SearchLyricsData *data)
{
  g_clear_object (&(data->current_genius_input_stream));
  g_clear_object (&(data->processed_track));
}
