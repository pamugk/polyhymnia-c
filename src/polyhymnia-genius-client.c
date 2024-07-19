
#include "polyhymnia-genius-client.h"

#include <libsoup/soup.h>

/* Type metadata */
struct _PolyhymniaGeniusClient
{
  GObject  parent_instance;

  SoupSession *session;
};

G_DEFINE_FINAL_TYPE (PolyhymniaGeniusClient, polyhymnia_genius_client, G_TYPE_OBJECT)

/* Class stuff - constructors, destructors, etc */
static void
polyhymnia_genius_client_constructed (GObject *obj)
{
  G_OBJECT_CLASS (polyhymnia_genius_client_parent_class)->constructed (obj);
}

static GObject*
polyhymnia_genius_client_constructor (GType                  type,
                                      unsigned int           n_construct_params,
                                      GObjectConstructParam *construct_params)
{
  static GObject *self = NULL;

  if (self == NULL)
  {
    self = G_OBJECT_CLASS (polyhymnia_genius_client_parent_class)->constructor (
          type, n_construct_params, construct_params);
    g_object_add_weak_pointer (self, (gpointer) &self);
    return self;
  }

  return g_object_ref (self);
}

static void
polyhymnia_genius_client_dispose (GObject *gobject)
{
  PolyhymniaGeniusClient *self = POLYHYMNIA_GENIUS_CLIENT (gobject);

  g_clear_object (&(self->session));

  G_OBJECT_CLASS (polyhymnia_genius_client_parent_class)->dispose (gobject);
}

static void
polyhymnia_genius_client_finalize (GObject *gobject)
{
  PolyhymniaGeniusClient *self = POLYHYMNIA_GENIUS_CLIENT (gobject);

  G_OBJECT_CLASS (polyhymnia_genius_client_parent_class)->finalize (gobject);
}

static void
polyhymnia_genius_client_class_init (PolyhymniaGeniusClientClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->constructed = polyhymnia_genius_client_constructed;
  gobject_class->constructor = polyhymnia_genius_client_constructor;
  gobject_class->dispose = polyhymnia_genius_client_dispose;
  gobject_class->finalize = polyhymnia_genius_client_finalize;
}

static void
polyhymnia_genius_client_init (PolyhymniaGeniusClient *self)
{
  self->session = soup_session_new();
}

/* Instance methods */
void
polyhymnia_genius_client_search_lyrics_async (PolyhymniaGeniusClient *self,
                                              const char             *query,
                                              GCancellable           *cancellable,
                                              GAsyncReadyCallback     callback,
                                              void                   *user_data)
{
  char        *encoded_query;
  SoupMessage *message;

  g_assert_nonnull (query);
  encoded_query = g_uri_escape_string (query, NULL, FALSE);
  message = soup_message_new_from_encoded_form (SOUP_METHOD_GET,
                                                "https://api.genius.com/search",
                                                g_strconcat ("q=", encoded_query, NULL));
  soup_message_headers_append (soup_message_get_request_headers (message),
                               "Authorization", "Bearer <token goes here>");

  soup_session_send_and_read_async (self->session, message,
                                    G_PRIORITY_DEFAULT, NULL, NULL, NULL);
}

void
polyhymnia_genius_client_search_lyrics_finish (PolyhymniaGeniusClient *self,
                                               GAsyncResult           *result,
                                               GError                **error)
{
  // TODO: implement response handling
}
