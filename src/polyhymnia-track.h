
#pragma once

#ifndef __POLYHYMNIA_TRACK_H__
#define __POLYHYMNIA_TRACK_H__

#include <glib-2.0/glib-object.h>

G_BEGIN_DECLS

#define POLYHYMNIA_TYPE_TRACK (polyhymnia_track_get_type())

G_DECLARE_FINAL_TYPE (PolyhymniaTrack, polyhymnia_track, POLYHYMNIA, TRACK, GObject)

G_END_DECLS

#endif /* __POLYHYMNIA_TRACK_H__ */
