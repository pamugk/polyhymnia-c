
#pragma once

#include <adwaita.h>

G_BEGIN_DECLS

#define POLYHYMNIA_TYPE_LAST_MODIFIED_PAGE (polyhymnia_last_modified_page_get_type())

G_DECLARE_FINAL_TYPE (PolyhymniaLastModifiedPage, polyhymnia_last_modified_page,
                      POLYHYMNIA, LAST_MODIFIED_PAGE, AdwNavigationPage)

G_END_DECLS
