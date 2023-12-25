
#include "polyhymnia-output.h"

typedef enum
{
  PROP_ID = 1,
  PROP_NAME,
  PROP_PLUGIN,
  PROP_ENABLED,
  N_PROPERTIES,
} PolyhymniaOutputProperty;

struct _PolyhymniaOutput
{
  GObject  parent_instance;

  /* Data */
  guint    id;
  gchar    *name;
  gchar    *plugin;
  gboolean enabled;
};

G_DEFINE_FINAL_TYPE (PolyhymniaOutput, polyhymnia_output, G_TYPE_OBJECT)

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

static void
polyhymnia_output_finalize (GObject *gobject)
{
  PolyhymniaOutput *self = POLYHYMNIA_OUTPUT (gobject);

  g_free (self->name);
  g_free (self->plugin);

  G_OBJECT_CLASS (polyhymnia_output_parent_class)->finalize (gobject);
}

static void
polyhymnia_output_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  PolyhymniaOutput *self = POLYHYMNIA_OUTPUT (object);

  switch ((PolyhymniaOutputProperty) property_id)
    {
    case PROP_ID:
      g_value_set_uint (value, self->id);
      break;
    case PROP_NAME:
      g_value_set_string (value, self->name);
      break;
    case PROP_PLUGIN:
      g_value_set_string (value, self->plugin);
      break;
    case PROP_ENABLED:
      g_value_set_boolean (value, self->enabled);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
polyhymnia_output_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  PolyhymniaOutput *self = POLYHYMNIA_OUTPUT (object);

  switch ((PolyhymniaOutputProperty) property_id)
    {
    case PROP_ID:
      self->id = g_value_get_uint (value);
      break;
    case PROP_NAME:
      g_set_str (&(self->name), g_value_get_string (value));
      break;
    case PROP_PLUGIN:
      g_set_str (&(self->plugin), g_value_get_string (value));
      break;
    case PROP_ENABLED:
      self->enabled = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
polyhymnia_output_class_init (PolyhymniaOutputClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->finalize = polyhymnia_output_finalize;
  gobject_class->get_property = polyhymnia_output_get_property;
  gobject_class->set_property = polyhymnia_output_set_property;

  obj_properties[PROP_ID] =
    g_param_spec_uint ("id",
                       "Id",
                       "Id of an output.",
                       0, G_MAXUINT,
                       0,
                       G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  obj_properties[PROP_NAME] =
    g_param_spec_string ("name",
                         "Name",
                         "Name of an output.",
                         NULL,
                         G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  obj_properties[PROP_PLUGIN] =
    g_param_spec_string ("plugin",
                         "Plugin",
                         "Plugin that is used for an output.",
                         NULL,
                         G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  obj_properties[PROP_ENABLED] =
    g_param_spec_boolean ("enabled",
                          "Enabled",
                          "Whether an output is enabled.",
                          FALSE,
                          G_PARAM_CONSTRUCT | G_PARAM_READWRITE);

  g_object_class_install_properties (gobject_class,
                                     N_PROPERTIES,
                                     obj_properties);
}

static void
polyhymnia_output_init (PolyhymniaOutput *self)
{
}

/* Instance methods */
guint
polyhymnia_output_get_id (PolyhymniaOutput *self)
{
  g_return_val_if_fail (POLYHYMNIA_IS_OUTPUT (self), 0);
  return self->id;
}

const gchar *
polyhymnia_output_get_name (PolyhymniaOutput *self)
{
  g_return_val_if_fail (POLYHYMNIA_IS_OUTPUT (self), NULL);
  return self->name;
}

const gchar *
polyhymnia_output_get_plugin (PolyhymniaOutput *self)
{
  g_return_val_if_fail (POLYHYMNIA_IS_OUTPUT (self), NULL);
  return self->plugin;
}

gboolean
polyhymnia_output_get_enabled (PolyhymniaOutput *self)
{
  g_return_val_if_fail (POLYHYMNIA_IS_OUTPUT (self), FALSE);
  return self->enabled;
}
