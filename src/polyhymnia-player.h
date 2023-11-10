
#pragma once

#include <glib-object.h>

#include "polyhymnia-player-playback-status.h"
#include "polyhymnia-track.h"

G_BEGIN_DECLS

#define POLYHYMNIA_TYPE_PLAYER (polyhymnia_player_get_type())

G_DECLARE_FINAL_TYPE (PolyhymniaPlayer, polyhymnia_player, POLYHYMNIA, PLAYER, GObject)

const PolyhymniaTrack *
polyhymnia_player_get_current_track (const PolyhymniaPlayer *self);

PolyhymniaPlayerPlaybackStatus
polyhymnia_player_get_playback_status (const PolyhymniaPlayer *self);

G_END_DECLS
