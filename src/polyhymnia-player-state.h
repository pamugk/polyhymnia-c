
#pragma once

#include "polyhymnia-player-playback-status.h"
#include "polyhymnia-track.h"

G_BEGIN_DECLS

typedef struct
{
  gboolean random;
  gboolean repeat;
} PolyhymniaPlayerPlaybackOptions;

typedef struct
{
  gint            current_track_id;
  guint           elapsed_seconds;
  gboolean        has_next;
  gboolean        has_previous;
  PolyhymniaPlayerPlaybackStatus  playback_status;
} PolyhymniaPlayerPlaybackState;

typedef struct
{
  gboolean                        audio_available;
  PolyhymniaTrack                 *current_track;
  PolyhymniaPlayerPlaybackOptions playback_options;
  PolyhymniaPlayerPlaybackState   playback_state;
  guint                           volume;
} PolyhymniaPlayerState;

G_END_DECLS
