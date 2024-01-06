
#pragma once

#include <glib-2.0/gio/gio.h>

#include "polyhymnia-mpd-client-common.h"

G_BEGIN_DECLS

void
polyhymnia_mpd_client_clear_queue(PolyhymniaMpdClient *self,
                                  GError              **error);

void
polyhymnia_mpd_client_delete_from_queue(PolyhymniaMpdClient *self,
                                        guint               id,
                                        GError              **error);

void
polyhymnia_mpd_client_delete_songs_from_queue(PolyhymniaMpdClient *self,
                                              GArray              *ids,
                                              GError              **error);

GPtrArray *
polyhymnia_mpd_client_get_queue(PolyhymniaMpdClient *self,
                                GError              **error);

void
polyhymnia_mpd_client_get_queue_async (PolyhymniaMpdClient *self,
                                       GCancellable        *cancellable,
                                       GAsyncReadyCallback  callback,
                                       gpointer             user_data);

GPtrArray *
polyhymnia_mpd_client_get_queue_finish (PolyhymniaMpdClient *self,
                                        GAsyncResult        *result,
                                        GError             **error);

void
polyhymnia_mpd_client_play_song_from_queue(PolyhymniaMpdClient *self,
                                           guint               id,
                                           GError              **error);

void
polyhymnia_mpd_client_save_queue_as_playlist (PolyhymniaMpdClient *self,
                                              const gchar         *name,
                                              GError              **error);

void
polyhymnia_mpd_client_swap_songs_in_queue(PolyhymniaMpdClient *self,
                                          guint               id1,
                                          guint               id2,
                                          GError              **error);

G_END_DECLS
