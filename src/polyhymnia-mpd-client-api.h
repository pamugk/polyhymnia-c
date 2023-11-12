
#pragma once

#include "polyhymnia-mpd-client-common.h"

#include "polyhymnia-album.h"
#include "polyhymnia-artist.h"
#include "polyhymnia-track.h"

G_BEGIN_DECLS

gint
polyhymnia_mpd_client_add_next_to_queue(PolyhymniaMpdClient *self,
                                        const gchar         *song_uri,
                                        GError              **error);

void
polyhymnia_mpd_client_append_album_to_queue(PolyhymniaMpdClient *self,
                                            const gchar         *album,
                                            GError              **error);

void
polyhymnia_mpd_client_append_artist_to_queue(PolyhymniaMpdClient *self,
                                             const gchar         *artist,
                                             GError              **error);

gint
polyhymnia_mpd_client_append_to_queue(PolyhymniaMpdClient *self,
                                      const gchar         *song_uri,
                                      GError              **error);

void
polyhymnia_mpd_client_connect(PolyhymniaMpdClient *self,
                              GError             **error);

void
polyhymnia_mpd_client_play (PolyhymniaMpdClient *self,
                            GError              **error);

void
polyhymnia_mpd_client_play_album(PolyhymniaMpdClient *self,
                                 const gchar         *album,
                                 GError              **error);

void
polyhymnia_mpd_client_play_artist(PolyhymniaMpdClient *self,
                                  const gchar         *artist,
                                  GError              **error);

gint
polyhymnia_mpd_client_play_song(PolyhymniaMpdClient *self,
                                const gchar         *song_uri,
                                GError              **error);

void
polyhymnia_mpd_client_scan(PolyhymniaMpdClient *self,
                           GError              **error);

GPtrArray *
polyhymnia_mpd_client_search_albums(PolyhymniaMpdClient *self,
                                    GError              **error);

GPtrArray *
polyhymnia_mpd_client_search_artists(PolyhymniaMpdClient *self,
                                      GError              **error);

GPtrArray *
polyhymnia_mpd_client_search_genres(PolyhymniaMpdClient *self,
                                    GError              **error);

GPtrArray *
polyhymnia_mpd_client_search_tracks(PolyhymniaMpdClient *self,
                                    const gchar         *query,
                                    GError              **error);

G_END_DECLS
