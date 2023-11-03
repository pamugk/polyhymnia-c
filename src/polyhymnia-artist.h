
#pragma once

#ifndef __POLYHYMNIA_ARTIST_H__
#define __POLYHYMNIA_ARTIST_H__

#include <glib-2.0/glib-object.h>

G_BEGIN_DECLS

#define POLYHYMNIA_TYPE_ARTIST (polyhymnia_artist_get_type())

G_DECLARE_FINAL_TYPE (PolyhymniaArtist, polyhymnia_artist, POLYHYMNIA, ARTIST, GObject)

G_END_DECLS

#endif /* __POLYHYMNIA_ARTIST_H__ */
