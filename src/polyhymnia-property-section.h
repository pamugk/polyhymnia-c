
#pragma once

#include <gio/gio.h>

G_BEGIN_DECLS

#define POLYHYMNIA_TYPE_PROPERTY_SECTION (polyhymnia_property_section_get_type())

G_DECLARE_FINAL_TYPE (PolyhymniaPropertySection, polyhymnia_property_section,
                      POLYHYMNIA, PROPERTY_SECTION, GObject)

GListModel *
polyhymnia_property_section_get_model (PolyhymniaPropertySection *self);

void
polyhymnia_property_section_set_model (PolyhymniaPropertySection *self,
                                       GListModel                *model);

G_END_DECLS
