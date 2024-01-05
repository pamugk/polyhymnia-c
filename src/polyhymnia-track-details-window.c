
#include "config.h"

#include "polyhymnia-track-details-window.h"

#include "polyhymnia-mpd-client-details.h"
#include "polyhymnia-mpd-client-images.h"

#define _(x) g_dgettext (GETTEXT_PACKAGE, x)

/* Type metadata */
typedef enum
{
  PROP_TRACK_URI = 1,
  N_PROPERTIES,
} PolyhymniaTrackDetailsWindowProperty;

struct _PolyhymniaTrackDetailsWindow
{
  AdwWindow parent_instance;

  /* Stored UI state */
  GCancellable        *song_details_cancellable;

  /* Template widgets */
  AdwNavigationView   *root_navigation_view;

  AdwStatusPage       *error_status_page;
  AdwClamp            *main_content;
  GtkScrolledWindow   *main_scrolled_window;
  GtkSpinner          *spinner;

  GtkImage            *album_cover_image;
  GtkLabel            *track_title_label;
  GtkLabel            *album_title_label;
  GtkLabel            *album_artist_label;

  AdwPreferencesGroup *details_group;
  AdwActionRow        *details_row;
  AdwActionRow        *audio_format_row;

  GtkBox              *genre_row;
  GtkLabel            *genre_label;
  GtkBox              *disc_row;
  GtkLabel            *disc_label;
  GtkBox              *position_row;
  GtkLabel            *position_label;
  GtkBox              *date_row;
  GtkLabel            *date_label;
  GtkBox              *original_date_row;
  GtkLabel            *original_date_label;
  GtkBox              *work_row;
  GtkLabel            *work_label;
  GtkBox              *movement_row;
  GtkLabel            *movement_label;
  GtkBox              *movement_number_row;
  GtkLabel            *movement_number_label;
  GtkBox              *location_row;
  GtkLabel            *location_label;
  GtkBox              *comment_row;
  GtkLabel            *comment_label;

  GtkLabel            *sample_rate_label;
  GtkLabel            *bps_label;
  GtkLabel            *channels_label;

  AdwPreferencesGroup *legal_group;
  AdwActionRow        *personnel_row;
  AdwActionRow        *legal_row;

  GtkBox              *artists_row;
  GtkLabel            *artists_label;
  GtkBox              *performers_row;
  GtkLabel            *performers_label;
  GtkBox              *composers_row;
  GtkLabel            *composers_label;
  GtkBox              *conductors_row;
  GtkLabel            *conductors_label;
  GtkBox              *ensemble_row;
  GtkLabel            *ensemble_label;

  GtkLabel            *publisher_label;

  /* Template objects */
  PolyhymniaMpdClient *mpd_client;

  /* Instance properties */
  gchar *track_uri;
};

G_DEFINE_FINAL_TYPE (PolyhymniaTrackDetailsWindow, polyhymnia_track_details_window, ADW_TYPE_WINDOW)

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

/* Event handlers declaration */
static void
polyhymnia_track_details_window_get_song_details_callback (GObject *source_object,
                                                           GAsyncResult *result,
                                                           gpointer user_data);

static void
polyhymnia_track_details_window_mpd_client_initialized (PolyhymniaTrackDetailsWindow *self,
                                                        GParamSpec                   *pspec,
                                                        PolyhymniaMpdClient          *user_data);

static void
polyhymnia_track_details_window_mpd_database_updated (PolyhymniaTrackDetailsWindow *self,
                                                      PolyhymniaMpdClient          *user_data);

/* Private methods declaration */
static void
polyhymnia_track_details_window_fill_cover (PolyhymniaTrackDetailsWindow *self);

/* Class stuff */
static void
polyhymnia_track_details_window_constructed (GObject *gobject)
{
  PolyhymniaTrackDetailsWindow *self = POLYHYMNIA_TRACK_DETAILS_WINDOW (gobject);

  polyhymnia_track_details_window_mpd_client_initialized (self, NULL, self->mpd_client);

  G_OBJECT_CLASS (polyhymnia_track_details_window_parent_class)->constructed (gobject);
}

static void
polyhymnia_track_details_window_dispose(GObject *gobject)
{
  PolyhymniaTrackDetailsWindow *self = POLYHYMNIA_TRACK_DETAILS_WINDOW (gobject);

  g_cancellable_cancel (self->song_details_cancellable);
  g_clear_pointer (&(self->track_uri), g_free);
  gtk_widget_dispose_template (GTK_WIDGET (self), POLYHYMNIA_TYPE_TRACK_DETAILS_WINDOW);

  G_OBJECT_CLASS (polyhymnia_track_details_window_parent_class)->dispose (gobject);
}

static void
polyhymnia_track_details_window_get_property (GObject    *object,
                                              guint       property_id,
                                              GValue     *value,
                                              GParamSpec *pspec)
{
  PolyhymniaTrackDetailsWindow *self = POLYHYMNIA_TRACK_DETAILS_WINDOW (object);

  switch ((PolyhymniaTrackDetailsWindowProperty) property_id)
    {
    case PROP_TRACK_URI:
      g_value_set_string (value, self->track_uri);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
polyhymnia_track_details_window_set_property (GObject      *object,
                                              guint         property_id,
                                              const GValue *value,
                                              GParamSpec   *pspec)
{
  PolyhymniaTrackDetailsWindow *self = POLYHYMNIA_TRACK_DETAILS_WINDOW (object);

  switch ((PolyhymniaTrackDetailsWindowProperty) property_id)
    {
    case PROP_TRACK_URI:
      g_set_str (&(self->track_uri), g_value_get_string (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
polyhymnia_track_details_window_class_init (PolyhymniaTrackDetailsWindowClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->constructed = polyhymnia_track_details_window_constructed;
  gobject_class->dispose = polyhymnia_track_details_window_dispose;
  gobject_class->get_property = polyhymnia_track_details_window_get_property;
  gobject_class->set_property = polyhymnia_track_details_window_set_property;

  obj_properties[PROP_TRACK_URI] =
    g_param_spec_string ("track-uri",
                         "Track URI",
                         "URI of a track being displayed.",
                         NULL,
                         G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_NAME);

  g_object_class_install_properties (gobject_class,
                                     N_PROPERTIES,
                                     obj_properties);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/com/github/pamugk/polyhymnia/ui/polyhymnia-track-details-window.ui");

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsWindow, root_navigation_view);

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsWindow, error_status_page);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsWindow, main_content);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsWindow, main_scrolled_window);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsWindow, spinner);

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsWindow, album_cover_image);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsWindow, track_title_label);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsWindow, album_title_label);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsWindow, album_artist_label);

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsWindow, details_group);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsWindow, details_row);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsWindow, audio_format_row);

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsWindow, genre_row);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsWindow, genre_label);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsWindow, disc_row);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsWindow, disc_label);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsWindow, position_row);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsWindow, position_label);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsWindow, date_row);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsWindow, date_label);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsWindow, original_date_row);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsWindow, original_date_label);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsWindow, work_row);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsWindow, work_label);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsWindow, movement_row);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsWindow, movement_label);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsWindow, movement_number_row);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsWindow, movement_number_label);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsWindow, location_row);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsWindow, location_label);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsWindow, comment_row);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsWindow, comment_label);

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsWindow, sample_rate_label);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsWindow, bps_label);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsWindow, channels_label);

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsWindow, legal_group);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsWindow, personnel_row);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsWindow, legal_row);

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsWindow, artists_row);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsWindow, artists_label);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsWindow, performers_row);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsWindow, performers_label);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsWindow, composers_row);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsWindow, composers_label);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsWindow, conductors_row);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsWindow, conductors_label);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsWindow, ensemble_row);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsWindow, ensemble_label);

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsWindow, publisher_label);

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsWindow, mpd_client);

  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_track_details_window_mpd_client_initialized);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_track_details_window_mpd_database_updated);
}

static void
polyhymnia_track_details_window_init (PolyhymniaTrackDetailsWindow *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}

/* Event handlers implementation */
static void
polyhymnia_track_details_window_get_song_details_callback (GObject *source_object,
                                                           GAsyncResult *result,
                                                           gpointer user_data)
{
  PolyhymniaTrackFullInfo *details;
  GError *error = NULL;
  PolyhymniaMpdClient *mpd_client = POLYHYMNIA_MPD_CLIENT (source_object);
  PolyhymniaTrackDetailsWindow *self = user_data;

  details = polyhymnia_mpd_client_get_song_details_finish (mpd_client,
                                                           result,
                                                           &error);

  if (error == NULL)
  {
    const char *title = NULL;
    const char *album = NULL;
    const char *artist = NULL;

    const char *genre = NULL;
    const char *disc = NULL;
    const char *position = NULL;
    const char *date = NULL;
    const char *original_date = NULL;
    const char *work = NULL;
    const char *movement = NULL;
    const char *movement_number = NULL;
    const char *location = NULL;
    const char *comment = NULL;

    PolyhymniaAudioFormat *audio_format = NULL;

    const char *artists = NULL;
    const char *performers = NULL;
    const char *composers = NULL;
    const char *conductors = NULL;
    const char *ensemble = NULL;

    const char *publisher = NULL;

    if (details != NULL)
    {
      title = polyhymnia_track_full_info_get_title (details);
      album = polyhymnia_track_full_info_get_album (details);
      artist = polyhymnia_track_full_info_get_album_artist (details);

      genre = polyhymnia_track_full_info_get_genre (details);
      disc = polyhymnia_track_full_info_get_disc (details);
      position = polyhymnia_track_full_info_get_position (details);
      date = polyhymnia_track_full_info_get_date (details);
      original_date = polyhymnia_track_full_info_get_original_date (details);
      work = polyhymnia_track_full_info_get_work (details);
      movement = polyhymnia_track_full_info_get_movement (details);
      movement_number = polyhymnia_track_full_info_get_movement_number (details);
      location = polyhymnia_track_full_info_get_location (details);
      comment = polyhymnia_track_full_info_get_comment (details);

      audio_format = polyhymnia_track_full_info_get_audio_format (details);

      artists = polyhymnia_track_full_info_get_artists (details);
      performers = polyhymnia_track_full_info_get_performers (details);
      composers = polyhymnia_track_full_info_get_composers (details);
      conductors = polyhymnia_track_full_info_get_conductors (details);
      ensemble = polyhymnia_track_full_info_get_ensemble (details);

      publisher = polyhymnia_track_full_info_get_publisher (details);
    }

    gtk_label_set_label (self->track_title_label,
                         title == NULL ? _("Unknown track") : title);
    gtk_label_set_label (self->album_title_label,
                         album == NULL ? _("Unknown album") : album);
    gtk_label_set_label (self->album_artist_label,
                         artist == NULL
                         ? artists == NULL ? _("Unknown artist") : artists
                         : artist);

    gtk_label_set_text (self->genre_label, genre);
    gtk_widget_set_visible (GTK_WIDGET (self->genre_row), genre != NULL);
    gtk_label_set_text (self->disc_label, disc);
    gtk_widget_set_visible (GTK_WIDGET (self->disc_row), disc != NULL);
    gtk_label_set_text (self->position_label, position);
    gtk_widget_set_visible (GTK_WIDGET (self->position_row), position != NULL);
    gtk_label_set_text (self->date_label, date);
    gtk_widget_set_visible (GTK_WIDGET (self->date_row), date != NULL);
    gtk_label_set_text (self->original_date_label, original_date);
    gtk_widget_set_visible (GTK_WIDGET (self->original_date_row), original_date != NULL);
    gtk_label_set_text (self->work_label, work);
    gtk_widget_set_visible (GTK_WIDGET (self->work_row), work != NULL);
    gtk_label_set_text (self->movement_label, movement);
    gtk_widget_set_visible (GTK_WIDGET (self->movement_row), movement != NULL);
    gtk_label_set_text (self->movement_number_label, movement_number);
    gtk_widget_set_visible (GTK_WIDGET (self->movement_number_row), movement_number != NULL);
    gtk_label_set_text (self->location_label, location);
    gtk_widget_set_visible (GTK_WIDGET (self->location_row), location != NULL);
    gtk_label_set_text (self->comment_label, comment);
    gtk_widget_set_visible (GTK_WIDGET (self->comment_row), comment != NULL);

    gtk_widget_set_visible (GTK_WIDGET (self->details_row),
                            gtk_widget_get_visible (GTK_WIDGET (self->genre_row))
                            || gtk_widget_get_visible (GTK_WIDGET (self->disc_row))
                            || gtk_widget_get_visible (GTK_WIDGET (self->position_row))
                            || gtk_widget_get_visible (GTK_WIDGET (self->date_row))
                            || gtk_widget_get_visible (GTK_WIDGET (self->original_date_row))
                            || gtk_widget_get_visible (GTK_WIDGET (self->work_row))
                            || gtk_widget_get_visible (GTK_WIDGET (self->movement_row))
                            || gtk_widget_get_visible (GTK_WIDGET (self->movement_number_row))
                            || gtk_widget_get_visible (GTK_WIDGET (self->location_row))
                            || gtk_widget_get_visible (GTK_WIDGET (self->comment_row)));

    if (audio_format == NULL)
    {
      gtk_label_set_text (self->sample_rate_label, NULL);
      gtk_label_set_text (self->bps_label, NULL);
      gtk_label_set_text (self->channels_label, NULL);
      gtk_widget_set_visible (GTK_WIDGET (self->audio_format_row), FALSE);
    }
    else
    {
      if (polyhymnia_audio_format_get_sample_rate (audio_format) == 0)
      {
        gtk_label_set_text (self->sample_rate_label, _("Unknown or unspecified"));
      }
      else
      {
        char *sample_rate = g_strdup_printf (_("%d Hz"),
                                             polyhymnia_audio_format_get_sample_rate (audio_format));
        gtk_label_set_text (self->sample_rate_label, sample_rate);
        g_free (sample_rate);
      }
      if (polyhymnia_audio_format_get_bits (audio_format) == 0)
      {
        gtk_label_set_text (self->bps_label, _("Unknown or unspecified"));
      }
      else
      {
        char *bits = g_strdup_printf (_("%d bits per sample"),
                                      polyhymnia_audio_format_get_bits (audio_format));
        gtk_label_set_text (self->bps_label, bits);
        g_free (bits);
      }
      if (polyhymnia_audio_format_get_channels (audio_format) == 0)
      {
        gtk_label_set_text (self->channels_label, _("Unknown or unspecified"));
      }
      else
      {
        char *channels = g_strdup_printf ("%d",
                                          polyhymnia_audio_format_get_channels (audio_format));
        gtk_label_set_text (self->channels_label, channels);
        g_free (channels);
      }

      gtk_widget_set_visible (GTK_WIDGET (self->audio_format_row), TRUE);
    }

    gtk_widget_set_visible (GTK_WIDGET (self->details_group),
                            gtk_widget_get_visible (GTK_WIDGET (self->details_row))
                            || gtk_widget_get_visible (GTK_WIDGET (self->audio_format_row)));

    gtk_label_set_label (self->artists_label, artists);
    gtk_widget_set_visible (GTK_WIDGET (self->artists_row), artists != NULL);
    gtk_label_set_label (self->performers_label, performers);
    gtk_widget_set_visible (GTK_WIDGET (self->performers_row), performers != NULL);
    gtk_label_set_label (self->composers_label, composers);
    gtk_widget_set_visible (GTK_WIDGET (self->composers_row), composers != NULL);
    gtk_label_set_label (self->conductors_label, conductors);
    gtk_widget_set_visible (GTK_WIDGET (self->conductors_row), conductors != NULL);
    gtk_label_set_label (self->ensemble_label, ensemble);
    gtk_widget_set_visible (GTK_WIDGET (self->ensemble_row), ensemble != NULL);

    gtk_widget_set_visible (GTK_WIDGET (self->personnel_row),
                            gtk_widget_get_visible (GTK_WIDGET (self->artists_row))
                            || gtk_widget_get_visible (GTK_WIDGET (self->performers_row))
                            || gtk_widget_get_visible (GTK_WIDGET (self->composers_row))
                            || gtk_widget_get_visible (GTK_WIDGET (self->conductors_row))
                            || gtk_widget_get_visible (GTK_WIDGET (self->ensemble_row)));

    gtk_label_set_label (self->publisher_label, publisher);
    gtk_widget_set_visible (GTK_WIDGET (self->legal_row), publisher != NULL);

    gtk_widget_set_visible (GTK_WIDGET (self->legal_group),
                            gtk_widget_get_visible (GTK_WIDGET (self->personnel_row))
                            || gtk_widget_get_visible (GTK_WIDGET (self->legal_row)));

    gtk_scrolled_window_set_child (self->main_scrolled_window,
                                   GTK_WIDGET (self->main_content));
    if (details != NULL)
    {
      g_object_unref (details);
    }
  }
  else if(!g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
  {
    gtk_scrolled_window_set_child (self->main_scrolled_window,
                                   GTK_WIDGET (self->error_status_page));
    g_warning ("Failed to get track details: %s\n", error->message);
    g_error_free (error);
    error = NULL;
  }

  g_clear_object (&(self->song_details_cancellable));
}

static void
polyhymnia_track_details_window_mpd_client_initialized (PolyhymniaTrackDetailsWindow *self,
                                                        GParamSpec                   *pspec,
                                                        PolyhymniaMpdClient          *user_data)
{
  g_assert (POLYHYMNIA_IS_TRACK_DETAILS_WINDOW (self));

  if (polyhymnia_mpd_client_is_initialized (user_data))
  {
    polyhymnia_track_details_window_mpd_database_updated (self, user_data);
  }
  else
  {
    gtk_window_close (GTK_WINDOW (self));
  }
}

static void
polyhymnia_track_details_window_mpd_database_updated (PolyhymniaTrackDetailsWindow *self,
                                                      PolyhymniaMpdClient          *user_data)
{
  g_assert (POLYHYMNIA_IS_TRACK_DETAILS_WINDOW (self));

  polyhymnia_track_details_window_fill_cover (self);

  if (self->song_details_cancellable == NULL)
  {
    self->song_details_cancellable = g_cancellable_new ();
    polyhymnia_mpd_client_get_song_details_async (self->mpd_client,
                                                  self->track_uri,
                                                  self->song_details_cancellable,
                                                  polyhymnia_track_details_window_get_song_details_callback,
                                                  self);
    gtk_spinner_start (self->spinner);
    gtk_scrolled_window_set_child (self->main_scrolled_window,
                                   GTK_WIDGET (self->spinner));
  }
}

/* Private methods implementation */
static void
polyhymnia_track_details_window_fill_cover (PolyhymniaTrackDetailsWindow *self)
{
  GError *error = NULL;
  GBytes *cover;
  cover = polyhymnia_mpd_client_get_song_album_cover (self->mpd_client,
                                                      self->track_uri,
                                                      &error);

  if (error != NULL)
  {
    g_warning ("Failed to get album cover: %s\n", error->message);
    g_error_free (error);
    error = NULL;
    gtk_image_set_from_icon_name (self->album_cover_image,
                                  "image-missing-symbolic");
  }
  else if (cover != NULL)
  {
    GdkTexture *cover_texture = gdk_texture_new_from_bytes (cover, &error);
    if (error != NULL)
    {
      g_warning ("Failed to convert album cover: %s\n", error->message);
      g_error_free (error);
      error = NULL;
      gtk_image_set_from_icon_name (self->album_cover_image,
                                    "image-missing-symbolic");
    }
    else
    {
      gtk_image_set_from_paintable (self->album_cover_image,
                                    GDK_PAINTABLE (cover_texture));
      g_object_unref (cover_texture);
    }
    g_bytes_unref (cover);
  }
  else
  {
    gtk_image_set_from_icon_name (self->album_cover_image,
                                  "image-missing-symbolic");
  }
}
