
#pragma once

#include "polyhymnia-mpd-client-common.h"

#include "polyhymnia-player-state.h"

G_BEGIN_DECLS

PolyhymniaPlayerState
polyhymnia_mpd_client_get_state(PolyhymniaMpdClient *self,
                                 GError              **error);

int
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
                                    time_t              position,
                                    GError              **error);

void
polyhymnia_mpd_client_set_volume(PolyhymniaMpdClient *self,
                                 guint               volume,
                                 GError              **error);

G_END_DECLS
