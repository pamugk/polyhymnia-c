
#pragma once

#include "polyhymnia-mpd-client-common.h"

#include "polyhymnia-player-state.h"

G_BEGIN_DECLS
void
polyhymnia_mpd_client_change_volume(PolyhymniaMpdClient *self,
                                    gint8               volume_diff,
                                    GError              **error);

PolyhymniaPlayerPlaybackOptions
polyhymnia_mpd_client_get_playback_options(PolyhymniaMpdClient *self,
                                           GError              **error);

PolyhymniaPlayerPlaybackState
polyhymnia_mpd_client_get_playback_state(PolyhymniaMpdClient *self,
                                         GError              **error);

PolyhymniaTrack *
polyhymnia_mpd_client_get_song_from_queue(PolyhymniaMpdClient *self,
                                          guint               id,
                                          GError              **error);

PolyhymniaPlayerState
polyhymnia_mpd_client_get_state(PolyhymniaMpdClient *self,
                                 GError              **error);

guint
polyhymnia_mpd_client_get_volume(PolyhymniaMpdClient *self,
                                 GError              **error);

void
polyhymnia_mpd_client_play_next(PolyhymniaMpdClient *self,
                                GError              **error);

void
polyhymnia_mpd_client_play_previous(PolyhymniaMpdClient *self,
                                    GError              **error);

void
polyhymnia_mpd_client_pause_playback(PolyhymniaMpdClient *self,
                                     GError              **error);

void
polyhymnia_mpd_client_resume_playback(PolyhymniaMpdClient *self,
                                      GError              **error);

void
polyhymnia_mpd_client_seek_playback(PolyhymniaMpdClient *self,
                                    guint               id,
                                    guint               position,
                                    GError              **error);

void
polyhymnia_mpd_client_set_volume(PolyhymniaMpdClient *self,
                                 guint               volume,
                                 GError              **error);

void
polyhymnia_mpd_client_toggle_random_order(PolyhymniaMpdClient *self,
                                          gboolean            new_value,
                                          GError              **error);

void
polyhymnia_mpd_client_toggle_repeat(PolyhymniaMpdClient *self,
                                    gboolean            new_value,
                                    GError              **error);

G_END_DECLS
