
#include "app-features.h"
#include "config.h"

#include "polyhymnia-track-details-dialog.h"

#include "polyhymnia-mpd-client-details.h"
#include "polyhymnia-mpd-client-images.h"

#ifdef POLYHYMNIA_FEATURE_EXTERNAL_DATA

#include "polyhymnia-additional-info-provider.h"

#ifdef POLYHYMNIA_FEATURE_LYRICS
#include <webkit/webkit.h>
#include "polyhymnia-lyrics-provider.h"
#endif

#endif

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
  GCancellable                      *song_details_cancellable;

  /* Template widgets */
  AdwNavigationView                 *root_navigation_view;

  AdwStatusPage                     *error_status_page;
  AdwClamp                          *main_content;
  GtkScrolledWindow                 *main_scrolled_window;
  GtkSpinner                        *spinner;

  GtkImage                          *album_cover_image;
  GtkLabel                          *track_title_label;
  GtkLabel                          *album_title_label;
  GtkLabel                          *album_artist_label;

  AdwPreferencesGroup               *details_group;
  AdwActionRow                      *details_row;
  AdwActionRow                      *audio_format_row;

  GtkBox                            *genre_row;
  GtkLabel                          *genre_label;
  GtkBox                            *disc_row;
  GtkLabel                          *disc_label;
  GtkBox                            *position_row;
  GtkLabel                          *position_label;
  GtkBox                            *date_row;
  GtkLabel                          *date_label;
  GtkBox                            *original_date_row;
  GtkLabel                          *original_date_label;
  GtkBox                            *work_row;
  GtkLabel                          *work_label;
  GtkBox                            *movement_row;
  GtkLabel                          *movement_label;
  GtkBox                            *movement_number_row;
  GtkLabel                          *movement_number_label;
  GtkBox                            *location_row;
  GtkLabel                          *location_label;
  GtkBox                            *comment_row;
  GtkLabel                          *comment_label;

  GtkLabel                          *sample_rate_label;
  GtkLabel                          *bps_label;
  GtkLabel                          *channels_label;

  AdwPreferencesGroup               *legal_group;
  AdwActionRow                      *personnel_row;
  AdwActionRow                      *legal_row;

  GtkBox                            *artists_row;
  GtkLabel                          *artists_label;
  GtkBox                            *performers_row;
  GtkLabel                          *performers_label;
  GtkBox                            *composers_row;
  GtkLabel                          *composers_label;
  GtkBox                            *conductors_row;
  GtkLabel                          *conductors_label;
  GtkBox                            *ensemble_row;
  GtkLabel                          *ensemble_label;

  GtkLabel                          *publisher_label;

  /* Template objects */
  PolyhymniaMpdClient               *mpd_client;

  /* Instance properties */
  gchar *track_uri;

  /* Feature-specific fields */
#ifdef POLYHYMNIA_FEATURE_EXTERNAL_DATA
  AdwActionRow                     *additional_info_row;

  AdwToolbarView                   *additional_info_page_content;
  GtkSpinner                       *additional_info_spinner;
  AdwStatusPage                    *additional_info_status_page;
  GtkLabel                         *additional_info_label;

  PolyhymniaAdditionalInfoProvider *additional_info_provider;
  GCancellable                     *additional_info_cancellable;

#ifdef POLYHYMNIA_FEATURE_LYRICS
  AdwActionRow                     *lyrics_row;

  AdwToolbarView                   *lyrics_page_content;
  GtkSpinner                       *lyrics_spinner;
  AdwStatusPage                    *lyrics_status_page;
  WebKitWebView                    *lyrics_web_view;

  PolyhymniaLyricsProvider         *lyrics_provider;
  GCancellable                     *song_lyrics_cancellable;
  GtkUriLauncher                   *uri_launcher;
  GCancellable                     *uri_launcher_cancellable;
#endif

#endif
};

G_DEFINE_FINAL_TYPE (PolyhymniaTrackDetailsDialog, polyhymnia_track_details_dialog, ADW_TYPE_DIALOG)

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

/* Event handlers declaration */
static void
polyhymnia_track_details_dialog_get_song_details_callback (GObject      *source_object,
                                                           GAsyncResult *result,
                                                           gpointer      user_data);

static void
polyhymnia_track_details_dialog_mpd_client_initialized (PolyhymniaTrackDetailsDialog *self,
                                                        GParamSpec                   *pspec,
                                                        PolyhymniaMpdClient          *user_data);

static void
polyhymnia_track_details_dialog_mpd_database_updated (PolyhymniaTrackDetailsDialog *self,
                                                      PolyhymniaMpdClient          *user_data);

#ifdef POLYHYMNIA_FEATURE_EXTERNAL_DATA

static void
polyhymnia_track_details_dialog_search_additional_info_callback (GObject      *source,
                                                                 GAsyncResult *result,
                                                                 gpointer      user_data);

#ifdef POLYHYMNIA_FEATURE_LYRICS
static gboolean
polyhymnia_track_details_dialog_lyrics_web_view_decide_policy (PolyhymniaTrackDetailsDialog *self,
                                                               WebKitPolicyDecision         *decision,
                                                               WebKitPolicyDecisionType      decision_type,
                                                               WebKitWebView                *user_data);

static void
polyhymnia_track_details_dialog_lyrics_web_view_load_changed (PolyhymniaTrackDetailsDialog *self,
                                                              WebKitLoadEvent               load_event,
                                                              WebKitWebView                *user_data);

static void
polyhymnia_track_details_dialog_search_song_lyrics_callback (GObject      *source,
                                                             GAsyncResult *result,
                                                             gpointer      user_data);

static void
polyhymnia_track_details_dialog_show_uri_callback (GObject      *source_object,
                                                   GAsyncResult *result,
                                                   gpointer      user_data);
#endif

#endif

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

#ifdef POLYHYMNIA_FEATURE_EXTERNAL_DATA

  g_cancellable_cancel (self->additional_info_cancellable);
  g_clear_object (&(self->additional_info_cancellable));

  g_clear_object (&(self->additional_info_status_page));
  g_clear_object (&(self->additional_info_spinner));
  g_clear_object (&(self->additional_info_label));
  g_clear_object (&(self->additional_info_provider));

#ifdef POLYHYMNIA_FEATURE_LYRICS
  g_cancellable_cancel (self->song_lyrics_cancellable);
  g_clear_object (&(self->song_lyrics_cancellable));
  g_cancellable_cancel (self->uri_launcher_cancellable);
  g_clear_object (&(self->uri_launcher_cancellable));

  g_clear_object (&(self->lyrics_status_page));
  g_clear_object (&(self->lyrics_spinner));
  g_clear_object (&(self->lyrics_web_view));
  g_clear_object (&(self->lyrics_provider));
  g_clear_object (&(self->uri_launcher));
#endif

#endif
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

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsDialog, mpd_client);

  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_track_details_dialog_mpd_client_initialized);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_track_details_dialog_mpd_database_updated);
}

static void
polyhymnia_track_details_dialog_init (PolyhymniaTrackDetailsDialog *self)
{
  g_type_ensure (POLYHYMNIA_TYPE_MPD_CLIENT);

  gtk_widget_init_template (GTK_WIDGET (self));

#ifdef POLYHYMNIA_FEATURE_EXTERNAL_DATA
  self->additional_info_page_content = ADW_TOOLBAR_VIEW (adw_toolbar_view_new ());
  adw_toolbar_view_add_top_bar (self->additional_info_page_content,
                                adw_header_bar_new ());
  adw_navigation_view_add (self->root_navigation_view,
                           adw_navigation_page_new_with_tag (GTK_WIDGET (self->additional_info_page_content),
                                                             _("Additional Info"),
                                                             "additional-info-page"));

  self->additional_info_row = ADW_ACTION_ROW (g_object_ref_sink (adw_action_row_new ()));
  gtk_list_box_row_set_activatable (GTK_LIST_BOX_ROW (self->additional_info_row), TRUE);
  adw_action_row_add_suffix (self->additional_info_row, gtk_image_new_from_icon_name ("right-symbolic"));
  gtk_actionable_set_detailed_action_name (GTK_ACTIONABLE (self->additional_info_row), "navigation.push::additional-info-page");
  adw_preferences_row_set_title (ADW_PREFERENCES_ROW (self->additional_info_row), _("Additional Info"));
  adw_preferences_group_add (self->details_group, GTK_WIDGET (self->additional_info_row));
  g_object_unref (self->additional_info_row);

  self->additional_info_status_page = ADW_STATUS_PAGE (g_object_ref_sink (adw_status_page_new ()));
  self->additional_info_spinner = GTK_SPINNER (g_object_ref_sink (gtk_spinner_new ()));
  gtk_widget_set_halign (GTK_WIDGET (self->additional_info_spinner), GTK_ALIGN_CENTER);
  gtk_widget_set_size_request (GTK_WIDGET (self->additional_info_spinner), 32, 32);
  gtk_widget_set_valign (GTK_WIDGET (self->additional_info_spinner), GTK_ALIGN_CENTER);
  self->additional_info_label = GTK_LABEL (g_object_ref_sink (gtk_label_new (NULL)));
  gtk_label_set_use_markup (self->additional_info_label, TRUE);
  gtk_label_set_wrap (self->additional_info_label, TRUE);
  gtk_label_set_xalign (self->additional_info_label, 0);
  gtk_label_set_yalign (self->additional_info_label, 0);

  self->additional_info_provider = g_object_new (POLYHYMNIA_TYPE_ADDITIONAL_INFO_PROVIDER, NULL);

#ifdef POLYHYMNIA_FEATURE_LYRICS
  self->lyrics_page_content = ADW_TOOLBAR_VIEW (adw_toolbar_view_new ());
  adw_toolbar_view_add_top_bar (self->lyrics_page_content,
                                adw_header_bar_new ());
  adw_navigation_view_add (self->root_navigation_view,
                           adw_navigation_page_new_with_tag (GTK_WIDGET (self->lyrics_page_content),
                                                             _("Lyrics"),
                                                             "lyrics-page"));

  self->lyrics_row = ADW_ACTION_ROW (g_object_ref_sink (adw_action_row_new ()));
  gtk_list_box_row_set_activatable (GTK_LIST_BOX_ROW (self->lyrics_row), TRUE);
  adw_action_row_add_suffix (self->lyrics_row, gtk_image_new_from_icon_name ("right-symbolic"));
  gtk_actionable_set_detailed_action_name (GTK_ACTIONABLE (self->lyrics_row), "navigation.push::lyrics-page");
  adw_preferences_row_set_title (ADW_PREFERENCES_ROW (self->lyrics_row), _("Lyrics"));
  adw_preferences_group_add (self->details_group, GTK_WIDGET (self->lyrics_row));
  g_object_unref (self->lyrics_row);

  self->lyrics_status_page = ADW_STATUS_PAGE (g_object_ref_sink (adw_status_page_new ()));
  self->lyrics_spinner = GTK_SPINNER (g_object_ref_sink (gtk_spinner_new ()));
  gtk_widget_set_halign (GTK_WIDGET (self->lyrics_spinner), GTK_ALIGN_CENTER);
  gtk_widget_set_size_request (GTK_WIDGET (self->lyrics_spinner), 32, 32);
  gtk_widget_set_valign (GTK_WIDGET (self->lyrics_spinner), GTK_ALIGN_CENTER);
  self->lyrics_web_view = WEBKIT_WEB_VIEW (g_object_ref_sink (webkit_web_view_new ()));
  g_signal_connect_swapped (self->lyrics_web_view, "decide-policy",
                            (GCallback) polyhymnia_track_details_dialog_lyrics_web_view_decide_policy,
                            self);
  g_signal_connect_swapped (self->lyrics_web_view, "load-changed",
                            (GCallback) polyhymnia_track_details_dialog_lyrics_web_view_load_changed,
                            self);

  self->lyrics_provider = g_object_new (POLYHYMNIA_TYPE_LYRICS_PROVIDER, NULL);
  self->uri_launcher = gtk_uri_launcher_new (NULL);
  self->uri_launcher_cancellable = g_cancellable_new ();
#endif

#endif
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

#ifdef POLYHYMNIA_FEATURE_EXTERNAL_DATA
    {
      PolyhymniaSearchTrackInfoRequest additional_info_request;
      additional_info_request.artist_name = polyhymnia_track_full_info_get_artists (details);
      additional_info_request.track_musicbrainz_id = polyhymnia_track_full_info_get_musicbrainz_track_id (details);
      additional_info_request.track_name = polyhymnia_track_full_info_get_title (details);

      self->additional_info_cancellable = g_cancellable_new ();
      if (adw_toolbar_view_get_content (self->additional_info_page_content) != GTK_WIDGET (self->additional_info_spinner))
      {
        adw_toolbar_view_set_content (self->additional_info_page_content,
                                      GTK_WIDGET (g_object_ref (self->additional_info_spinner)));
      }
      gtk_spinner_start (self->lyrics_spinner);
      polyhymnia_additional_info_provider_search_track_info_async (self->additional_info_provider,
                                                                   &additional_info_request,
                                                                   self->additional_info_cancellable,
                                                                   polyhymnia_track_details_dialog_search_additional_info_callback,
                                                                   self);
    }

#ifdef POLYHYMNIA_FEATURE_LYRICS
    {
      PolyhymniaSearchLyricsRequest *search_lyrics_request;

      search_lyrics_request = g_malloc (sizeof (PolyhymniaSearchLyricsRequest));
      search_lyrics_request->artist = g_strdup (polyhymnia_track_full_info_get_artists (details));
      search_lyrics_request->title = g_strdup (polyhymnia_track_full_info_get_title (details));

      self->song_lyrics_cancellable = g_cancellable_new ();
      if (adw_toolbar_view_get_content (self->lyrics_page_content) != GTK_WIDGET (self->lyrics_spinner))
      {
        adw_toolbar_view_set_content (self->lyrics_page_content,
                                      GTK_WIDGET (g_object_ref (self->lyrics_spinner)));
      }
      gtk_spinner_start (self->lyrics_spinner);
      polyhymnia_lyrics_provider_search_lyrics_async (self->lyrics_provider,
                                                      search_lyrics_request,
                                                      self->song_lyrics_cancellable,
                                                      polyhymnia_track_details_dialog_search_song_lyrics_callback,
                                                      self);
    }
#endif

#endif

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
#ifdef POLYHYMNIA_FEATURE_LYRICS
    webkit_web_view_stop_loading (self->lyrics_web_view);
    g_cancellable_cancel (self->song_lyrics_cancellable);
    g_clear_object (&(self->song_lyrics_cancellable));
#endif

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

#ifdef POLYHYMNIA_FEATURE_EXTERNAL_DATA

static void
polyhymnia_track_details_dialog_search_additional_info_callback (GObject      *source,
                                                                 GAsyncResult *result,
                                                                 gpointer      user_data)
{
  GError                            *error = NULL;
  PolyhymniaSearchTrackInfoResponse *response;
  PolyhymniaTrackDetailsDialog      *self = POLYHYMNIA_TRACK_DETAILS_DIALOG (user_data);

  response = polyhymnia_additional_info_provider_search_track_info_finish (POLYHYMNIA_ADDITIONAL_INFO_PROVIDER (source),
                                                                           result,
                                                                           &error);
  if (error != NULL)
  {
    if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
    {
      return;
    }
    else
    {
      g_object_set (G_OBJECT (self->additional_info_status_page),
                    "description", _("Failed to find additional info"),
                    NULL);
      if (adw_toolbar_view_get_content (self->additional_info_page_content) != GTK_WIDGET (self->additional_info_status_page))
      {
        adw_toolbar_view_set_content (self->additional_info_page_content,
                                      GTK_WIDGET (g_object_ref (self->additional_info_status_page)));
      }
    }
  }
  else if (response == NULL || response->description_full == NULL)
  {
    g_object_set (G_OBJECT (self->additional_info_status_page),
                  "description", _("No additional info found"),
                  NULL);
    if (adw_toolbar_view_get_content (self->additional_info_page_content) != GTK_WIDGET (self->additional_info_status_page))
    {
      adw_toolbar_view_set_content (self->additional_info_page_content,
                                    GTK_WIDGET (g_object_ref (self->additional_info_status_page)));
    }
  }
  else
  {
    gtk_label_set_label (self->additional_info_label, response->description_full);

    if (adw_toolbar_view_get_content (self->additional_info_page_content) != GTK_WIDGET (self->additional_info_label))
    {
      adw_toolbar_view_set_content (self->additional_info_page_content,
                                    GTK_WIDGET (g_object_ref (self->additional_info_label)));
    }
  }

  gtk_spinner_stop (GTK_SPINNER (self->additional_info_spinner));

  polyhymnia_search_track_info_response_free (response);
  g_clear_object (&(self->additional_info_cancellable));
}

#ifdef POLYHYMNIA_FEATURE_LYRICS
static gboolean
polyhymnia_track_details_dialog_lyrics_web_view_decide_policy (PolyhymniaTrackDetailsDialog *self,
                                                               WebKitPolicyDecision         *decision,
                                                               WebKitPolicyDecisionType      decision_type,
                                                               WebKitWebView                *user_data)
{
  g_assert (POLYHYMNIA_IS_TRACK_DETAILS_DIALOG (self));

  switch (decision_type)
  {
    case WEBKIT_POLICY_DECISION_TYPE_NAVIGATION_ACTION:
    case WEBKIT_POLICY_DECISION_TYPE_NEW_WINDOW_ACTION:
    {
      WebKitNavigationAction         *navigation_action;
      WebKitNavigationPolicyDecision *navigation_decision;
      const char                     *uri;
      WebKitURIRequest               *uri_request;

      navigation_decision = WEBKIT_NAVIGATION_POLICY_DECISION (decision);
      navigation_action = webkit_navigation_policy_decision_get_navigation_action (navigation_decision);
      uri_request = webkit_navigation_action_get_request (navigation_action);
      uri = webkit_uri_request_get_uri (uri_request);

      // about:blank used for embedded content, it should be opened in WebView
      if (g_strcmp0 ("about:blank", uri) == 0)
      {
        webkit_policy_decision_use (decision);
      }
      else
      {
        webkit_policy_decision_ignore (decision);
        gtk_uri_launcher_set_uri (self->uri_launcher,
                                  webkit_uri_request_get_uri (uri_request));
        gtk_uri_launcher_launch (self->uri_launcher,
                                 GTK_WINDOW (gtk_widget_get_root (GTK_WIDGET (self))),
                                 self->uri_launcher_cancellable,
                                 polyhymnia_track_details_dialog_show_uri_callback,
                                 self);
      }

      break;
    }
    default:
    {
      return FALSE;
    }
  }
  return TRUE;
}

static void
polyhymnia_track_details_dialog_lyrics_web_view_load_changed (PolyhymniaTrackDetailsDialog *self,
                                                              WebKitLoadEvent               load_event,
                                                              WebKitWebView                *user_data)
{
  g_assert (POLYHYMNIA_IS_TRACK_DETAILS_DIALOG (self));

  if (load_event == WEBKIT_LOAD_FINISHED)
  {
    if (adw_toolbar_view_get_content (self->lyrics_page_content) != GTK_WIDGET (self->lyrics_web_view))
    {
      adw_toolbar_view_set_content (self->lyrics_page_content,
                                    GTK_WIDGET (g_object_ref (self->lyrics_web_view)));
    }
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
    if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
    {
      return;
    }
    else
    {
      g_object_set (G_OBJECT (self->lyrics_status_page),
                    "description", _("Failed to find song lyrics"),
                    NULL);
      if (adw_toolbar_view_get_content (self->lyrics_page_content) != GTK_WIDGET (self->lyrics_status_page))
      {
        adw_toolbar_view_set_content (self->lyrics_page_content,
                                      GTK_WIDGET (g_object_ref (self->lyrics_status_page)));
      }
      gtk_spinner_stop (GTK_SPINNER (self->lyrics_spinner));
    }
  }
  else if (lyrics == NULL)
  {
    g_object_set (G_OBJECT (self->lyrics_status_page),
                  "description", _("No song lyrics found"),
                  NULL);
    if (adw_toolbar_view_get_content (self->lyrics_page_content) != GTK_WIDGET (self->lyrics_status_page))
    {
      adw_toolbar_view_set_content (self->lyrics_page_content,
                                    GTK_WIDGET (g_object_ref (self->lyrics_status_page)));
    }
    gtk_spinner_stop (GTK_SPINNER (self->lyrics_spinner));
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

  g_clear_object (&(self->song_lyrics_cancellable));
}

static void
polyhymnia_track_details_dialog_show_uri_callback (GObject      *source_object,
                                                   GAsyncResult *result,
                                                   gpointer      user_data)
{
  GError *error = NULL;

  g_assert (POLYHYMNIA_IS_TRACK_DETAILS_DIALOG (user_data));

  gtk_uri_launcher_launch_finish (GTK_URI_LAUNCHER (source_object), result,
                                  &error);
  if (error != NULL)
  {
    if (!g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
    {
      g_warning ("Failed to open requested URI. Error: %s", error->message);
    }
    g_error_free (error);
  }
}
#endif

#endif

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
