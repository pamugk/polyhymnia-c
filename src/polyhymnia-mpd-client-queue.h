
#pragma once

#include "polyhymnia-mpd-client-common.h"

G_BEGIN_DECLS

void
polyhymnia_mpd_client_clear_queue(PolyhymniaMpdClient *self,
                                  GError              **error);

void
polyhymnia_mpd_client_delete_from_queue(PolyhymniaMpdClient *self,
                                        guint               id,
                                        GError              **error);

GPtrArray *
polyhymnia_mpd_client_get_queue(PolyhymniaMpdClient *self,
                                GError              **error);

void
polyhymnia_mpd_client_play_song_from_queue(PolyhymniaMpdClient *self,
                                           guint               id,
                                           GError              **error);

void
polyhymnia_mpd_client_swap_songs_in_queue(PolyhymniaMpdClient *self,
                                          guint               id1,
                                          guint               id2,
                                          GError              **error);

G_END_DECLS
