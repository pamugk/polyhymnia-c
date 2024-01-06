
#pragma once

#include <glib-2.0/glib.h>

#include "polyhymnia-mpd-client-common.h"
#include "polyhymnia-output.h"

G_BEGIN_DECLS

GPtrArray *
polyhymnia_mpd_client_get_outputs (PolyhymniaMpdClient *self,
                                   GError              **error);

void
polyhymnia_mpd_client_toggle_output (PolyhymniaMpdClient *self,
                                     unsigned int         output_id,
                                     GError             **error);

G_END_DECLS
