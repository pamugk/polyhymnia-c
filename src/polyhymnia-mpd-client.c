
#include "polyhymnia-mpd-client.h"

#include <mpd/connection.h>

struct _PolyhymniaMpdClient
{
  GObject  parent_instance;

  struct mpd_connection   *mpd_connection;
};

G_DEFINE_FINAL_TYPE (PolyhymniaMpdClient, polyhymnia_mpd_client, G_TYPE_OBJECT)


static void
polyhymnia_mpd_client_finalize (GObject *gobject)
{
  PolyhymniaWindow *self = POLYHYMNIA_MPD_CLIENT (gobject);
  if (self->mpd_connection != NULL)
  {
    mpd_connection_free(self->mpd_connection);
  }

  G_OBJECT_CLASS (polyhymnia_mpd_client_parent_class)->finalize (gobject);
}

static void
polyhymnia_mpd_client_class_init (PolyhymniaMpdClientClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->finalize = polyhymnia_mpd_client_finalize;
}

static void
polyhymnia_mpd_client_init (PolyhymniaMpdClient *self)
{
  self->mpd_connection = mpd_connection_new(NULL, 0, 0);

  if (self->mpd_connection != NULL)
  {
    if (mpd_connection_get_error(self->mpd_connection) == MPD_ERROR_SUCCESS)
    {
      const unsigned *mpd_version = mpd_connection_get_server_version (self->mpd_connection);
      g_debug("Connected to MPD %d.%d.%d", mpd_version[0], mpd_version[1], mpd_version[2]);
    }
    else
    {
      g_warning("An error occurred on MPD connection initialization: %s\n",
              mpd_connection_get_error_message(self->mpd_connection));
    }
  }
}
