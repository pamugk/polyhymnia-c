
#pragma once

#include <glib-2.0/glib.h>

#include "polyhymnia-mpd-client-common.h"

G_BEGIN_DECLS

GBytes *
polyhymnia_mpd_client_get_song_album_cover(PolyhymniaMpdClient *self,
                                           const gchar         *song_uri,
                                           GError              **error);

G_END_DECLS
