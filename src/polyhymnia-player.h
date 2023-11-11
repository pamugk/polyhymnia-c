
#pragma once

#include <glib-object.h>

#include "polyhymnia-player-playback-status.h"
#include "polyhymnia-track.h"

G_BEGIN_DECLS

#define POLYHYMNIA_TYPE_PLAYER (polyhymnia_player_get_type())

G_DECLARE_FINAL_TYPE (PolyhymniaPlayer, polyhymnia_player, POLYHYMNIA, PLAYER, GObject)

void
polyhymnia_player_change_volume (PolyhymniaPlayer *self,
                                 gint8             volume_diff,
                                 GError           **error);

const PolyhymniaTrack *
polyhymnia_player_get_current_track (const PolyhymniaPlayer *self);

GBytes *
polyhymnia_player_get_current_track_album_cover (PolyhymniaPlayer *self,
                                                 GError           **error);

guint
polyhymnia_player_get_elapsed (const PolyhymniaPlayer *self);

PolyhymniaPlayerPlaybackStatus
polyhymnia_player_get_playback_status (const PolyhymniaPlayer *self);

void
polyhymnia_player_play_next (PolyhymniaPlayer *self,
                             GError           **error);

void
polyhymnia_player_play_previous (PolyhymniaPlayer *self,
                                 GError           **error);

void
polyhymnia_player_toggle_playback_state (PolyhymniaPlayer *self,
                                         GError           **error);

G_END_DECLS
