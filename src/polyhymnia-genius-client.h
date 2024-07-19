
#pragma once

#include <glib-object.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#define POLYHYMNIA_TYPE_GENIUS_CLIENT (polyhymnia_genius_client_get_type())

G_DECLARE_FINAL_TYPE (PolyhymniaGeniusClient, polyhymnia_genius_client, POLYHYMNIA, GENIUS_CLIENT, GObject)

void
polyhymnia_genius_client_search_lyrics_async (PolyhymniaGeniusClient *self,
                                              const char             *query,
                                              GCancellable           *cancellable,
                                              GAsyncReadyCallback     callback,
                                              void                   *user_data);

void
polyhymnia_genius_client_search_lyrics_finish (PolyhymniaGeniusClient *self,
                                               GAsyncResult           *result,
                                               GError                **error);

G_END_DECLS
