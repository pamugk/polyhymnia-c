
#pragma once

#include <glib-2.0/glib-object.h>

G_BEGIN_DECLS

#define POLYHYMNIA_TYPE_STATISTICS (polyhymnia_statistics_get_type())

G_DECLARE_FINAL_TYPE (PolyhymniaStatistics, polyhymnia_statistics, POLYHYMNIA, STATISTICS, GObject)

G_END_DECLS
