
#pragma once

#include "polyhymnia-player-playback-status.h"
#include "polyhymnia-track.h"

G_BEGIN_DECLS

typedef struct
{
  gboolean        audio_available;
  PolyhymniaTrack *current_track;
  guint           elapsed_seconds;
  gboolean        has_next;
  gboolean        has_previous;
  gboolean        random;
  gboolean        repeat;
  guint           volume;

  PolyhymniaPlayerPlaybackStatus playback_status;
} PolyhymniaPlayerState;

G_END_DECLS
