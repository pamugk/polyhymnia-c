
#pragma once

#include <glib-2.0/glib.h>

#include "polyhymnia-mpd-client-common.h"

G_BEGIN_DECLS

void
polyhymnia_mpd_client_append_playlist_to_queue (PolyhymniaMpdClient *self,
                                                const gchar         *name,
                                                GError              **error);

void
polyhymnia_mpd_client_add_to_playlist (PolyhymniaMpdClient *self,
                                       const gchar         *name,
                                       const gchar         *uri,
                                       GError              **error);

void
polyhymnia_mpd_client_clear_playlist (PolyhymniaMpdClient *self,
                                      const gchar         *name,
                                      GError              **error);

void
polyhymnia_mpd_client_delete_playlist (PolyhymniaMpdClient *self,
                                       const gchar         *name,
                                       GError              **error);

GPtrArray *
polyhymnia_mpd_client_get_playlist_tracks (PolyhymniaMpdClient *self,
                                           const gchar         *name,
                                           GError              **error);

void
polyhymnia_mpd_client_play_playlist (PolyhymniaMpdClient *self,
                                     const gchar         *name,
                                     GError              **error);

void
polyhymnia_mpd_client_rename_playlist (PolyhymniaMpdClient *self,
                                       const gchar         *old_name,
                                       const gchar         *new_name,
                                       GError              **error);

GPtrArray *
polyhymnia_mpd_client_search_playlists (PolyhymniaMpdClient *self,
                                        GError              **error);

void
polyhymnia_mpd_client_search_playlists_async (PolyhymniaMpdClient *self,
                                              GCancellable        *cancellable,
                                              GAsyncReadyCallback  callback,
                                              gpointer             user_data);

GPtrArray *
polyhymnia_mpd_client_search_playlists_finish (PolyhymniaMpdClient *self,
                                               GAsyncResult        *result,
                                               GError             **error);

G_END_DECLS
