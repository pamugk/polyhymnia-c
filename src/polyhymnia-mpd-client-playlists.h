
#pragma once

#include <glib-2.0/glib.h>

#include "polyhymnia-mpd-client-common.h"

G_BEGIN_DECLS

GPtrArray *
polyhymnia_mpd_client_search_playlists (PolyhymniaMpdClient *self,
                                        GError              **error);

G_END_DECLS
