
#pragma once

#include <glib-2.0/glib.h>

G_BEGIN_DECLS

gchar*
seconds_to_readable (guint duration);

gchar*
timespan_to_readable (gulong timespan);

G_END_DECLS
