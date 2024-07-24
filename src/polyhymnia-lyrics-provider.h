
#pragma once

#include <glib-object.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#define POLYHYMNIA_TYPE_LYRICS_PROVIDER (polyhymnia_lyrics_provider_get_type())

typedef struct
{
  char *artist;
  char *title;
} PolyhymniaSearchLyricsRequest;

void
polyhymnia_search_lyrics_request_free (PolyhymniaSearchLyricsRequest *self);

G_DECLARE_FINAL_TYPE (PolyhymniaLyricsProvider, polyhymnia_lyrics_provider, POLYHYMNIA, LYRICS_PROVIDER, GObject)

void
polyhymnia_lyrics_provider_search_lyrics_async (PolyhymniaLyricsProvider      *self,
                                                PolyhymniaSearchLyricsRequest *track_info,
                                                GCancellable                  *cancellable,
                                                GAsyncReadyCallback            callback,
                                                void                          *user_data);

char *
polyhymnia_lyrics_provider_search_lyrics_finish (PolyhymniaLyricsProvider *self,
                                                 GAsyncResult             *result,
                                                 GError                  **error);

G_END_DECLS
