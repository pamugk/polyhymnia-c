
#pragma once

#include <glib-2.0/glib.h>

#include "polyhymnia-mpd-client-common.h"
#include "polyhymnia-statistics.h"

G_BEGIN_DECLS

PolyhymniaStatistics *
polyhymnia_mpd_client_get_statistics (PolyhymniaMpdClient *self,
                                      GError              **error);

G_END_DECLS
