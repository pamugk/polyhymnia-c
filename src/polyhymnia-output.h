
#pragma once

#include <glib-2.0/glib-object.h>

G_BEGIN_DECLS

#define POLYHYMNIA_TYPE_OUTPUT (polyhymnia_output_get_type())

G_DECLARE_FINAL_TYPE (PolyhymniaOutput, polyhymnia_output, POLYHYMNIA, OUTPUT, GObject)

guint
polyhymnia_output_get_id (PolyhymniaOutput *self);

const gchar *
polyhymnia_output_get_name (PolyhymniaOutput *self);

const gchar *
polyhymnia_output_get_plugin (PolyhymniaOutput *self);

gboolean
polyhymnia_output_get_enabled (PolyhymniaOutput *self);

G_END_DECLS
