
#include "polyhymnia-format-utils.h"

gchar*
seconds_to_readable(guint duration)
{
  gushort hours = duration / 3600;
  gushort minutes = (duration % 3600) / 60;
  gushort seconds = duration %  60;
  return hours == 0
    ? g_strdup_printf ("%02d:%02d", minutes, seconds)
    : g_strdup_printf ("%d:%02d:%02d", hours, minutes, seconds);
}

gchar*
timespan_to_readable (gulong duration)
{
  guint hours = duration / 3600;
  gushort minutes = (duration % 3600) / 60;
  gushort seconds = duration %  60;
  return hours == 0
    ? g_strdup_printf ("%02d:%02d", minutes, seconds)
    : g_strdup_printf ("%d:%02d:%02d", hours, minutes, seconds);
}
