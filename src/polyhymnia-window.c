
#include "config.h"

#include "polyhymnia-mpd-client.h"
#include "polyhymnia-player.h"
#include "polyhymnia-window.h"

#define _(x) g_dgettext (GETTEXT_PACKAGE, x)

/* Type metadata */
struct _PolyhymniaWindow
{
  AdwApplicationWindow  parent_instance;

  /* Template widgets */
  AdwOverlaySplitView *content;
  AdwStatusPage       *no_mpd_connection_page;
  AdwToastOverlay     *root_toast_overlay;

  GtkLabel            *current_track_artist_label;
  GtkLabel            *current_track_title_label;
  GtkButton           *play_button;

  AdwBin              *artist_stack_page_content;
  AdwNavigationView   *artist_navigation_view;
  AdwStatusPage       *artists_status_page;

  AdwBin              *album_stack_page_content;
  AdwNavigationView   *album_navigation_view;
  AdwStatusPage       *albums_status_page;

  AdwBin              *track_stack_page_content;
  AdwNavigationView   *track_navigation_view;
  AdwToolbarView      *track_toolbar_view;
  GtkButton           *play_tracks_button;
  GtkButton           *add_tracks_to_queue_button;
  AdwStatusPage       *tracks_status_page;

  AdwBin              *genre_stack_page_content;
  AdwNavigationView   *genre_navigation_view;
  AdwStatusPage       *genres_status_page;

  GtkButton           *clear_queue_button;
  GtkActionBar        *queue_action_bar;
  GtkScrolledWindow   *queue_page_content;
  GtkListView         *queue_list_view;
  AdwStatusPage       *queue_status_page;
  GtkButton           *queue_to_playlist_button;

  /* Template objects */
  PolyhymniaMpdClient *mpd_client;
  PolyhymniaPlayer    *player;
  GSettings           *settings;

  GListStore          *album_model;
  GtkNoSelection      *album_selection_model;
  GListStore          *artist_model;
  GtkNoSelection      *artist_selection_model;
  GtkStringList       *genre_model;
  GtkNoSelection      *genre_selection_model;
  GListStore          *track_model;
  GtkMultiSelection   *track_selection_model;

  GListStore          *queue_model;
  GtkNoSelection      *queue_selection_model;
};

G_DEFINE_FINAL_TYPE (PolyhymniaWindow, polyhymnia_window, ADW_TYPE_APPLICATION_WINDOW)

/* Event handler & other utility functions declaration */
static void
polyhymnia_window_add_tracks_to_queue_button_clicked (GtkButton* self,
                                                      gpointer user_data);
static void
polyhymnia_window_clear_queue_button_clicked (GtkButton* self,
                                              gpointer user_data);

static void
polyhymnia_window_content_clear (PolyhymniaWindow *self);

static void
polyhymnia_window_content_init (PolyhymniaWindow *self);

static void
polyhymnia_window_mpd_client_initialized(PolyhymniaMpdClient* self,
                                         GParamSpec* pspec,
                                         PolyhymniaWindow* user_data);

static void
polyhymnia_window_mpd_database_updated(GObject* self, gpointer user_data);

static void
polyhymnia_window_mpd_queue_modified(GObject* self, gpointer user_data);

static void
polyhymnia_window_play_tracks_button_clicked (GtkButton* self,
                                              gpointer user_data);

static void
polyhymnia_window_player_current_track(PolyhymniaPlayer* self,
                                       GParamSpec* pspec,
                                       PolyhymniaWindow* user_data);

static void
polyhymnia_window_player_state(PolyhymniaPlayer* self,
                               GParamSpec* pspec,
                               PolyhymniaWindow* user_data);

static const gchar *
polyhymnia_window_player_state_to_icon(PolyhymniaPlayerPlaybackStatus state);

static void
polyhymnia_window_queue_to_playlist_button_clicked (GtkButton* self,
                                                    gpointer user_data);

static void
polyhymnia_window_queue_pane_init (PolyhymniaWindow *self);

static void
polyhymnia_window_track_selection_changed (GtkSelectionModel* self,
                                           guint position,
                                           guint n_items,
                                           gpointer user_data);

/* Class stuff */
static void
polyhymnia_window_dispose(GObject *gobject)
{
  PolyhymniaWindow *self = POLYHYMNIA_WINDOW (gobject);

  gtk_widget_dispose_template (GTK_WIDGET (self), POLYHYMNIA_TYPE_WINDOW);

  G_OBJECT_CLASS (polyhymnia_window_parent_class)->dispose (gobject);
}

static void
polyhymnia_window_class_init (PolyhymniaWindowClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->dispose = polyhymnia_window_dispose;

  gtk_widget_class_set_template_from_resource (widget_class, "/com/github/pamugk/polyhymnia/ui/polyhymnia-window.ui");

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, content);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, no_mpd_connection_page);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, root_toast_overlay);

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, current_track_artist_label);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, current_track_title_label);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, play_button);

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, artist_stack_page_content);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, artist_navigation_view);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, artists_status_page);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, album_stack_page_content);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, album_navigation_view);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, albums_status_page);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, genre_stack_page_content);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, genre_navigation_view);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, genres_status_page);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, track_stack_page_content);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, track_navigation_view);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, track_toolbar_view);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, tracks_status_page);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, play_tracks_button);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, add_tracks_to_queue_button);

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, clear_queue_button);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, queue_action_bar);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, queue_page_content);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, queue_list_view);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, queue_status_page);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, queue_to_playlist_button);

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, mpd_client);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, player);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, settings);

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, album_selection_model);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, artist_selection_model);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, genre_model);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, genre_selection_model);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, track_selection_model);

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, queue_selection_model);
}

static void
polyhymnia_window_init (PolyhymniaWindow *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  self->album_model = g_list_store_new (POLYHYMNIA_TYPE_ALBUM);
  gtk_no_selection_set_model (self->album_selection_model,
                              G_LIST_MODEL (self->album_model));
  self->artist_model = g_list_store_new (POLYHYMNIA_TYPE_ARTIST);
  gtk_no_selection_set_model (self->artist_selection_model,
                              G_LIST_MODEL (self->artist_model));
  self->track_model = g_list_store_new (POLYHYMNIA_TYPE_TRACK);
  gtk_multi_selection_set_model (self->track_selection_model,
                                 G_LIST_MODEL (self->track_model));

  self->queue_model = g_list_store_new (POLYHYMNIA_TYPE_TRACK);
  gtk_no_selection_set_model (self->queue_selection_model,
                              G_LIST_MODEL (self->queue_model));

  g_settings_bind (self->settings, "window-width",
                    self, "default-width",
                    G_SETTINGS_BIND_DEFAULT);
  g_settings_bind (self->settings, "window-height",
                    self, "default-height",
                    G_SETTINGS_BIND_DEFAULT);
  g_settings_bind (self->settings, "window-maximized",
                    self, "maximized",
                    G_SETTINGS_BIND_DEFAULT);
  g_settings_bind (self->settings, "window-fullscreened",
                    self, "fullscreened",
                    G_SETTINGS_BIND_DEFAULT);

  g_signal_connect (self->add_tracks_to_queue_button, "clicked",
                    G_CALLBACK (polyhymnia_window_add_tracks_to_queue_button_clicked),
                    self);
  g_signal_connect (self->play_tracks_button, "clicked",
                    G_CALLBACK (polyhymnia_window_play_tracks_button_clicked),
                    self);
  g_signal_connect (self->track_selection_model, "selection-changed",
                    G_CALLBACK (polyhymnia_window_track_selection_changed),
                    self);

  g_signal_connect (self->clear_queue_button, "clicked",
                    G_CALLBACK (polyhymnia_window_clear_queue_button_clicked),
                    self);
  g_signal_connect (self->clear_queue_button, "clicked",
                    G_CALLBACK (polyhymnia_window_queue_to_playlist_button_clicked),
                    self);

  polyhymnia_window_mpd_client_initialized (self->mpd_client, NULL, self);
  g_signal_connect (self->mpd_client, "notify::initialized",
                    G_CALLBACK (polyhymnia_window_mpd_client_initialized),
                    self);

  polyhymnia_window_player_current_track (self->player, NULL, self);
  g_signal_connect (self->player, "notify::current-track",
                    G_CALLBACK (polyhymnia_window_player_current_track),
                    self);

  polyhymnia_window_player_state (self->player, NULL, self);
  g_signal_connect (self->player, "notify::playback-status",
                    G_CALLBACK (polyhymnia_window_player_state),
                    self);

  g_signal_connect (self->mpd_client, "database-updated",
                    G_CALLBACK (polyhymnia_window_mpd_database_updated),
                    self);
  g_signal_connect (self->mpd_client, "queue-modified",
                    G_CALLBACK (polyhymnia_window_mpd_queue_modified),
                    self);
}

/* Event handler & other utility functions implementation */
static void
polyhymnia_window_clear_queue_button_clicked (GtkButton* self,
                                              gpointer user_data)
{
  GError           *error = NULL;
  PolyhymniaWindow *window_self = user_data;

  g_assert (POLYHYMNIA_IS_WINDOW (window_self));

  polyhymnia_mpd_client_clear_queue (window_self->mpd_client, &error);

  if (error != NULL)
  {
    g_warning("Failed to clear queue: %s\n", error->message);
    g_error_free (error);
    error = NULL;
  }
}

static void
polyhymnia_window_add_tracks_to_queue_button_clicked (GtkButton* self,
                                                      gpointer user_data)
{
  GError            *error = NULL;
  GListModel        *track_list_model;
  GtkBitset         *selected_tracks;
  GtkSelectionModel *track_selection_model;
  PolyhymniaWindow  *window_self = user_data;

  g_assert (POLYHYMNIA_IS_WINDOW (window_self));

  track_list_model = G_LIST_MODEL (window_self->track_selection_model);
  track_selection_model = GTK_SELECTION_MODEL (window_self->track_selection_model);
  selected_tracks = gtk_selection_model_get_selection (track_selection_model);

  for (guint i = 0; i < gtk_bitset_get_size (selected_tracks); i++)
  {
    guint track_index = gtk_bitset_get_nth (selected_tracks, i);
    const PolyhymniaTrack *track = g_list_model_get_item (track_list_model,
                                                          track_index);
    const gchar *track_uri = polyhymnia_track_get_uri (track);
    polyhymnia_mpd_client_append_to_queue (window_self->mpd_client, track_uri,
                                           &error);
    if (error != NULL)
    {
      g_warning("Failed to add track %s into queue: %s\n", track_uri, error->message);
      g_error_free (error);
      error = NULL;
    }
  }

  gtk_bitset_unref (selected_tracks);
  gtk_selection_model_unselect_all (track_selection_model);
}

static void
polyhymnia_window_content_clear (PolyhymniaWindow *self)
{
  GListModel *genres_list = G_LIST_MODEL (self->genre_model);
  guint genres_count = g_list_model_get_n_items (genres_list);

  g_list_store_remove_all (self->artist_model);
  g_list_store_remove_all (self->album_model);
  g_list_store_remove_all (self->track_model);

  while (genres_count > 0)
  {
    gtk_string_list_remove (self->genre_model, genres_count - 1);
    genres_count = g_list_model_get_n_items (genres_list);
  }
}

static void
polyhymnia_window_content_init (PolyhymniaWindow *self)
{
  GError *error = NULL;

  GPtrArray *albums;
  GPtrArray *artists;
  GPtrArray *genres;
  GPtrArray *tracks;

  artists = polyhymnia_mpd_client_search_artists (self->mpd_client, &error);
  if (error != NULL)
  {
    g_object_set (G_OBJECT (self->artists_status_page),
                  "description", NULL,
                  "icon-name", "error-symbolic",
                  "title", _("Search for artists failed"),
                  NULL);
    adw_bin_set_child (self->artist_stack_page_content,
                       GTK_WIDGET (self->artists_status_page));
    g_warning("Search for artists failed: %s\n", error->message);
    g_error_free (error);
    error = NULL;
  }
  else if (artists->len == 0)
  {
    g_ptr_array_free (artists, FALSE);
    g_object_set (G_OBJECT (self->artists_status_page),
                  "description", _("If something is missing, try launching library scanning"),
                  "icon-name", "question-round-symbolic",
                  "title", _("No artists found"),
                  NULL);
    adw_bin_set_child (self->artist_stack_page_content,
                       GTK_WIDGET (self->artists_status_page));
  }
  else
  {
    for (int i = 0; i < artists->len; i++)
    {
      PolyhymniaArtist *artist = g_ptr_array_index(artists, i);
      g_list_store_append (self->artist_model, artist);
    }
    g_ptr_array_free (artists, TRUE);
    adw_bin_set_child (self->artist_stack_page_content,
                       GTK_WIDGET (self->artist_navigation_view));
  }

  albums = polyhymnia_mpd_client_search_albums (self->mpd_client, &error);
  if (error != NULL)
  {
    g_object_set (G_OBJECT (self->albums_status_page),
                  "description", NULL,
                  "icon-name", "error-symbolic",
                  "title", _("Search for albums failed"),
                  NULL);
    adw_bin_set_child (self->album_stack_page_content,
                       GTK_WIDGET (self->albums_status_page));
    g_warning("Search for albums failed: %s\n", error->message);
    g_error_free (error);
    error = NULL;
  }
  else if (albums->len == 0)
  {
    g_ptr_array_free (albums, FALSE);
    g_object_set (G_OBJECT (self->albums_status_page),
                  "description", _("If something is missing, try launching library scanning"),
                  "icon-name", "question-round-symbolic",
                  "title", _("No albums found"),
                  NULL);
    adw_bin_set_child (self->album_stack_page_content,
                       GTK_WIDGET (self->albums_status_page));
  }
  else
  {
    for (int i = 0; i < albums->len; i++)
    {
      PolyhymniaAlbum *album = g_ptr_array_index(albums, i);
      g_list_store_append (self->album_model, album);
    }
    g_ptr_array_free (albums, TRUE);
    adw_bin_set_child (self->album_stack_page_content,
                       GTK_WIDGET (self->album_navigation_view));
  }

  tracks = polyhymnia_mpd_client_search_tracks (self->mpd_client, "", &error);
  if (error != NULL)
  {
    g_object_set (G_OBJECT (self->tracks_status_page),
                  "description", NULL,
                  "icon-name", "error-symbolic",
                  "title", _("Search for songs failed"),
                  NULL);
    adw_bin_set_child (self->track_stack_page_content,
                       GTK_WIDGET (self->tracks_status_page));
    g_warning("Search for tracks failed: %s\n", error->message);
    g_error_free (error);
    error = NULL;
  }
  else if (tracks->len == 0)
  {
    g_ptr_array_free (tracks, FALSE);
    g_object_set (G_OBJECT (self->tracks_status_page),
                  "description", _("If something is missing, try launching library scanning"),
                  "icon-name", "question-round-symbolic",
                  "title", _("No songs found"),
                  NULL);
    adw_bin_set_child (self->track_stack_page_content,
                       GTK_WIDGET (self->tracks_status_page));
  }
  else
  {
    for (int i = 0; i < tracks->len; i++)
    {
      PolyhymniaTrack *track = g_ptr_array_index(tracks, i);
      g_list_store_append (self->track_model, track);
    }
    g_ptr_array_free (tracks, TRUE);
    adw_bin_set_child (self->track_stack_page_content,
                       GTK_WIDGET (self->track_navigation_view));
  }

  genres = polyhymnia_mpd_client_search_genres (self->mpd_client, &error);
  if (error != NULL)
  {
    g_object_set (G_OBJECT (self->genres_status_page),
                  "description", NULL,
                  "icon-name", "error-symbolic",
                  "title", _("Search for genres failed"),
                  NULL);
    adw_bin_set_child (self->genre_stack_page_content,
                       GTK_WIDGET (self->genres_status_page));
    g_warning("Search for genres failed: %s\n", error->message);
    g_error_free (error);
    error = NULL;
  }
  else if (genres->len == 0)
  {
    g_ptr_array_free (genres, FALSE);
    g_object_set (G_OBJECT (self->genres_status_page),
                  "description", _("If something is missing, try launching library scanning"),
                  "icon-name", "question-round-symbolic",
                  "title", _("No genres found"),
                  NULL);
    adw_bin_set_child (self->genre_stack_page_content,
                       GTK_WIDGET (self->genres_status_page));
  }
  else
  {
    for (int i = 0; i < genres->len; i++)
    {
      const char *genre = g_ptr_array_index(genres, i);
      gtk_string_list_append (self->genre_model, genre);
    }
    g_ptr_array_free (genres, TRUE);
    adw_bin_set_child (self->genre_stack_page_content,
                       GTK_WIDGET (self->genre_navigation_view));
  }
}

static void
polyhymnia_window_mpd_client_initialized(PolyhymniaMpdClient* self,
                                         GParamSpec* pspec,
                                         PolyhymniaWindow* user_data)
{
  gboolean mpd_initialized = FALSE;

  g_assert (POLYHYMNIA_IS_WINDOW (user_data));

  g_object_get (user_data->mpd_client, "initialized", &mpd_initialized, NULL);
  if (mpd_initialized)
  {
    adw_toast_overlay_set_child (user_data->root_toast_overlay,
                                 GTK_WIDGET (user_data->content));
    polyhymnia_window_content_init (user_data);
    polyhymnia_window_queue_pane_init (user_data);
  }
  else
  {
    adw_toast_overlay_set_child (user_data->root_toast_overlay,
                                 GTK_WIDGET (user_data->no_mpd_connection_page));
    g_list_store_remove_all (user_data->queue_model);
    polyhymnia_window_content_clear (user_data);
  }
}

static void
polyhymnia_window_mpd_database_updated(GObject* self, gpointer user_data)
{
  AdwToast *database_updated_toast = adw_toast_new (_("Library has been updated"));
  PolyhymniaWindow *window_self = user_data;

  g_assert (POLYHYMNIA_IS_WINDOW (window_self));

  adw_toast_overlay_add_toast (window_self->root_toast_overlay,
                               database_updated_toast);
  polyhymnia_window_content_clear (window_self);
  polyhymnia_window_content_init (window_self);
}

static void
polyhymnia_window_play_tracks_button_clicked (GtkButton* self,
                                              gpointer user_data)
{
  GError            *error = NULL;
  GListModel        *track_list_model;
  GtkBitset         *selected_tracks;
  GtkSelectionModel *track_selection_model;
  PolyhymniaWindow *window_self = user_data;

  g_assert (POLYHYMNIA_IS_WINDOW (window_self));

  polyhymnia_mpd_client_clear_queue (window_self->mpd_client, &error);
  if (error != NULL)
  {
    g_warning("Failed to clear queue: %s\n", error->message);
    g_error_free (error);
    error = NULL;
  }

  track_list_model = G_LIST_MODEL (window_self->track_selection_model);
  track_selection_model = GTK_SELECTION_MODEL (window_self->track_selection_model);
  selected_tracks = gtk_selection_model_get_selection (track_selection_model);

  for (guint i = 0; i < gtk_bitset_get_size (selected_tracks); i++)
  {
    guint track_index = gtk_bitset_get_nth (selected_tracks, i);
    const PolyhymniaTrack *track = g_list_model_get_item (track_list_model,
                                                          track_index);
    const gchar *track_uri = polyhymnia_track_get_uri (track);
    polyhymnia_mpd_client_append_to_queue (window_self->mpd_client, track_uri,
                                           &error);
    if (error != NULL)
    {
      g_warning("Failed to add track %s into queue: %s\n", track_uri, error->message);
      g_error_free (error);
      error = NULL;
    }
  }

  polyhymnia_mpd_client_play (window_self->mpd_client, &error);
  if (error != NULL)
  {
    g_warning("Failed to start playback: %s\n", error->message);
    g_error_free (error);
    error = NULL;
  }

  gtk_selection_model_unselect_all (GTK_SELECTION_MODEL (window_self->track_selection_model));
}

static void
polyhymnia_window_queue_to_playlist_button_clicked (GtkButton* self,
                                                    gpointer user_data)
{

}

static void
polyhymnia_window_mpd_queue_modified(GObject* self, gpointer user_data)
{
  PolyhymniaWindow *window_self = user_data;

  g_assert (POLYHYMNIA_IS_WINDOW (window_self));

  g_list_store_remove_all (window_self->queue_model);
  polyhymnia_window_queue_pane_init (window_self);
}

static void
polyhymnia_window_player_current_track(PolyhymniaPlayer* self,
                                       GParamSpec* pspec,
                                       PolyhymniaWindow* user_data)
{
  const PolyhymniaTrack *current_track;

  g_assert (POLYHYMNIA_IS_WINDOW (user_data));

  current_track = polyhymnia_player_get_current_track (self);
  if (current_track == NULL)
  {
    gtk_label_set_text (user_data->current_track_artist_label, NULL);
    gtk_label_set_text (user_data->current_track_title_label, NULL);
  }
  else
  {
    const gchar *artist = polyhymnia_track_get_artist (current_track);
    const gchar *title = polyhymnia_track_get_title (current_track);

    gtk_label_set_text (user_data->current_track_artist_label, artist);
    gtk_label_set_text (user_data->current_track_title_label, title);
  }
}

static void
polyhymnia_window_player_state(PolyhymniaPlayer* self,
                               GParamSpec* pspec,
                               PolyhymniaWindow* user_data)
{
  const char *icon_name;
  PolyhymniaPlayerPlaybackStatus player_state;

  g_assert (POLYHYMNIA_IS_WINDOW (user_data));

  player_state = polyhymnia_player_get_playback_status (self);
  icon_name = polyhymnia_window_player_state_to_icon (player_state);
  gtk_button_set_icon_name (user_data->play_button, icon_name);
}

static const gchar *
polyhymnia_window_player_state_to_icon(PolyhymniaPlayerPlaybackStatus state)
{
  switch (state)
  {
  case POLYHYMNIA_PLAYER_PLAYBACK_STATUS_PAUSED:
    return "pause-large-symbolic";
  case POLYHYMNIA_PLAYER_PLAYBACK_STATUS_UNKNOWN:
    return "play-large-disabled-symbolic";
  default:
    return "play-large-disabled-symbolic";
  }
}

static void
polyhymnia_window_queue_pane_init (PolyhymniaWindow *self)
{
  GError *error = NULL;
  GPtrArray *queue = polyhymnia_mpd_client_get_queue (self->mpd_client,
                                                      &error);
  if (error != NULL)
  {
    g_object_set (G_OBJECT (self->queue_status_page),
                  "description", _("Failed to fetch queue"),
                  NULL);
    gtk_widget_set_visible (GTK_WIDGET (self->queue_to_playlist_button), FALSE);
    gtk_action_bar_set_revealed (self->queue_action_bar, FALSE);
    gtk_scrolled_window_set_child (self->queue_page_content,
                                   GTK_WIDGET (self->queue_status_page));
    g_warning("Queue fetch failed: %s\n", error->message);
    g_error_free (error);
    error = NULL;
  }
  else if (queue->len == 0)
  {
    g_ptr_array_free (queue, FALSE);
    g_object_set (G_OBJECT (self->queue_status_page),
                  "description", _("Queue is empty"),
                  NULL);
    gtk_widget_set_visible (GTK_WIDGET (self->queue_to_playlist_button), FALSE);
    gtk_action_bar_set_revealed (self->queue_action_bar, FALSE);
    gtk_scrolled_window_set_child (self->queue_page_content,
                                   GTK_WIDGET (self->queue_status_page));
  }
  else
  {
    for (int i = 0; i < queue->len; i++)
    {
      PolyhymniaTrack *queue_track = g_ptr_array_index(queue, i);
      g_list_store_append (self->queue_model, queue_track);
    }
    g_ptr_array_free (queue, TRUE);
    gtk_scrolled_window_set_child (self->queue_page_content,
                                   GTK_WIDGET (self->queue_list_view));
    gtk_widget_set_visible (GTK_WIDGET (self->queue_to_playlist_button), TRUE);
    gtk_action_bar_set_revealed (self->queue_action_bar, TRUE);
  }
}

static void
polyhymnia_window_track_selection_changed (GtkSelectionModel* self,
                                           guint position,
                                           guint n_items,
                                           gpointer user_data)
{
  GtkBitset *selection = gtk_selection_model_get_selection (self);
  PolyhymniaWindow *window_self = user_data;

  g_assert (POLYHYMNIA_IS_WINDOW (window_self));

  adw_toolbar_view_set_reveal_top_bars (window_self->track_toolbar_view,
                                        !gtk_bitset_is_empty (selection));
  gtk_bitset_unref (selection);
}
