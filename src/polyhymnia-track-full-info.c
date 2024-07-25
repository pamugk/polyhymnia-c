
#include "polyhymnia-track-full-info.h"

/* Type metadata */
typedef enum
{
  PROP_ALBUM = 1,
  PROP_ALBUM_ARTIST,
  PROP_ARTISTS,
  PROP_AUDIO_FORMAT,
  PROP_COMMENT,
  PROP_COMPOSERS,
  PROP_CONDUCTORS,
  PROP_DATE,
  PROP_DISC,
  PROP_ENSEMBLE,
  PROP_GENRE,
  PROP_LOCATION,
  PROP_MOVEMENT,
  PROP_MOVEMENT_NUMBER,
  PROP_MUSICBRAINZ_ALBUM_ID,
  PROP_MUSICBRAINZ_ALBUM_ARTIST_ID,
  PROP_MUSICBRAINZ_ARTIST_ID,
  PROP_MUSICBRAINZ_RELEASE_GROUP_ID,
  PROP_MUSICBRAINZ_RELEASE_TRACK_ID,
  PROP_MUSICBRAINZ_TRACK_ID,
  PROP_MUSICBRAINZ_WORK_ID,
  PROP_ORIGINAL_DATE,
  PROP_PERFORMERS,
  PROP_POSITION,
  PROP_PUBLISHER,
  PROP_TITLE,
  PROP_URI,
  PROP_WORK,
  N_PROPERTIES,
} PolyhymniaTrackFullInfoProperty;

struct _PolyhymniaTrackFullInfo
{
  GObject  parent_instance;

  /* Data */
  char                  *album;
  char                  *album_artist;
  char                  *artists;
  PolyhymniaAudioFormat *audio_format;
  char                  *comment;
  char                  *composers;
  char                  *conductors;
  char                  *date;
  char                  *disc;
  char                  *ensemble;
  char                  *genre;
  char                  *location;
  char                  *movement;
  char                  *movement_number;
  char                  *musicbrainz_album_id;
  char                  *musicbrainz_album_artist_id;
  char                  *musicbrainz_artist_id;
  char                  *musicbrainz_release_group_id;
  char                  *musicbrainz_release_track_id;
  char                  *musicbrainz_track_id;
  char                  *musicbrainz_work_id;
  char                  *original_date;
  char                  *performers;
  char                  *position;
  char                  *publisher;
  char                  *title;
  char                  *uri;
  char                  *work;
};

G_DEFINE_FINAL_TYPE (PolyhymniaTrackFullInfo, polyhymnia_track_full_info, G_TYPE_OBJECT)

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

/* Class stuff */
static void
polyhymnia_track_full_info_finalize (GObject *gobject)
{
  PolyhymniaTrackFullInfo *self = POLYHYMNIA_TRACK_FULL_INFO (gobject);

  g_free (self->album);
  g_free (self->album_artist);
  g_free (self->artists);
  g_clear_object (&(self->audio_format));
  g_free (self->comment);
  g_free (self->composers);
  g_free (self->conductors);
  g_free (self->date);
  g_free (self->disc);
  g_free (self->ensemble);
  g_free (self->genre);
  g_free (self->location);
  g_free (self->movement);
  g_free (self->movement_number);
  g_free (self->musicbrainz_album_id);
  g_free (self->musicbrainz_album_artist_id);
  g_free (self->musicbrainz_artist_id);
  g_free (self->musicbrainz_release_group_id);
  g_free (self->musicbrainz_release_track_id);
  g_free (self->musicbrainz_track_id);
  g_free (self->musicbrainz_work_id);
  g_free (self->original_date);
  g_free (self->performers);
  g_free (self->position);
  g_free (self->publisher);
  g_free (self->title);
  g_free (self->uri);
  g_free (self->work);

  G_OBJECT_CLASS (polyhymnia_track_full_info_parent_class)->finalize (gobject);
}

static void
polyhymnia_track_full_info_get_property (GObject    *object,
                                         guint       property_id,
                                         GValue     *value,
                                         GParamSpec *pspec)
{
  PolyhymniaTrackFullInfo *self = POLYHYMNIA_TRACK_FULL_INFO (object);

  switch ((PolyhymniaTrackFullInfoProperty) property_id)
    {
    case PROP_ALBUM:
      g_value_set_string (value, self->album);
      break;
    case PROP_ALBUM_ARTIST:
      g_value_set_string (value, self->album_artist);
      break;
    case PROP_ARTISTS:
      g_value_set_string (value, self->artists);
      break;
    case PROP_AUDIO_FORMAT:
      g_value_set_object (value, self->audio_format);
      break;
    case PROP_COMMENT:
      g_value_set_string (value, self->comment);
      break;
    case PROP_COMPOSERS:
      g_value_set_string (value, self->composers);
      break;
    case PROP_CONDUCTORS:
      g_value_set_string (value, self->conductors);
      break;
    case PROP_DATE:
      g_value_set_string (value, self->date);
      break;
    case PROP_DISC:
      g_value_set_string (value, self->disc);
      break;
    case PROP_ENSEMBLE:
      g_value_set_string (value, self->ensemble);
      break;
    case PROP_GENRE:
      g_value_set_string (value, self->genre);
      break;
    case PROP_LOCATION:
      g_value_set_string (value, self->location);
      break;
    case PROP_MOVEMENT:
      g_value_set_string (value, self->movement);
      break;
    case PROP_MOVEMENT_NUMBER:
      g_value_set_string (value, self->movement_number);
      break;
    case PROP_MUSICBRAINZ_ALBUM_ID:
      g_value_set_string (value, self->musicbrainz_album_id);
      break;
    case PROP_MUSICBRAINZ_ALBUM_ARTIST_ID:
      g_value_set_string (value, self->musicbrainz_album_artist_id);
      break;
    case PROP_MUSICBRAINZ_ARTIST_ID:
      g_value_set_string (value, self->musicbrainz_artist_id);
      break;
    case PROP_MUSICBRAINZ_RELEASE_GROUP_ID:
      g_value_set_string (value, self->musicbrainz_release_group_id);
      break;
    case PROP_MUSICBRAINZ_RELEASE_TRACK_ID:
      g_value_set_string (value, self->musicbrainz_release_track_id);
      break;
    case PROP_MUSICBRAINZ_TRACK_ID:
      g_value_set_string (value, self->musicbrainz_track_id);
      break;
    case PROP_MUSICBRAINZ_WORK_ID:
      g_value_set_string (value, self->musicbrainz_work_id);
      break;
    case PROP_ORIGINAL_DATE:
      g_value_set_string (value, self->original_date);
      break;
    case PROP_PERFORMERS:
      g_value_set_string (value, self->performers);
      break;
    case PROP_POSITION:
      g_value_set_string (value, self->position);
      break;
    case PROP_PUBLISHER:
      g_value_set_string (value, self->publisher);
      break;
    case PROP_TITLE:
      g_value_set_string (value, self->title);
      break;
    case PROP_URI:
      g_value_set_string (value, self->uri);
      break;
    case PROP_WORK:
      g_value_set_string (value, self->work);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
polyhymnia_track_full_info_set_property (GObject      *object,
                                         guint         property_id,
                                         const GValue *value,
                                         GParamSpec   *pspec)
{
  PolyhymniaTrackFullInfo *self = POLYHYMNIA_TRACK_FULL_INFO (object);

  switch ((PolyhymniaTrackFullInfoProperty) property_id)
    {
    case PROP_ALBUM:
      g_set_str (&(self->album), g_value_get_string (value));
      break;
    case PROP_ALBUM_ARTIST:
      g_set_str (&(self->album_artist), g_value_get_string (value));
      break;
    case PROP_ARTISTS:
      g_set_str (&(self->artists), g_value_get_string (value));
      break;
    case PROP_AUDIO_FORMAT:
      self->audio_format = g_value_get_object (value);
      break;
    case PROP_COMMENT:
      g_set_str (&(self->comment), g_value_get_string (value));
      break;
    case PROP_COMPOSERS:
      g_set_str (&(self->composers), g_value_get_string (value));
      break;
    case PROP_CONDUCTORS:
      g_set_str (&(self->conductors), g_value_get_string (value));
      break;
    case PROP_DATE:
      g_set_str (&(self->date), g_value_get_string (value));
      break;
    case PROP_DISC:
      g_set_str (&(self->disc), g_value_get_string (value));
      break;
    case PROP_ENSEMBLE:
      g_set_str (&(self->ensemble), g_value_get_string (value));
      break;
    case PROP_GENRE:
      g_set_str (&(self->genre), g_value_get_string (value));
      break;
    case PROP_LOCATION:
      g_set_str (&(self->location), g_value_get_string (value));
      break;
    case PROP_MOVEMENT:
      g_set_str (&(self->movement), g_value_get_string (value));
      break;
    case PROP_MOVEMENT_NUMBER:
      g_set_str (&(self->movement_number), g_value_get_string (value));
      break;
    case PROP_MUSICBRAINZ_ALBUM_ID:
      g_set_str (&(self->musicbrainz_album_id), g_value_get_string (value));
      break;
    case PROP_MUSICBRAINZ_ALBUM_ARTIST_ID:
      g_set_str (&(self->musicbrainz_album_artist_id), g_value_get_string (value));
      break;
    case PROP_MUSICBRAINZ_ARTIST_ID:
      g_set_str (&(self->musicbrainz_artist_id), g_value_get_string (value));
      break;
    case PROP_MUSICBRAINZ_RELEASE_GROUP_ID:
      g_set_str (&(self->musicbrainz_release_group_id), g_value_get_string (value));
      break;
    case PROP_MUSICBRAINZ_RELEASE_TRACK_ID:
      g_set_str (&(self->musicbrainz_release_track_id), g_value_get_string (value));
      break;
    case PROP_MUSICBRAINZ_TRACK_ID:
      g_set_str (&(self->musicbrainz_track_id), g_value_get_string (value));
      break;
    case PROP_MUSICBRAINZ_WORK_ID:
      g_set_str (&(self->musicbrainz_work_id), g_value_get_string (value));
      break;
    case PROP_ORIGINAL_DATE:
      g_set_str (&(self->original_date), g_value_get_string (value));
      break;
    case PROP_PERFORMERS:
      g_set_str (&(self->performers), g_value_get_string (value));
      break;
    case PROP_POSITION:
      g_set_str (&(self->position), g_value_get_string (value));
      break;
    case PROP_PUBLISHER:
      g_set_str (&(self->publisher), g_value_get_string (value));
      break;
    case PROP_TITLE:
      g_set_str (&(self->title), g_value_get_string (value));
      break;
    case PROP_URI:
      g_set_str (&(self->uri), g_value_get_string (value));
      break;
    case PROP_WORK:
      g_set_str (&(self->work), g_value_get_string (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
polyhymnia_track_full_info_class_init (PolyhymniaTrackFullInfoClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->finalize = polyhymnia_track_full_info_finalize;
  gobject_class->get_property = polyhymnia_track_full_info_get_property;
  gobject_class->set_property = polyhymnia_track_full_info_set_property;

  obj_properties[PROP_ALBUM] =
    g_param_spec_string ("album",
                         "Album",
                         "Title of source album",
                         NULL,
                         G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  obj_properties[PROP_ALBUM_ARTIST] =
    g_param_spec_string ("album-artist",
                         "Album artist",
                         "Name of a main album artist",
                         NULL,
                         G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  obj_properties[PROP_ARTISTS] =
    g_param_spec_string ("artists",
                         "Artists",
                         "The artist name",
                         NULL,
                         G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  obj_properties[PROP_AUDIO_FORMAT] =
    g_param_spec_object ("audio-format",
                         "Audio format",
                         "Audio format of a song",
                         POLYHYMNIA_TYPE_AUDIO_FORMAT,
                         G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  obj_properties[PROP_COMMENT] =
    g_param_spec_string ("comment",
                         "Comment",
                         "A human-readable comment about this song",
                         NULL,
                         G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  obj_properties[PROP_COMPOSERS] =
    g_param_spec_string ("composers",
                         "Composers",
                         "The artist who composed the song",
                         NULL,
                         G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  obj_properties[PROP_CONDUCTORS] =
    g_param_spec_string ("conductors",
                         "Conductors",
                         "The conductor who conducted the song",
                         NULL,
                         G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  obj_properties[PROP_DATE] =
    g_param_spec_string ("date",
                         "Date",
                         "The song’s release date. This is usually a 4-digit year",
                         NULL,
                         G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  obj_properties[PROP_DISC] =
    g_param_spec_string ("disc",
                         "Disc",
                         "Disc number of an album that track belongs to",
                         NULL,
                         G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  obj_properties[PROP_ENSEMBLE] =
    g_param_spec_string ("ensemble",
                         "Ensemble",
                         "The ensemble performing this song, e.g. “Wiener Philharmoniker”",
                         NULL,
                         G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  obj_properties[PROP_GENRE] =
    g_param_spec_string ("genre",
                         "Genre",
                         "The music genre",
                         NULL,
                         G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  obj_properties[PROP_LOCATION] =
    g_param_spec_string ("location",
                         "Location",
                         "Location of the recording, e.g. “Royal Albert Hall”",
                         NULL,
                         G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  obj_properties[PROP_MOVEMENT] =
    g_param_spec_string ("movement",
                         "Movement",
                         "Name of the movement, e.g. “Andante con moto”",
                         NULL,
                         G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  obj_properties[PROP_MOVEMENT_NUMBER] =
    g_param_spec_string ("movement-number",
                         "Movement number",
                         "Movement number, e.g. “2” or “II”",
                         NULL,
                         G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  obj_properties[PROP_MUSICBRAINZ_ALBUM_ID] =
    g_param_spec_string ("musicbrainz-album-id",
                         "MusicBrainz album id",
                         "The album id in the MusicBrainz database",
                         NULL,
                         G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  obj_properties[PROP_MUSICBRAINZ_ALBUM_ARTIST_ID] =
    g_param_spec_string ("musicbrainz-album-artist-id",
                         "MusicBrainz album artist id",
                         "The album artist id in the MusicBrainz database",
                         NULL,
                         G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  obj_properties[PROP_MUSICBRAINZ_ARTIST_ID] =
    g_param_spec_string ("musicbrainz-artist-id",
                         "MusicBrainz artist id",
                         "The artist id in the MusicBrainz database",
                         NULL,
                         G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  obj_properties[PROP_MUSICBRAINZ_RELEASE_GROUP_ID] =
    g_param_spec_string ("musicbrainz-release-group-id",
                         "MusicBrainz release group id",
                         "The release group id in the MusicBrainz database",
                         NULL,
                         G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  obj_properties[PROP_MUSICBRAINZ_RELEASE_TRACK_ID] =
    g_param_spec_string ("musicbrainz-release-track-id",
                         "MusicBrainz release track id",
                         "The release track id in the MusicBrainz database",
                         NULL,
                         G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  obj_properties[PROP_MUSICBRAINZ_TRACK_ID] =
    g_param_spec_string ("musicbrainz-track-id",
                         "MusicBrainz track id",
                         "The track id in the MusicBrainz database",
                         NULL,
                         G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  obj_properties[PROP_MUSICBRAINZ_WORK_ID] =
    g_param_spec_string ("musicbrainz-work-id",
                         "MusicBrainz work id",
                         "The work id in the MusicBrainz database",
                         NULL,
                         G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  obj_properties[PROP_ORIGINAL_DATE] =
    g_param_spec_string ("original-date",
                         "Original date",
                         "The song’s original release date",
                         NULL,
                         G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  obj_properties[PROP_PERFORMERS] =
    g_param_spec_string ("performers",
                         "Performers",
                         "The artist who performed the song",
                         NULL,
                         G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  obj_properties[PROP_POSITION] =
    g_param_spec_string ("position",
                         "Position",
                         "The decimal track number within the album",
                         NULL,
                         G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  obj_properties[PROP_PUBLISHER] =
    g_param_spec_string ("publisher",
                         "Publisher",
                         "The name of the label or publisher",
                         NULL,
                         G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  obj_properties[PROP_TITLE] =
    g_param_spec_string ("title",
                         "Title",
                         "Track title",
                         NULL,
                         G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  obj_properties[PROP_URI] =
    g_param_spec_string ("uri",
                         "URI",
                         "Track filepath on MPD server",
                         NULL,
                         G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  obj_properties[PROP_WORK] =
    g_param_spec_string ("work",
                         "Work",
                         "A work is a distinct intellectual or artistic creation, which can be expressed in the form of one or more audio recordings (def. by Musicbrainz)",
                         NULL,
                         G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);

  g_object_class_install_properties (gobject_class,
                                     N_PROPERTIES,
                                     obj_properties);
}

static void
polyhymnia_track_full_info_init (PolyhymniaTrackFullInfo *self)
{
}

/* Instance methods */
const char *
polyhymnia_track_full_info_get_album (PolyhymniaTrackFullInfo *self)
{
  g_return_val_if_fail (POLYHYMNIA_IS_TRACK_FULL_INFO (self), NULL);
  return self->album;
}

const char *
polyhymnia_track_full_info_get_album_artist (PolyhymniaTrackFullInfo *self)
{
  g_return_val_if_fail (POLYHYMNIA_IS_TRACK_FULL_INFO (self), NULL);
  return self->album_artist;
}

const char *
polyhymnia_track_full_info_get_artists (PolyhymniaTrackFullInfo *self)
{
  g_return_val_if_fail (POLYHYMNIA_IS_TRACK_FULL_INFO (self), NULL);
  return self->artists;
}

PolyhymniaAudioFormat *
polyhymnia_track_full_info_get_audio_format (PolyhymniaTrackFullInfo *self)
{
  g_return_val_if_fail (POLYHYMNIA_IS_TRACK_FULL_INFO (self), NULL);
  return self->audio_format;
}

const char *
polyhymnia_track_full_info_get_comment (PolyhymniaTrackFullInfo *self)
{
  g_return_val_if_fail (POLYHYMNIA_IS_TRACK_FULL_INFO (self), NULL);
  return self->comment;
}

const char *
polyhymnia_track_full_info_get_composers (PolyhymniaTrackFullInfo *self)
{
  g_return_val_if_fail (POLYHYMNIA_IS_TRACK_FULL_INFO (self), NULL);
  return self->composers;
}

const char *
polyhymnia_track_full_info_get_conductors (PolyhymniaTrackFullInfo *self)
{
  g_return_val_if_fail (POLYHYMNIA_IS_TRACK_FULL_INFO (self), NULL);
  return self->conductors;
}

const char *
polyhymnia_track_full_info_get_date (PolyhymniaTrackFullInfo *self)
{
  g_return_val_if_fail (POLYHYMNIA_IS_TRACK_FULL_INFO (self), NULL);
  return self->date;
}

const char *
polyhymnia_track_full_info_get_disc (PolyhymniaTrackFullInfo *self)
{
  g_return_val_if_fail (POLYHYMNIA_IS_TRACK_FULL_INFO (self), NULL);
  return self->disc;
}

const char *
polyhymnia_track_full_info_get_ensemble (PolyhymniaTrackFullInfo *self)
{
  g_return_val_if_fail (POLYHYMNIA_IS_TRACK_FULL_INFO (self), NULL);
  return self->ensemble;
}

const char *
polyhymnia_track_full_info_get_genre (PolyhymniaTrackFullInfo *self)
{
  g_return_val_if_fail (POLYHYMNIA_IS_TRACK_FULL_INFO (self), NULL);
  return self->genre;
}

const char *
polyhymnia_track_full_info_get_location (PolyhymniaTrackFullInfo *self)
{
  g_return_val_if_fail (POLYHYMNIA_IS_TRACK_FULL_INFO (self), NULL);
  return self->location;
}

const char *
polyhymnia_track_full_info_get_movement (PolyhymniaTrackFullInfo *self)
{
  g_return_val_if_fail (POLYHYMNIA_IS_TRACK_FULL_INFO (self), NULL);
  return self->movement;
}

const char *
polyhymnia_track_full_info_get_movement_number (PolyhymniaTrackFullInfo *self)
{
  g_return_val_if_fail (POLYHYMNIA_IS_TRACK_FULL_INFO (self), NULL);
  return self->movement_number;
}

const char *
polyhymnia_track_full_info_get_musicbrainz_album_id (PolyhymniaTrackFullInfo *self)
{
  return self->musicbrainz_album_id;
}

const char *
polyhymnia_track_full_info_get_musicbrainz_album_artist_id (PolyhymniaTrackFullInfo *self)
{
  return self->musicbrainz_album_artist_id;
}

const char *
polyhymnia_track_full_info_get_musicbrainz_artist_id (PolyhymniaTrackFullInfo *self)
{
  return self->musicbrainz_artist_id;
}

const char *
polyhymnia_track_full_info_get_musicbrainz_release_group_id (PolyhymniaTrackFullInfo *self)
{
  return self->musicbrainz_release_group_id;
}

const char *
polyhymnia_track_full_info_get_musicbrainz_release_track_id (PolyhymniaTrackFullInfo *self)
{
  return self->musicbrainz_release_track_id;
}

const char *
polyhymnia_track_full_info_get_musicbrainz_track_id (PolyhymniaTrackFullInfo *self)
{
  return self->musicbrainz_track_id;
}

const char *
polyhymnia_track_full_info_get_musicbrainz_work_id (PolyhymniaTrackFullInfo *self)
{
  return self->musicbrainz_work_id;
}

const char *
polyhymnia_track_full_info_get_original_date (PolyhymniaTrackFullInfo *self)
{
  g_return_val_if_fail (POLYHYMNIA_IS_TRACK_FULL_INFO (self), NULL);
  return self->original_date;
}

const char *
polyhymnia_track_full_info_get_performers (PolyhymniaTrackFullInfo *self)
{
  g_return_val_if_fail (POLYHYMNIA_IS_TRACK_FULL_INFO (self), NULL);
  return self->performers;
}

const char *
polyhymnia_track_full_info_get_position (PolyhymniaTrackFullInfo *self)
{
  g_return_val_if_fail (POLYHYMNIA_IS_TRACK_FULL_INFO (self), NULL);
  return self->position;
}

const char *
polyhymnia_track_full_info_get_publisher (PolyhymniaTrackFullInfo *self)
{
  g_return_val_if_fail (POLYHYMNIA_IS_TRACK_FULL_INFO (self), NULL);
  return self->publisher;
}

const char *
polyhymnia_track_full_info_get_title (PolyhymniaTrackFullInfo *self)
{
  g_return_val_if_fail (POLYHYMNIA_IS_TRACK_FULL_INFO (self), NULL);
  return self->title;
}

const char *
polyhymnia_track_full_info_get_uri (PolyhymniaTrackFullInfo *self)
{
  g_return_val_if_fail (POLYHYMNIA_IS_TRACK_FULL_INFO (self), NULL);
  return self->uri;
}

const char *
polyhymnia_track_full_info_get_work (PolyhymniaTrackFullInfo *self)
{
  g_return_val_if_fail (POLYHYMNIA_IS_TRACK_FULL_INFO (self), NULL);
  return self->work;
}
