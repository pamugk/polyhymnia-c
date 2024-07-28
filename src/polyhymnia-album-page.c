
#include "app-features.h"
#include "config.h"

#include "polyhymnia-album-page.h"

#include "polyhymnia-mpd-client-api.h"
#include "polyhymnia-mpd-client-images.h"

#ifdef POLYHYMNIA_FEATURE_EXTERNAL_DATA

#include "polyhymnia-additional-info-provider.h"

#endif

#define _(x) g_dgettext (GETTEXT_PACKAGE, x)

/* Type metadata */
typedef enum
{
  PROP_ALBUM_TITLE = 1,
  N_PROPERTIES,
} PolyhymniaAlbumPageProperty;

typedef enum
{
  SIGNAL_DELETED = 1,
  SIGNAL_VIEW_TRACK_DETAILS,
  N_SIGNALS,
} PolyhymniaAlbumPageSignal;

struct _PolyhymniaAlbumPage
{
  AdwNavigationPage  parent_instance;

  /* Stored UI state */
  GdkTexture                       *album_cover;
  GCancellable                     *album_cover_cancellable;
  GCancellable                     *tracks_cancellable;

  /* Template widgets */
  AdwToolbarView                   *root_toolbar_view;
  AdwHeaderBar                     *root_header_bar;
  GtkButton                        *play_button;
  GtkButton                        *playlist_add_button;
  AdwBreakpointBin                 *root_content;

  GtkImage                         *cover_image;
  GtkLabel                         *artist_label;
  GtkLabel                         *year_label;

  GtkScrolledWindow                *tracks_content_scroll;
  GtkColumnView                    *tracks_column_view;

  GtkLabel                         *statistics_label;
  GtkLabel                         *duration_label;

  GtkSpinner                       *spinner;

  AdwStatusPage                    *tracks_status_page;

  /* Template objects */
  PolyhymniaMpdClient              *mpd_client;

  GtkBuilderListItemFactory        *disc_header_factory;
  GListStore                       *tracks_model;
  GtkNoSelection                   *tracks_selection_model;
  GtkSortListModel                 *tracks_sort_model;

  /* Instance properties */
  char                             *album_title;

// Feature-specific fields
#ifdef POLYHYMNIA_FEATURE_EXTERNAL_DATA
  GtkButton                        *additional_info_button;
  AdwDialog                        *additional_info_dialog;

  char                             *additional_info;
  PolyhymniaAdditionalInfoProvider *additional_info_provider;
  GCancellable                     *additional_info_cancellable;
#endif
};

G_DEFINE_FINAL_TYPE (PolyhymniaAlbumPage, polyhymnia_album_page, ADW_TYPE_NAVIGATION_PAGE)

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

static unsigned int obj_signals[N_SIGNALS] = { 0, };

/* Event handler declarations */
static void
polyhymnia_album_page_get_album_cover_callback (GObject      *source_object,
                                                GAsyncResult *result,
                                                void         *user_data);

static void
polyhymnia_album_page_get_album_tracks_callback (GObject      *source_object,
                                                 GAsyncResult *result,
                                                 void         *user_data);

static void
polyhymnia_album_page_mpd_client_initialized (PolyhymniaAlbumPage *self,
                                              GParamSpec          *pspec,
                                              PolyhymniaMpdClient *user_data);

static void
polyhymnia_album_page_mpd_database_updated (PolyhymniaAlbumPage *self,
                                            PolyhymniaMpdClient *user_data);

static void
polyhymnia_album_page_add_album_to_queue_button_clicked (PolyhymniaAlbumPage *self,
                                                         GtkButton           *user_data);

static void
polyhymnia_album_page_play_album_button_clicked (PolyhymniaAlbumPage *self,
                                                 GtkButton           *user_data);

static void
polyhymnia_album_page_track_activated (PolyhymniaAlbumPage *self,
                                       unsigned int         position,
                                       GtkColumnView       *user_data);

#ifdef POLYHYMNIA_FEATURE_EXTERNAL_DATA
static void
polyhymnia_album_page_additional_info_button_clicked (PolyhymniaAlbumPage *self,
                                                      GtkButton           *user_data);

static void
polyhymnia_album_page_additional_info_dialog_closed (PolyhymniaAlbumPage *self,
                                                     AdwDialog           *user_data);

static void
polyhymnia_album_page_search_additional_info_callback (GObject      *source,
                                                       GAsyncResult *result,
                                                       gpointer      user_data);
#endif

/* Private function declarations */
static char *
get_disc_title (GtkListHeader *header, PolyhymniaTrack *item);

static void
polyhymnia_album_page_get_album_cover_async (PolyhymniaAlbumPage *self,
                                             GPtrArray           *tracks,
                                             GCancellable        *cancellable,
                                             GAsyncReadyCallback  callback,
                                             void                *user_data);

static void
polyhymnia_album_page_get_album_cover_async_thread (GTask         *task,
                                                    void          *source_object,
                                                    void          *task_data,
                                                    GCancellable  *cancellable);

static GdkTexture *
polyhymnia_album_page_get_album_cover_finish (PolyhymniaAlbumPage *self,
                                              GAsyncResult        *result,
                                              GError             **error);

/* Class stuff */
static void
polyhymnia_album_page_constructed (GObject *gobject)
{
  PolyhymniaAlbumPage *self = POLYHYMNIA_ALBUM_PAGE (gobject);

  adw_navigation_page_set_title (ADW_NAVIGATION_PAGE (self), self->album_title);
  polyhymnia_album_page_mpd_client_initialized (self, NULL, self->mpd_client);

  G_OBJECT_CLASS (polyhymnia_album_page_parent_class)->constructed (gobject);
}

static void
polyhymnia_album_page_dispose (GObject *gobject)
{
  PolyhymniaAlbumPage *self = POLYHYMNIA_ALBUM_PAGE (gobject);

  g_cancellable_cancel (self->album_cover_cancellable);
  g_clear_object (&(self->album_cover_cancellable));
  g_clear_object (&(self->album_cover));
  g_clear_pointer (&(self->album_title), g_free);
  g_cancellable_cancel (self->tracks_cancellable);
  g_clear_object (&(self->tracks_cancellable));

#ifdef POLYHYMNIA_FEATURE_EXTERNAL_DATA
  g_cancellable_cancel (self->additional_info_cancellable);
  g_clear_object (&(self->additional_info_cancellable));

  g_clear_pointer (&(self->additional_info_dialog), adw_dialog_close);
  g_clear_pointer (&(self->additional_info), g_free);
#endif

  adw_navigation_page_set_child (ADW_NAVIGATION_PAGE (self), NULL);
  gtk_widget_dispose_template (GTK_WIDGET (self), POLYHYMNIA_TYPE_ALBUM_PAGE);

  G_OBJECT_CLASS (polyhymnia_album_page_parent_class)->dispose (gobject);
}

static void
polyhymnia_album_page_get_property (GObject     *object,
                                    unsigned int property_id,
                                    GValue      *value,
                                    GParamSpec  *pspec)
{
  PolyhymniaAlbumPage *self = POLYHYMNIA_ALBUM_PAGE (object);

  switch ((PolyhymniaAlbumPageProperty) property_id)
    {
    case PROP_ALBUM_TITLE:
      g_value_set_string (value, self->album_title);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
polyhymnia_album_page_set_property (GObject      *object,
                                    unsigned int  property_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
  PolyhymniaAlbumPage *self = POLYHYMNIA_ALBUM_PAGE (object);

  switch ((PolyhymniaAlbumPageProperty) property_id)
    {
    case PROP_ALBUM_TITLE:
      g_set_str (&(self->album_title), g_value_get_string (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
polyhymnia_album_page_class_init (PolyhymniaAlbumPageClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GType type = G_TYPE_FROM_CLASS (gobject_class);
  GType view_detail_types[] = { G_TYPE_STRING };

  gobject_class->constructed = polyhymnia_album_page_constructed;
  gobject_class->dispose = polyhymnia_album_page_dispose;
  gobject_class->get_property = polyhymnia_album_page_get_property;
  gobject_class->set_property = polyhymnia_album_page_set_property;

  obj_properties[PROP_ALBUM_TITLE] =
    g_param_spec_string ("album-title",
                         "Album title",
                         "Title of a displayed album.",
                         NULL,
                         G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_NAME);

  g_object_class_install_properties (gobject_class,
                                     N_PROPERTIES,
                                     obj_properties);

  obj_signals[SIGNAL_DELETED] =
     g_signal_newv ("deleted", type,
                    G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                    NULL, NULL, NULL, NULL,
                    G_TYPE_NONE,
                    0, NULL);
  obj_signals[SIGNAL_VIEW_TRACK_DETAILS] =
     g_signal_newv ("view-track-details", type,
                    G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                    NULL, NULL, NULL, NULL,
                    G_TYPE_NONE,
                    1, view_detail_types);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/com/github/pamugk/polyhymnia/ui/polyhymnia-album-page.ui");

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaAlbumPage, root_toolbar_view);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaAlbumPage, root_header_bar);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaAlbumPage, play_button);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaAlbumPage, playlist_add_button);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaAlbumPage, root_content);

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaAlbumPage, cover_image);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaAlbumPage, artist_label);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaAlbumPage, year_label);

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaAlbumPage, tracks_content_scroll);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaAlbumPage, tracks_column_view);

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaAlbumPage, statistics_label);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaAlbumPage, duration_label);

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaAlbumPage, spinner);

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaAlbumPage, tracks_status_page);

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaAlbumPage, disc_header_factory);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaAlbumPage, mpd_client);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaAlbumPage, tracks_selection_model);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaAlbumPage, tracks_sort_model);

  gtk_widget_class_bind_template_callback (widget_class,
                                           get_disc_title);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_track_get_disc);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_album_page_mpd_database_updated);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_album_page_mpd_client_initialized);

  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_album_page_add_album_to_queue_button_clicked);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_album_page_play_album_button_clicked);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_album_page_track_activated);
}

static void
polyhymnia_album_page_init (PolyhymniaAlbumPage *self)
{
  self->tracks_model = g_list_store_new (POLYHYMNIA_TYPE_TRACK);
  gtk_widget_init_template (GTK_WIDGET (self));

#ifdef POLYHYMNIA_FEATURE_EXTERNAL_DATA
  self->additional_info_button = GTK_BUTTON (gtk_button_new_from_icon_name ("info-symbolic"));
  g_signal_connect_swapped (self->additional_info_button, "clicked",
                            (GCallback) polyhymnia_album_page_additional_info_button_clicked,
                            self);
  adw_header_bar_pack_end (self->root_header_bar,
                           GTK_WIDGET(self->additional_info_button));
  self->additional_info_provider = g_object_new (POLYHYMNIA_TYPE_ADDITIONAL_INFO_PROVIDER, NULL);
#endif
  adw_header_bar_pack_end (self->root_header_bar, GTK_WIDGET(self->playlist_add_button));
  adw_header_bar_pack_end (self->root_header_bar, GTK_WIDGET(self->play_button));

  gtk_sort_list_model_set_model (self->tracks_sort_model,
                                 G_LIST_MODEL (self->tracks_model));
}

/* Event handler implementations */
static void
polyhymnia_album_page_get_album_cover_callback (GObject      *source_object,
                                                GAsyncResult *result,
                                                void         *user_data)
{
  GdkTexture          *cover_image;
  GError              *error = NULL;
  PolyhymniaAlbumPage *self = POLYHYMNIA_ALBUM_PAGE (source_object);

  cover_image = polyhymnia_album_page_get_album_cover_finish (self, result, &error);

  if (error == NULL)
  {
    if (cover_image == NULL)
    {
      gtk_image_set_from_icon_name (self->cover_image, "cd-symbolic");
    }
    else
    {
      self->album_cover = cover_image;
      gtk_image_set_from_paintable (self->cover_image,
                                    GDK_PAINTABLE (cover_image));
    }

    g_clear_object (&(self->album_cover_cancellable));
  }
  else if (!g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
  {
    g_warning ("Failed to get album cover: %s\n", error->message);
    gtk_image_set_from_icon_name (self->cover_image, "cd-symbolic");

    g_clear_object (&(self->album_cover_cancellable));
  }
}

static void
polyhymnia_album_page_get_album_tracks_callback (GObject      *source_object,
                                                 GAsyncResult *result,
                                                 void         *user_data)
{
  GError              *error = NULL;
  PolyhymniaMpdClient *mpd_client = POLYHYMNIA_MPD_CLIENT (source_object);
  PolyhymniaAlbumPage *self = user_data;
  GPtrArray           *tracks;

  tracks = polyhymnia_mpd_client_get_album_tracks_finish (mpd_client, result,
                                                          &error);
  if (error == NULL)
  {
    g_clear_object (&(self->album_cover));
    if (tracks->len == 0)
    {
      g_list_store_remove_all (self->tracks_model);
      g_object_set (G_OBJECT (self->tracks_status_page),
                    "description", _("Album not found"),
                    NULL);
      adw_toolbar_view_set_content (self->root_toolbar_view,
                                    GTK_WIDGET (self->tracks_status_page));
      g_signal_emit (self, obj_signals[SIGNAL_DELETED], 0);
    }
    else
    {
      PolyhymniaTrack *any_track = g_ptr_array_index (tracks, 0);
      unsigned int     last_seen_disc = polyhymnia_track_get_disc (any_track);
      unsigned int     total_duration = polyhymnia_track_get_duration (any_track);
      char            *total_duration_translated;
      unsigned int     hours;
      unsigned int     minutes;
      char            *statistics = g_strdup_printf (g_dngettext(GETTEXT_PACKAGE, "%d song", "%d songs", tracks->len),
                                                     tracks->len);
      gboolean multidisc_album = FALSE;
      for (unsigned int i = 1; i < tracks->len; i++)
      {
        const PolyhymniaTrack *track = g_ptr_array_index (tracks, i);
        unsigned int current_disc = polyhymnia_track_get_disc (track);
        multidisc_album = multidisc_album || last_seen_disc != current_disc;
        last_seen_disc = current_disc;
        total_duration += polyhymnia_track_get_duration (track);
      }

      minutes = (total_duration % 3600) / 60;
      hours = total_duration / 3600;
      if (hours > 0)
      {
        total_duration_translated = g_strdup_printf (_("%d h. %d min."),
                                                     hours, minutes);
      }
      else
      {
        total_duration_translated = g_strdup_printf (_("%d min."), minutes);
      }

      if (multidisc_album)
      {
        gtk_column_view_set_header_factory (self->tracks_column_view,
                                            GTK_LIST_ITEM_FACTORY (self->disc_header_factory));
      }
      else
      {
        gtk_column_view_set_header_factory (self->tracks_column_view, NULL);
      }

      gtk_image_set_from_icon_name (self->cover_image, "cd-symbolic");
      gtk_label_set_text (self->artist_label,
                          polyhymnia_track_get_album_artist (any_track));
      gtk_label_set_text (self->year_label, polyhymnia_track_get_date (any_track));

      gtk_label_set_text (self->statistics_label, statistics);
      gtk_label_set_text (self->duration_label, total_duration_translated);
      g_free (total_duration_translated);
      g_free (statistics);

      self->album_cover_cancellable = g_cancellable_new ();
      polyhymnia_album_page_get_album_cover_async (self, tracks,
                                                   self->album_cover_cancellable,
                                                   polyhymnia_album_page_get_album_cover_callback,
                                                   NULL);

      g_list_store_splice (self->tracks_model, 0,
                            g_list_model_get_n_items (G_LIST_MODEL (self->tracks_model)),
                            tracks->pdata, tracks->len);
      adw_toolbar_view_set_content (self->root_toolbar_view,
                                    GTK_WIDGET (self->root_content));

#ifdef POLYHYMNIA_FEATURE_EXTERNAL_DATA
      {
        // TODO: maybe iterate over tracks to guarantee that all search params are available?
        PolyhymniaSearchAlbumInfoRequest additional_info_request;
        additional_info_request.artist_name = polyhymnia_track_get_album_artist (any_track);
        additional_info_request.album_name = self->album_title;
        additional_info_request.album_musicbrainz_id = polyhymnia_track_get_musicbrainz_album_id (any_track);

        self->additional_info_cancellable = g_cancellable_new ();
        polyhymnia_additional_info_provider_search_album_info_async (self->additional_info_provider,
                                                                     &additional_info_request,
                                                                     self->additional_info_cancellable,
                                                                     polyhymnia_album_page_search_additional_info_callback,
                                                                     self);
      }
#endif
    }
    g_ptr_array_unref (tracks);
  }
  else if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
  {
    return;
  }
  else
  {
    g_clear_object (&(self->album_cover));
    g_list_store_remove_all (self->tracks_model);
    g_object_set (G_OBJECT (self->tracks_status_page),
                  "description", _("Failed to get an album"),
                  NULL);
    adw_toolbar_view_set_content (self->root_toolbar_view,
                                  GTK_WIDGET (self->tracks_status_page));
    g_warning("Failed to find an album: %s", error->message);
  }

  gtk_spinner_stop (self->spinner);
  g_clear_object (&(self->tracks_cancellable));
}

static void
polyhymnia_album_page_mpd_client_initialized (PolyhymniaAlbumPage   *self,
                                              GParamSpec            *pspec,
                                              PolyhymniaMpdClient   *user_data)
{
  g_assert (POLYHYMNIA_IS_ALBUM_PAGE (self));

  g_clear_object (&(self->album_cover));
  if (polyhymnia_mpd_client_is_initialized (user_data))
  {
    polyhymnia_album_page_mpd_database_updated (self, user_data);
  }
  else
  {
    g_cancellable_cancel (self->album_cover_cancellable);
    g_clear_object (&(self->album_cover_cancellable));
    g_clear_object (&(self->album_cover));

    g_cancellable_cancel (self->tracks_cancellable);
    g_clear_object (&(self->tracks_cancellable));
    g_list_store_remove_all (self->tracks_model);

#ifdef POLYHYMNIA_FEATURE_EXTERNAL_DATA
    g_cancellable_cancel (self->additional_info_cancellable);
    g_clear_object (&(self->additional_info_cancellable));
    gtk_widget_set_sensitive (GTK_WIDGET (self->additional_info_button), FALSE);
    g_clear_pointer (&(self->additional_info_dialog), adw_dialog_close);
    g_clear_pointer (&(self->additional_info), g_free);
#endif
  }
}

static void
polyhymnia_album_page_mpd_database_updated (PolyhymniaAlbumPage *self,
                                            PolyhymniaMpdClient *user_data)
{
  g_assert (POLYHYMNIA_IS_ALBUM_PAGE (self));

  g_cancellable_cancel (self->album_cover_cancellable);
  g_clear_object (&(self->album_cover_cancellable));

  g_cancellable_cancel (self->tracks_cancellable);
  g_clear_object (&(self->tracks_cancellable));

#ifdef POLYHYMNIA_FEATURE_EXTERNAL_DATA
  g_cancellable_cancel (self->additional_info_cancellable);
  g_clear_object (&(self->additional_info_cancellable));
  gtk_widget_set_sensitive (GTK_WIDGET (self->additional_info_button), FALSE);
  g_clear_pointer (&(self->additional_info_dialog), adw_dialog_close);
  g_clear_pointer (&(self->additional_info), g_free);
#endif

  self->tracks_cancellable = g_cancellable_new ();

  polyhymnia_mpd_client_get_album_tracks_async (user_data,
                                                self->album_title,
                                                self->tracks_cancellable,
                                                polyhymnia_album_page_get_album_tracks_callback,
                                                self);

  if (adw_toolbar_view_get_content (self->root_toolbar_view) != GTK_WIDGET (self->spinner))
  {
    adw_toolbar_view_set_content (self->root_toolbar_view,
                                  GTK_WIDGET (self->spinner));
  }
  gtk_spinner_start (self->spinner);
}

static void
polyhymnia_album_page_add_album_to_queue_button_clicked (PolyhymniaAlbumPage *self,
                                                         GtkButton           *user_data)
{
  GError *error = NULL;

  g_assert (POLYHYMNIA_IS_ALBUM_PAGE (self));

  polyhymnia_mpd_client_append_album_to_queue (self->mpd_client,
                                               self->album_title, &error);

  if (error != NULL)
  {
    g_warning("Failed to add album into queue: %s\n", error->message);
    g_error_free (error);
    error = NULL;
  }
}

static void
polyhymnia_album_page_play_album_button_clicked (PolyhymniaAlbumPage *self,
                                                 GtkButton           *user_data)
{
  GError *error = NULL;

  g_assert (POLYHYMNIA_IS_ALBUM_PAGE (self));

  polyhymnia_mpd_client_play_album (self->mpd_client,
                                    self->album_title, &error);

  if (error != NULL)
  {
    g_warning("Failed to start playing album: %s\n", error->message);
    g_error_free (error);
    error = NULL;
  }
}

static void
polyhymnia_album_page_track_activated (PolyhymniaAlbumPage *self,
                                       unsigned int         position,
                                       GtkColumnView       *user_data)
{
  PolyhymniaTrack *track;

  g_assert (POLYHYMNIA_IS_ALBUM_PAGE (self));

  track = g_list_model_get_item (G_LIST_MODEL (self->tracks_model), position);
  g_signal_emit (self, obj_signals[SIGNAL_VIEW_TRACK_DETAILS], 0,
                 polyhymnia_track_get_uri (track));
}

#ifdef POLYHYMNIA_FEATURE_EXTERNAL_DATA
static void
polyhymnia_album_page_additional_info_button_clicked (PolyhymniaAlbumPage *self,
                                                      GtkButton           *user_data)
{
  AdwToolbarView    *additional_info_toolbar_view;
  GtkScrolledWindow *additional_info_scrolled_window;
  AdwClamp          *additional_info_label_clamp;
  GtkLabel          *additional_info_label;
  AdwBin            *lastfm_bin = ADW_BIN (adw_bin_new ());
  GtkLabel          *lastfm_label = GTK_LABEL (gtk_label_new (_("Powered by <a href=\"https://www.last.fm\">Last.fm</a>")));

  g_assert (POLYHYMNIA_IS_ALBUM_PAGE (self));

  self->additional_info_dialog = adw_dialog_new ();
  adw_dialog_set_content_height (self->additional_info_dialog, 588);
  adw_dialog_set_content_width (self->additional_info_dialog, 360);
  adw_dialog_set_title (self->additional_info_dialog, _("Additional Info"));
  g_signal_connect_swapped (self->additional_info_dialog,
                            "closed",
                            (GCallback) polyhymnia_album_page_additional_info_dialog_closed,
                            self);

  additional_info_toolbar_view = ADW_TOOLBAR_VIEW (adw_toolbar_view_new ());
  gtk_label_set_use_markup (lastfm_label, TRUE);
  gtk_label_set_xalign (lastfm_label, 0);
  adw_bin_set_child (lastfm_bin, GTK_WIDGET (lastfm_label));
  gtk_widget_add_css_class (GTK_WIDGET (lastfm_bin), "toolbar");
  adw_toolbar_view_add_bottom_bar (additional_info_toolbar_view,
                                   GTK_WIDGET (lastfm_bin));
  adw_toolbar_view_add_top_bar (additional_info_toolbar_view, adw_header_bar_new ());

  additional_info_scrolled_window = GTK_SCROLLED_WINDOW (gtk_scrolled_window_new ());
  gtk_scrolled_window_set_policy (additional_info_scrolled_window,
                                  GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  additional_info_label = GTK_LABEL (gtk_label_new (NULL));
  gtk_label_set_use_markup (additional_info_label, TRUE);
  gtk_label_set_label (additional_info_label, self->additional_info);
  gtk_label_set_wrap (additional_info_label, TRUE);
  gtk_label_set_xalign (additional_info_label, 0);
  gtk_label_set_yalign (additional_info_label, 0);
  additional_info_label_clamp = ADW_CLAMP (g_object_ref_sink (adw_clamp_new ()));
  adw_clamp_set_child (additional_info_label_clamp, GTK_WIDGET (additional_info_label));
  gtk_widget_set_margin_bottom (GTK_WIDGET (additional_info_label_clamp), 16);
  gtk_widget_set_margin_end (GTK_WIDGET (additional_info_label_clamp), 12);
  gtk_widget_set_margin_start (GTK_WIDGET (additional_info_label_clamp), 16);
  gtk_widget_set_margin_top (GTK_WIDGET (additional_info_label_clamp), 12);
  gtk_scrolled_window_set_child (additional_info_scrolled_window,
                                   GTK_WIDGET (additional_info_label_clamp));
  adw_toolbar_view_set_content (additional_info_toolbar_view,
                                GTK_WIDGET (additional_info_scrolled_window));
  adw_dialog_set_child (self->additional_info_dialog,
                        GTK_WIDGET (additional_info_toolbar_view));

  adw_dialog_present (self->additional_info_dialog, GTK_WIDGET (self));
}

static void
polyhymnia_album_page_additional_info_dialog_closed (PolyhymniaAlbumPage *self,
                                                     AdwDialog           *user_data)
{
  g_return_if_fail (POLYHYMNIA_ALBUM_PAGE (self));
  self->additional_info_dialog = NULL;
}

static void
polyhymnia_album_page_search_additional_info_callback (GObject      *source,
                                                       GAsyncResult *result,
                                                       gpointer      user_data)
{
  GError                            *error = NULL;
  PolyhymniaSearchAlbumInfoResponse *response;

  response = polyhymnia_additional_info_provider_search_album_info_finish (POLYHYMNIA_ADDITIONAL_INFO_PROVIDER (source),
                                                                           result,
                                                                           &error);

  if (error == NULL || !g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
  {
    PolyhymniaAlbumPage *self = POLYHYMNIA_ALBUM_PAGE (user_data);
    if (response != NULL)
    {
      gtk_widget_set_sensitive (GTK_WIDGET (self->additional_info_button),
                                response->description_full != NULL
                                && g_strcmp0 ("", response->description_full) != 0);
      self->additional_info = g_strdup (response->description_full);
      polyhymnia_search_album_info_response_free (response);
    }
    g_clear_object (&(self->additional_info_cancellable));
  }
}
#endif

/* Private function declarations */
static char *
get_disc_title (GtkListHeader *header, PolyhymniaTrack *item)
{
  if (item == NULL)
  {
    return NULL;
  }
  else
  {
    unsigned int disc = polyhymnia_track_get_disc (item);
    return disc == 0
      ? g_strdup (_("Disc —"))
      : g_strdup_printf (_("Disc %d"), disc);
  }
}

static void
polyhymnia_album_page_get_album_cover_async (PolyhymniaAlbumPage *self,
                                             GPtrArray           *tracks,
                                             GCancellable        *cancellable,
                                             GAsyncReadyCallback  callback,
                                             void                *user_data)
{
  GTask *task;

  task = g_task_new (self, cancellable, callback, user_data);
  g_task_set_task_data (task, g_ptr_array_ref (tracks),
                        (GDestroyNotify) g_ptr_array_unref);
  g_task_set_source_tag (task, polyhymnia_album_page_get_album_cover_async);
  g_task_set_return_on_cancel (task, TRUE);
  g_task_run_in_thread (task, polyhymnia_album_page_get_album_cover_async_thread);
  g_object_unref (task);
}

static void
polyhymnia_album_page_get_album_cover_async_thread (GTask         *task,
                                                    void          *source_object,
                                                    void          *task_data,
                                                    GCancellable  *cancellable)
{
  GdkTexture          *cover_image = NULL;
  PolyhymniaAlbumPage *self = source_object;
  GPtrArray           *tracks = task_data;

  for (unsigned int i = 0; i < tracks->len; i++)
  {
    GBytes          *cover = NULL;
    GError          *error = NULL;
    PolyhymniaTrack *track = g_ptr_array_index (tracks, i);

    cover = polyhymnia_mpd_client_get_song_album_cover (self->mpd_client,
                                                        polyhymnia_track_get_uri (track),
                                                        &error);

    if (error != NULL)
    {
      g_error_free (error);
    }
    else if (cover != NULL)
    {
      cover_image = gdk_texture_new_from_bytes (cover, &error);
      g_bytes_unref (cover);
      if (error == NULL)
      {
        break;
      }
      else
      {
        g_error_free (error);
      }
    }
  }

  if (g_task_set_return_on_cancel (task, FALSE))
  {
    g_task_return_pointer (task, cover_image, (GDestroyNotify) g_object_unref);
  }
}

static GdkTexture *
polyhymnia_album_page_get_album_cover_finish (PolyhymniaAlbumPage *self,
                                              GAsyncResult        *result,
                                              GError             **error)
{
  g_return_val_if_fail (g_task_is_valid (result, self), NULL);
  return g_task_propagate_pointer (G_TASK (result), error);
}
