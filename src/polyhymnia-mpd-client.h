
#pragma once

#include <glib-object.h>

G_BEGIN_DECLS

#define POLYHYMNIA_TYPE_MPD_CLIENT (polyhymnia_mpd_client_get_type())

G_DECLARE_FINAL_TYPE (PolyhymniaMpdClient, polyhymnia_mpd_client, POLYHYMNIA, MPD_CLIENT, GObject)

typedef enum
{
  POLYHYMNIA_MPD_CLIENT_ERROR_OOM,
  POLYHYMNIA_MPD_CLIENT_ERROR_FAIL,
} PolyhymniaMpdClientError;
#define POLYHYMNIA_MPD_CLIENT_ERROR polyhymnia_mpd_client_error_quark ()

GQuark polyhymnia_mpd_client_error_quark(void);

void polyhymnia_mpd_client_connect(PolyhymniaMpdClient *self,
                                    GError             **error);

void polyhymnia_mpd_client_scan(PolyhymniaMpdClient *self,
                                GError              **error);

G_END_DECLS
