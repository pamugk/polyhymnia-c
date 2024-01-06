
#pragma once

#include <glib-2.0/glib.h>

#include "polyhymnia-mpd-client-common.h"

G_BEGIN_DECLS

void
polyhymnia_mpd_client_connect (PolyhymniaMpdClient *self,
                               GError             **error);

void
polyhymnia_mpd_client_rescan (PolyhymniaMpdClient *self,
                              GError              **error);

void
polyhymnia_mpd_client_scan (PolyhymniaMpdClient *self,
                            GError              **error);

G_END_DECLS
