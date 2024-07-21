
#include "config.h"

#include "polyhymnia-track-details-dialog.h"

#include <webkit/webkit.h>

#include "polyhymnia-lyrics-provider.h"
#include "polyhymnia-mpd-client-details.h"
#include "polyhymnia-mpd-client-images.h"

#define _(x) g_dgettext (GETTEXT_PACKAGE, x)

/* Type metadata */
typedef enum
{
  PROP_TRACK_URI = 1,
  N_PROPERTIES,
} PolyhymniaTrackDetailsDialogProperty;

struct _PolyhymniaTrackDetailsDialog
{
  AdwDialog parent_instance;

  /* Stored UI state */
  GCancellable             *song_details_cancellable;
  GCancellable             *song_lyrics_cancellable;

  /* Template widgets */
  AdwNavigationView        *root_navigation_view;

  AdwStatusPage            *error_status_page;
  AdwClamp                 *main_content;
  GtkScrolledWindow        *main_scrolled_window;
  GtkSpinner               *spinner;

  GtkImage                 *album_cover_image;
  GtkLabel                 *track_title_label;
  GtkLabel                 *album_title_label;
  GtkLabel                 *album_artist_label;

  AdwPreferencesGroup      *details_group;
  AdwActionRow             *details_row;
  AdwActionRow             *lyrics_row;
  AdwActionRow             *audio_format_row;

  GtkBox                   *genre_row;
  GtkLabel                 *genre_label;
  GtkBox                   *disc_row;
  GtkLabel                 *disc_label;
  GtkBox                   *position_row;
  GtkLabel                 *position_label;
  GtkBox                   *date_row;
  GtkLabel                 *date_label;
  GtkBox                   *original_date_row;
  GtkLabel                 *original_date_label;
  GtkBox                   *work_row;
  GtkLabel                 *work_label;
  GtkBox                   *movement_row;
  GtkLabel                 *movement_label;
  GtkBox                   *movement_number_row;
  GtkLabel                 *movement_number_label;
  GtkBox                   *location_row;
  GtkLabel                 *location_label;
  GtkBox                   *comment_row;
  GtkLabel                 *comment_label;

  AdwBin                   *lyrics_page_content;
  GtkSpinner               *lyrics_spinner;
  AdwStatusPage            *lyrics_status_page;
  WebKitWebView            *lyrics_web_view;

  GtkLabel                 *sample_rate_label;
  GtkLabel                 *bps_label;
  GtkLabel                 *channels_label;

  AdwPreferencesGroup      *legal_group;
  AdwActionRow             *personnel_row;
  AdwActionRow             *legal_row;

  GtkBox                   *artists_row;
  GtkLabel                 *artists_label;
  GtkBox                   *performers_row;
  GtkLabel                 *performers_label;
  GtkBox                   *composers_row;
  GtkLabel                 *composers_label;
  GtkBox                   *conductors_row;
  GtkLabel                 *conductors_label;
  GtkBox                   *ensemble_row;
  GtkLabel                 *ensemble_label;

  GtkLabel                 *publisher_label;

  /* Template objects */
  PolyhymniaLyricsProvider *lyrics_provider;
  PolyhymniaMpdClient      *mpd_client;

  /* Instance properties */
  gchar *track_uri;
};

G_DEFINE_FINAL_TYPE (PolyhymniaTrackDetailsDialog, polyhymnia_track_details_dialog, ADW_TYPE_DIALOG)

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

/* Event handlers declaration */
static void
polyhymnia_track_details_dialog_get_song_details_callback (GObject      *source_object,
                                                           GAsyncResult *result,
                                                           gpointer      user_data);

static void
polyhymnia_track_details_dialog_lyrics_web_view_load_changed (PolyhymniaTrackDetailsDialog *self,
                                                              WebKitLoadEvent               load_event,
                                                              WebKitWebView                *user_data);

static void
polyhymnia_track_details_dialog_mpd_client_initialized (PolyhymniaTrackDetailsDialog *self,
                                                        GParamSpec                   *pspec,
                                                        PolyhymniaMpdClient          *user_data);

static void
polyhymnia_track_details_dialog_mpd_database_updated (PolyhymniaTrackDetailsDialog *self,
                                                      PolyhymniaMpdClient          *user_data);

static void
polyhymnia_track_details_dialog_search_song_lyrics_callback (GObject      *source,
                                                             GAsyncResult *result,
                                                             gpointer      user_data);

/* Private methods declaration */
static void
polyhymnia_track_details_dialog_fill_cover (PolyhymniaTrackDetailsDialog *self);

/* Class stuff */
static void
polyhymnia_track_details_dialog_constructed (GObject *gobject)
{
  PolyhymniaTrackDetailsDialog *self = POLYHYMNIA_TRACK_DETAILS_DIALOG (gobject);

  polyhymnia_track_details_dialog_mpd_client_initialized (self, NULL, self->mpd_client);

  G_OBJECT_CLASS (polyhymnia_track_details_dialog_parent_class)->constructed (gobject);
}

static void
polyhymnia_track_details_dialog_dispose(GObject *gobject)
{
  PolyhymniaTrackDetailsDialog *self = POLYHYMNIA_TRACK_DETAILS_DIALOG (gobject);

  g_cancellable_cancel (self->song_details_cancellable);
  g_cancellable_cancel (self->song_lyrics_cancellable);
  g_clear_pointer (&(self->track_uri), g_free);
  gtk_widget_dispose_template (GTK_WIDGET (self), POLYHYMNIA_TYPE_TRACK_DETAILS_DIALOG);

  G_OBJECT_CLASS (polyhymnia_track_details_dialog_parent_class)->dispose (gobject);
}

static void
polyhymnia_track_details_dialog_get_property (GObject    *object,
                                              guint       property_id,
                                              GValue     *value,
                                              GParamSpec *pspec)
{
  PolyhymniaTrackDetailsDialog *self = POLYHYMNIA_TRACK_DETAILS_DIALOG (object);

  switch ((PolyhymniaTrackDetailsDialogProperty) property_id)
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
polyhymnia_track_details_dialog_set_property (GObject      *object,
                                              guint         property_id,
                                              const GValue *value,
                                              GParamSpec   *pspec)
{
  PolyhymniaTrackDetailsDialog *self = POLYHYMNIA_TRACK_DETAILS_DIALOG (object);

  switch ((PolyhymniaTrackDetailsDialogProperty) property_id)
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
polyhymnia_track_details_dialog_class_init (PolyhymniaTrackDetailsDialogClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->constructed = polyhymnia_track_details_dialog_constructed;
  gobject_class->dispose = polyhymnia_track_details_dialog_dispose;
  gobject_class->get_property = polyhymnia_track_details_dialog_get_property;
  gobject_class->set_property = polyhymnia_track_details_dialog_set_property;

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
                                               "/com/github/pamugk/polyhymnia/ui/polyhymnia-track-details-dialog.ui");

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsDialog, root_navigation_view);

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsDialog, error_status_page);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsDialog, main_content);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsDialog, main_scrolled_window);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsDialog, spinner);

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsDialog, album_cover_image);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsDialog, track_title_label);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsDialog, album_title_label);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsDialog, album_artist_label);

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsDialog, details_group);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsDialog, details_row);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsDialog, lyrics_row);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsDialog, audio_format_row);

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsDialog, genre_row);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsDialog, genre_label);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsDialog, disc_row);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsDialog, disc_label);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsDialog, position_row);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsDialog, position_label);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsDialog, date_row);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsDialog, date_label);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsDialog, original_date_row);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsDialog, original_date_label);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsDialog, work_row);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsDialog, work_label);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsDialog, movement_row);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsDialog, movement_label);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsDialog, movement_number_row);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsDialog, movement_number_label);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsDialog, location_row);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsDialog, location_label);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsDialog, comment_row);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsDialog, comment_label);

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsDialog, lyrics_page_content);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsDialog, lyrics_spinner);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsDialog, lyrics_status_page);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsDialog, lyrics_web_view);

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsDialog, sample_rate_label);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsDialog, bps_label);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsDialog, channels_label);

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsDialog, legal_group);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsDialog, personnel_row);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsDialog, legal_row);

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsDialog, artists_row);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsDialog, artists_label);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsDialog, performers_row);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsDialog, performers_label);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsDialog, composers_row);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsDialog, composers_label);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsDialog, conductors_row);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsDialog, conductors_label);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsDialog, ensemble_row);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsDialog, ensemble_label);

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsDialog, publisher_label);

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsDialog, lyrics_provider);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsDialog, mpd_client);

  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_track_details_dialog_lyrics_web_view_load_changed);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_track_details_dialog_mpd_client_initialized);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_track_details_dialog_mpd_database_updated);
}

static void
polyhymnia_track_details_dialog_init (PolyhymniaTrackDetailsDialog *self)
{
  g_type_ensure (WEBKIT_TYPE_WEB_VIEW);

  g_type_ensure (POLYHYMNIA_TYPE_LYRICS_PROVIDER);
  g_type_ensure (POLYHYMNIA_TYPE_MPD_CLIENT);

  gtk_widget_init_template (GTK_WIDGET (self));
}

/* Event handlers implementation */
static void
polyhymnia_track_details_dialog_get_song_details_callback (GObject      *source_object,
                                                           GAsyncResult *result,
                                                           gpointer      user_data)
{
  PolyhymniaTrackFullInfo *details;
  GError *error = NULL;
  PolyhymniaMpdClient *mpd_client = POLYHYMNIA_MPD_CLIENT (source_object);
  PolyhymniaTrackDetailsDialog *self = user_data;

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

    {
      PolyhymniaSearchLyricsRequest *search_lyrics_request;

      search_lyrics_request = g_malloc (sizeof (PolyhymniaSearchLyricsRequest));
      search_lyrics_request->artist = g_strdup (polyhymnia_track_full_info_get_artists (details));
      search_lyrics_request->title = g_strdup (polyhymnia_track_full_info_get_title (details));

      self->song_lyrics_cancellable = g_cancellable_new ();
      adw_bin_set_child (self->lyrics_page_content, GTK_WIDGET (self->lyrics_spinner));
      gtk_spinner_start (self->lyrics_spinner);
      polyhymnia_lyrics_provider_search_lyrics_async (self->lyrics_provider,
                                                      search_lyrics_request,
                                                      self->song_lyrics_cancellable,
                                                      polyhymnia_track_details_dialog_search_song_lyrics_callback,
                                                      self);
    }

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
  else if (!g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
  {
    gtk_scrolled_window_set_child (self->main_scrolled_window,
                                   GTK_WIDGET (self->error_status_page));
    g_warning ("Failed to get track details: %s\n", error->message);
    g_error_free (error);
    error = NULL;
  }
  else
  {
    g_clear_object (&(self->song_details_cancellable));
    return;
  }

  gtk_spinner_stop (self->spinner);
  g_clear_object (&(self->song_details_cancellable));
}

static void
polyhymnia_track_details_dialog_lyrics_web_view_load_changed (PolyhymniaTrackDetailsDialog *self,
                                                              WebKitLoadEvent               load_event,
                                                              WebKitWebView                *user_data)
{
  g_assert (POLYHYMNIA_IS_TRACK_DETAILS_DIALOG (self));

  if (load_event == WEBKIT_LOAD_FINISHED)
  {
    adw_bin_set_child (self->lyrics_page_content,
                       GTK_WIDGET (self->lyrics_web_view));
  }
}

static void
polyhymnia_track_details_dialog_mpd_client_initialized (PolyhymniaTrackDetailsDialog *self,
                                                        GParamSpec                   *pspec,
                                                        PolyhymniaMpdClient          *user_data)
{
  g_assert (POLYHYMNIA_IS_TRACK_DETAILS_DIALOG (self));

  if (polyhymnia_mpd_client_is_initialized (user_data))
  {
    polyhymnia_track_details_dialog_mpd_database_updated (self, user_data);
  }
  else
  {
    adw_dialog_close (ADW_DIALOG (self));
  }
}

static void
polyhymnia_track_details_dialog_mpd_database_updated (PolyhymniaTrackDetailsDialog *self,
                                                      PolyhymniaMpdClient          *user_data)
{
  g_assert (POLYHYMNIA_IS_TRACK_DETAILS_DIALOG (self));

  polyhymnia_track_details_dialog_fill_cover (self);

  if (self->song_details_cancellable == NULL)
  {
    webkit_web_view_stop_loading (self->lyrics_web_view);
    g_cancellable_cancel (self->song_lyrics_cancellable);

    self->song_details_cancellable = g_cancellable_new ();
    polyhymnia_mpd_client_get_song_details_async (self->mpd_client,
                                                  self->track_uri,
                                                  self->song_details_cancellable,
                                                  polyhymnia_track_details_dialog_get_song_details_callback,
                                                  self);
    gtk_spinner_start (self->spinner);
    gtk_scrolled_window_set_child (self->main_scrolled_window,
                                   GTK_WIDGET (self->spinner));
  }
}

static void
polyhymnia_track_details_dialog_search_song_lyrics_callback (GObject      *source,
                                                             GAsyncResult *result,
                                                             gpointer      user_data)
{
  GError                       *error = NULL;
  char                         *lyrics;
  PolyhymniaTrackDetailsDialog *self = POLYHYMNIA_TRACK_DETAILS_DIALOG (user_data);

  lyrics = polyhymnia_lyrics_provider_search_lyrics_finish (POLYHYMNIA_LYRICS_PROVIDER (source),
                                                            result,
                                                            &error);
  if (error != NULL)
  {
      g_object_set (G_OBJECT (self->lyrics_status_page),
                    "description", _("Failed to find song lyrics"),
                    NULL);
      adw_bin_set_child (self->lyrics_page_content,
                         GTK_WIDGET (self->lyrics_status_page));
  }
  else if (lyrics == NULL)
  {
      g_object_set (G_OBJECT (self->lyrics_status_page),
                    "description", _("No song lyrics found"),
                    NULL);
      adw_bin_set_child (self->lyrics_page_content,
                         GTK_WIDGET (self->lyrics_status_page));
  }
  else
  {
    GString *lyrics_string = g_string_new_take (lyrics);
    // This is needed to bypass problems with script download
    g_string_replace (lyrics_string, "src='//", "src='https://", 0);
    // This is needed to follow dark theme (if enabled)
    g_string_prepend (lyrics_string, "<style>:root{color-scheme:light dark;} .rg_embed.music{background-color:transparent;}{background-color: transparent;}</style>");
    webkit_web_view_load_html (self->lyrics_web_view, lyrics_string->str, NULL);

    g_string_free (lyrics_string, TRUE);
  }

  gtk_spinner_stop (self->spinner);
  g_clear_object (&(self->song_lyrics_cancellable));
}

/* Private methods implementation */
static void
polyhymnia_track_details_dialog_fill_cover (PolyhymniaTrackDetailsDialog *self)
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
