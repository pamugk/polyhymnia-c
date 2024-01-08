
#pragma once

#include <glib-2.0/gio/gio.h>

#include "polyhymnia-mpd-client-common.h"
#include "polyhymnia-track-full-info.h"

G_BEGIN_DECLS

PolyhymniaTrackFullInfo *
polyhymnia_mpd_client_get_song_details (PolyhymniaMpdClient *self,
                                        const char          *song_uri,
                                        GError             **error);

void
polyhymnia_mpd_client_get_song_details_async (PolyhymniaMpdClient *self,
                                              const char          *song_uri,
                                              GCancellable        *cancellable,
                                              GAsyncReadyCallback  callback,
                                              gpointer             user_data);

PolyhymniaTrackFullInfo *
polyhymnia_mpd_client_get_song_details_finish (PolyhymniaMpdClient *self,
                                               GAsyncResult        *result,
                                               GError             **error);

G_END_DECLS
