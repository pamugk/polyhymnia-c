
#include "polyhymnia-audio-format.h"

/* Type metadata */
typedef enum
{
  PROP_BITS = 1,
  PROP_CHANNELS,
  PROP_SAMPLE_RATE,
  N_PROPERTIES,
} PolyhymniaAudioFormatProperty;

struct _PolyhymniaAudioFormat
{
  GObject  parent_instance;

  /* Data */
  unsigned char bits;
  unsigned char channels;
  unsigned int sample_rate;
};

G_DEFINE_FINAL_TYPE (PolyhymniaAudioFormat, polyhymnia_audio_format, G_TYPE_OBJECT)

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

/* Class stuff */
static void
polyhymnia_audio_format_get_property (GObject    *object,
                                      guint       property_id,
                                      GValue     *value,
                                      GParamSpec *pspec)
{
  PolyhymniaAudioFormat *self = POLYHYMNIA_AUDIO_FORMAT (object);

  switch ((PolyhymniaAudioFormatProperty) property_id)
    {
    case PROP_BITS:
      g_value_set_uchar (value, self->bits);
      break;
    case PROP_CHANNELS:
      g_value_set_uchar (value, self->channels);
      break;
    case PROP_SAMPLE_RATE:
      g_value_set_uint (value, self->sample_rate);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
polyhymnia_audio_format_set_property (GObject      *object,
                                      guint         property_id,
                                      const GValue *value,
                                      GParamSpec   *pspec)
{
  PolyhymniaAudioFormat *self = POLYHYMNIA_AUDIO_FORMAT (object);

  switch ((PolyhymniaAudioFormatProperty) property_id)
    {
    case PROP_BITS:
      self->bits = g_value_get_uchar (value);
      break;
    case PROP_CHANNELS:
      self->channels = g_value_get_uchar (value);
      break;
    case PROP_SAMPLE_RATE:
      self->sample_rate = g_value_get_uint (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
polyhymnia_audio_format_class_init (PolyhymniaAudioFormatClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->get_property = polyhymnia_audio_format_get_property;
  gobject_class->set_property = polyhymnia_audio_format_set_property;

  obj_properties[PROP_BITS] =
    g_param_spec_uchar ("bits",
                       "Bits",
                       "The number of significant bits per sample",
                       0, G_MAXUINT8, 0,
                       G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  obj_properties[PROP_CHANNELS] =
    g_param_spec_uchar ("channels",
                       "Channels",
                       "The number of channels",
                       0, G_MAXUINT8, 0,
                       G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  obj_properties[PROP_SAMPLE_RATE] =
    g_param_spec_uint ("sample-rate",
                       "Sample rate",
                       "The sample rate in Hz",
                       0, G_MAXUINT, 0,
                       G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);

  g_object_class_install_properties (gobject_class,
                                     N_PROPERTIES,
                                     obj_properties);
}

static void
polyhymnia_audio_format_init (PolyhymniaAudioFormat *self)
{
}

/* Instance methods */
unsigned char
polyhymnia_audio_format_get_bits (PolyhymniaAudioFormat *self)
{
  g_return_val_if_fail (POLYHYMNIA_IS_AUDIO_FORMAT (self), 0);
  return self->bits;
}

unsigned char
polyhymnia_audio_format_get_channels (PolyhymniaAudioFormat *self)
{
  g_return_val_if_fail (POLYHYMNIA_IS_AUDIO_FORMAT (self), 0);
  return self->channels;
}

unsigned int
polyhymnia_audio_format_get_sample_rate (PolyhymniaAudioFormat *self)
{
  g_return_val_if_fail (POLYHYMNIA_IS_AUDIO_FORMAT (self), 0);
  return self->sample_rate;
}
