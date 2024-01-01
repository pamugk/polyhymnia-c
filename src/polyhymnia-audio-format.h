
#pragma once

#include <glib-2.0/glib-object.h>

G_BEGIN_DECLS

#define POLYHYMNIA_TYPE_AUDIO_FORMAT (polyhymnia_audio_format_get_type())

G_DECLARE_FINAL_TYPE (PolyhymniaAudioFormat, polyhymnia_audio_format, POLYHYMNIA, AUDIO_FORMAT, GObject)

unsigned char
polyhymnia_audio_format_get_bits (PolyhymniaAudioFormat *self);

unsigned char
polyhymnia_audio_format_get_channels (PolyhymniaAudioFormat *self);

unsigned int
polyhymnia_audio_format_get_sample_rate (PolyhymniaAudioFormat *self);

G_END_DECLS
