
#pragma once

#include <adwaita.h>

G_BEGIN_DECLS

#define POLYHYMNIA_TYPE_APPLICATION (polyhymnia_application_get_type())

G_DECLARE_FINAL_TYPE (PolyhymniaApplication, polyhymnia_application, POLYHYMNIA, APPLICATION, AdwApplication)

PolyhymniaApplication *polyhymnia_application_new (const char        *application_id,
                                                   GApplicationFlags  flags);

G_END_DECLS
