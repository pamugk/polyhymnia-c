
#pragma once

#include <glib-2.0/glib.h>

#include "polyhymnia-mpd-client-common.h"
#include "polyhymnia-track-full-info.h"

G_BEGIN_DECLS

PolyhymniaTrackFullInfo *
polyhymnia_mpd_client_get_song_details (PolyhymniaMpdClient *self,
                                        const gchar         *song_uri,
                                        GError              **error);

G_END_DECLS
