
#pragma once

#include <adwaita.h>

G_BEGIN_DECLS

#define POLYHYMNIA_TYPE_SEARCH_PAGE (polyhymnia_search_page_get_type())

G_DECLARE_FINAL_TYPE (PolyhymniaSearchPage, polyhymnia_search_page, POLYHYMNIA, SEARCH_PAGE, AdwNavigationPage)

void
polyhymnia_search_page_set_search_query (PolyhymniaSearchPage *self,
                                         const char *search_query);

G_END_DECLS
