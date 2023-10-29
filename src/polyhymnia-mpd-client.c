
#include "polyhymnia-mpd-client.h"

#include <mpd/client.h>

typedef enum
{
  PROP_SCAN_AVAILABLE = 1,
  N_PROPERTIES,
} PolyhymniaMpdClientProperty;

struct _PolyhymniaMpdClient
{
  GObject  parent_instance;

  /* Underlying MPD fields */
  struct mpd_connection   *mpd_connection;

  /* State fields */
  gboolean                  initialized;
};

G_DEFINE_FINAL_TYPE (PolyhymniaMpdClient, polyhymnia_mpd_client, G_TYPE_OBJECT)

G_DEFINE_QUARK (PolyhymniaMpdClient, polyhymnia_mpd_client_error);

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

/* Class stuff - constructors, destructors, etc */
static void
polyhymnia_mpd_client_constructed (GObject *obj)
{
  G_OBJECT_CLASS (polyhymnia_mpd_client_parent_class)->constructed (obj);
}

static GObject*
polyhymnia_mpd_client_constructor (GType type,
             guint n_construct_params,
             GObjectConstructParam *construct_params)
{
  static GObject *self = NULL;

  if (self == NULL)
  {
    self = G_OBJECT_CLASS (polyhymnia_mpd_client_parent_class)->constructor (
          type, n_construct_params, construct_params);
    g_object_add_weak_pointer (self, (gpointer) &self);
    return self;
  }

  return g_object_ref (self);
}

static void
polyhymnia_mpd_client_finalize (GObject *gobject)
{
  PolyhymniaMpdClient *self = POLYHYMNIA_MPD_CLIENT (gobject);
  if (self->mpd_connection != NULL)
  {
    mpd_connection_free(self->mpd_connection);
  }

  G_OBJECT_CLASS (polyhymnia_mpd_client_parent_class)->finalize (gobject);
}

static void
polyhymnia_mpd_client_set_property (GObject      *object,
                          guint         property_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  PolyhymniaMpdClient *self = POLYHYMNIA_MPD_CLIENT (object);

  switch ((PolyhymniaMpdClientProperty) property_id)
    {
    case PROP_SCAN_AVAILABLE:
      self->initialized = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
polyhymnia_mpd_client_get_property (GObject    *object,
                          guint       property_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
  PolyhymniaMpdClient *self = POLYHYMNIA_MPD_CLIENT (object);

  switch ((PolyhymniaMpdClientProperty) property_id)
    {
    case PROP_SCAN_AVAILABLE:
      g_value_set_boolean (value, self->initialized);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
polyhymnia_mpd_client_class_init (PolyhymniaMpdClientClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->constructed = polyhymnia_mpd_client_constructed;
  gobject_class->constructor = polyhymnia_mpd_client_constructor;
  gobject_class->finalize = polyhymnia_mpd_client_finalize;
  gobject_class->get_property = polyhymnia_mpd_client_get_property;
  gobject_class->set_property = polyhymnia_mpd_client_set_property;

  obj_properties[PROP_SCAN_AVAILABLE] =
    g_param_spec_boolean ("initialized",
                         "Initialized",
                         "Whether MPD connection is established.",
                         FALSE,
                         G_PARAM_READWRITE | G_PARAM_STATIC_NAME);

  g_object_class_install_properties (gobject_class,
                                     N_PROPERTIES,
                                     obj_properties);
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
      g_object_set(G_OBJECT (self), "initialized", TRUE, NULL);
      g_debug("Connected to MPD %d.%d.%d", mpd_version[0], mpd_version[1], mpd_version[2]);
    }
    else
    {
      g_warning("An error occurred on MPD connection initialization: %s\n",
              mpd_connection_get_error_message(self->mpd_connection));
      mpd_connection_free (self->mpd_connection);
      self->mpd_connection = NULL;
    }
  }
}

/* Instance methods */
void
polyhymnia_mpd_client_scan(PolyhymniaMpdClient *self,
                           GError              **error)
{
  guint scan_job_id;

  g_return_if_fail (POLYHYMNIA_IS_MPD_CLIENT (self));
  g_return_if_fail (error == NULL || *error == NULL);

  scan_job_id = mpd_run_update (self->mpd_connection, NULL);
  if (scan_job_id > 0)
  {
    g_debug ("Scanning...");
  }
  else
  {
    g_set_error (error,
                 POLYHYMNIA_MPD_CLIENT_ERROR,
                 POLYHYMNIA_MPD_CLIENT_ERROR_SCAN_FAILED,
                 "MPD error - %s",
                 mpd_connection_get_error_message(self->mpd_connection));
  }
}
