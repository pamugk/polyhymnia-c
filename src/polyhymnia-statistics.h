
#pragma once

#include <glib-2.0/glib-object.h>

G_BEGIN_DECLS

#define POLYHYMNIA_TYPE_STATISTICS (polyhymnia_statistics_get_type())

G_DECLARE_FINAL_TYPE (PolyhymniaStatistics, polyhymnia_statistics, POLYHYMNIA, STATISTICS, GObject)

guint
polyhymnia_statistics_get_albums_count (PolyhymniaStatistics *self);

guint
polyhymnia_statistics_get_artists_count (PolyhymniaStatistics *self);

gulong
polyhymnia_statistics_get_db_play_time (PolyhymniaStatistics *self);

gulong
polyhymnia_statistics_get_db_last_update (PolyhymniaStatistics *self);

gulong
polyhymnia_statistics_get_mpd_play_time (PolyhymniaStatistics *self);

gulong
polyhymnia_statistics_get_mpd_uptime (PolyhymniaStatistics *self);

guint
polyhymnia_statistics_get_tracks_count (PolyhymniaStatistics *self);

G_END_DECLS
