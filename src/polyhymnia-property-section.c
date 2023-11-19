
#include "polyhymnia-property-section.h"

#include <gtk/gtk.h>
typedef enum
{
  PROP_ITEM_TYPE = 1,
  PROP_MODEL,
  PROP_N_ITEMS,
  N_PROPERTIES,
} PolyhymniaPropertySectionProperty;

struct _PolyhymniaPropertySection
{
  GObject  parent_instance;

  /* Properties */
  GListModel *model;
};

static void
polyhymnia_property_section_list_model_interface_init (GListModelInterface *interface);
static void
polyhymnia_property_section_section_model_interface_init (GtkSectionModelInterface *interface);

G_DEFINE_TYPE_WITH_CODE (PolyhymniaPropertySection, polyhymnia_property_section, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (G_TYPE_LIST_MODEL,
                                               polyhymnia_property_section_list_model_interface_init)
                         G_IMPLEMENT_INTERFACE (GTK_TYPE_SECTION_MODEL,
                                                polyhymnia_property_section_section_model_interface_init))

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

/* Interface implementation */
static gpointer
polyhymnia_property_section_get_item (GListModel *list, guint position)
{
  g_return_val_if_fail (POLYHYMNIA_IS_PROPERTY_SECTION (list), NULL);
  g_return_val_if_fail (POLYHYMNIA_PROPERTY_SECTION (list)->model != NULL, NULL);
  return g_list_model_get_item (POLYHYMNIA_PROPERTY_SECTION (list)->model, position);
}

static GType
polyhymnia_property_section_get_item_type (GListModel *list)
{
  g_return_val_if_fail (POLYHYMNIA_IS_PROPERTY_SECTION (list), G_TYPE_OBJECT);
  g_return_val_if_fail (POLYHYMNIA_PROPERTY_SECTION (list)->model != NULL, G_TYPE_OBJECT);
  return g_list_model_get_item_type (POLYHYMNIA_PROPERTY_SECTION (list)->model);
}

static guint
polyhymnia_property_section_get_n_items(GListModel *list)
{
  g_return_val_if_fail (POLYHYMNIA_IS_PROPERTY_SECTION (list), 0);
  g_return_val_if_fail (POLYHYMNIA_PROPERTY_SECTION (list)->model != NULL, 0);
  return g_list_model_get_n_items (POLYHYMNIA_PROPERTY_SECTION (list)->model);
}

static void
polyhymnia_property_section_list_model_interface_init (GListModelInterface *interface)
{
  interface->get_item = polyhymnia_property_section_get_item;
  interface->get_item_type = polyhymnia_property_section_get_item_type;
  interface->get_n_items = polyhymnia_property_section_get_n_items;
}

static void
polyhymnia_property_section_get_section (GtkSectionModel *model,
                                         guint           position,
                                         guint           *out_start,
                                         guint           *out_end)
{
  guint n_items;
  PolyhymniaPropertySection *self;
  g_return_if_fail (out_start != NULL && out_end != NULL);

  if (!POLYHYMNIA_IS_PROPERTY_SECTION (model))
  {
    *out_start = 0;
    *out_end = 0;
    return;
  }

  self = POLYHYMNIA_PROPERTY_SECTION (model);

  if (self->model == NULL)
  {
    *out_start = 0;
    *out_end = 0;
    return;
  }

  n_items = g_list_model_get_n_items (self->model);
  *out_start = 0;
  *out_end = n_items;
}

static void
polyhymnia_property_section_section_model_interface_init (GtkSectionModelInterface *interface)
{
  interface->get_section = polyhymnia_property_section_get_section;
}

/* Class stuff */
static void
polyhymnia_property_section_constructed (GObject *gobject)
{
  PolyhymniaPropertySection *self = POLYHYMNIA_PROPERTY_SECTION (gobject);

  if (self->model != NULL)
  {

  }

  G_OBJECT_CLASS (polyhymnia_property_section_parent_class)->constructed (gobject);
}

static void
polyhymnia_property_section_dispose (GObject *gobject)
{
  PolyhymniaPropertySection *self = POLYHYMNIA_PROPERTY_SECTION (gobject);

  self->model = NULL;

  G_OBJECT_CLASS (polyhymnia_property_section_parent_class)->dispose (gobject);
}

static void
polyhymnia_property_section_get_property (GObject    *object,
                                          guint      property_id,
                                          GValue     *value,
                                          GParamSpec *pspec)
{
  PolyhymniaPropertySection *self = POLYHYMNIA_PROPERTY_SECTION (object);

  switch ((PolyhymniaPropertySectionProperty) property_id)
    {
    case PROP_ITEM_TYPE:
      g_value_set_gtype (value, polyhymnia_property_section_get_item_type (G_LIST_MODEL (object)));
      break;
    case PROP_MODEL:
      g_value_set_object (value, self->model);
    case PROP_N_ITEMS:
      g_value_set_uint (value, polyhymnia_property_section_get_n_items (G_LIST_MODEL (object)));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
polyhymnia_property_section_set_property (GObject      *object,
                                          guint        property_id,
                                          const GValue *value,
                                          GParamSpec   *pspec)
{
  PolyhymniaPropertySection *self = POLYHYMNIA_PROPERTY_SECTION (object);

  switch ((PolyhymniaPropertySectionProperty) property_id)
    {
    case PROP_MODEL:
      self->model = g_value_get_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
polyhymnia_property_section_class_init (PolyhymniaPropertySectionClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->constructed = polyhymnia_property_section_constructed;
  gobject_class->dispose = polyhymnia_property_section_dispose;
  gobject_class->get_property = polyhymnia_property_section_get_property;
  gobject_class->set_property = polyhymnia_property_section_set_property;

  obj_properties[PROP_ITEM_TYPE] =
    g_param_spec_gtype ("item-type", "", "", G_TYPE_OBJECT,
                        G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  obj_properties[PROP_MODEL] =
    g_param_spec_object ("model", "Model", "An underlying list model", G_TYPE_LIST_MODEL,
                        G_PARAM_CONSTRUCT | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
  obj_properties[PROP_N_ITEMS] =
    g_param_spec_uint ("n-items", "", "", 0, G_MAXUINT, 0,
                        G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
}

static void
polyhymnia_property_section_init (PolyhymniaPropertySection *self)
{

}

/* Instance methods */
GListModel *
polyhymnia_property_section_get_model (PolyhymniaPropertySection *self)
{
  g_return_val_if_fail (POLYHYMNIA_IS_PROPERTY_SECTION (self), NULL);
  return self->model;
}

void
polyhymnia_property_section_set_model (PolyhymniaPropertySection *self,
                                       GListModel                *model)
{
  g_return_if_fail (POLYHYMNIA_IS_PROPERTY_SECTION (self));

  self->model = model;
}
