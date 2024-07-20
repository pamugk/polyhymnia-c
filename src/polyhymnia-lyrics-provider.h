
#pragma once

#include <glib-object.h>
#include <gio/gio.h>

#include "polyhymnia-track.h"

G_BEGIN_DECLS

#define POLYHYMNIA_TYPE_LYRICS_PROVIDER (polyhymnia_lyrics_provider_get_type())

G_DECLARE_FINAL_TYPE (PolyhymniaLyricsProvider, polyhymnia_lyrics_provider, POLYHYMNIA, LYRICS_PROVIDER, GObject)

void
polyhymnia_lyrics_provider_search_track_lyrics_async (PolyhymniaLyricsProvider *self,
                                                      PolyhymniaTrack          *track,
                                                      GCancellable             *cancellable,
                                                      GAsyncReadyCallback       callback,
                                                      void                     *user_data);

gboolean
polyhymnia_lyrics_provider_search_track_lyrics_finish (PolyhymniaLyricsProvider *self,
                                                       GAsyncResult             *result,
                                                       GError                  **error);

G_END_DECLS
