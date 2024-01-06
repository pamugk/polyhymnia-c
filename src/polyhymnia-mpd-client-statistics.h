
#pragma once

#include <glib-2.0/gio/gio.h>

#include "polyhymnia-mpd-client-common.h"
#include "polyhymnia-statistics.h"

G_BEGIN_DECLS

PolyhymniaStatistics *
polyhymnia_mpd_client_get_statistics (PolyhymniaMpdClient *self,
                                      GError             **error);

void
polyhymnia_mpd_client_get_statistics_async (PolyhymniaMpdClient *self,
                                            GCancellable        *cancellable,
                                            GAsyncReadyCallback  callback,
                                            void                *user_data);

PolyhymniaStatistics *
polyhymnia_mpd_client_get_statistics_finish (PolyhymniaMpdClient *self,
                                             GAsyncResult        *result,
                                             GError             **error);

G_END_DECLS
