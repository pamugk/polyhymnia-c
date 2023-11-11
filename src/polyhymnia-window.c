
#include "config.h"

#include "polyhymnia-mpd-client-api.h"
#include "polyhymnia-player-bar.h"
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
  PolyhymniaPlayerBar *player_bar;

  AdwBin              *artist_stack_page_content;
  AdwNavigationView   *artist_navigation_view;
  AdwStatusPage       *artists_status_page;

  AdwBin              *album_stack_page_content;
  AdwNavigationView   *album_navigation_view;
  AdwStatusPage       *albums_status_page;

  AdwBin              *track_stack_page_content;
  AdwNavigationView   *track_navigation_view;
  AdwToolbarView      *track_toolbar_view;
  AdwStatusPage       *tracks_status_page;

  AdwBin              *genre_stack_page_content;
  AdwNavigationView   *genre_navigation_view;
  AdwStatusPage       *genres_status_page;

  AdwToolbarView      *queue_toolbar_view;
  GtkListView         *queue_list_view;
  GtkScrolledWindow   *queue_page_content;
  AdwStatusPage       *queue_status_page;

  /* Template objects */
  PolyhymniaMpdClient *mpd_client;
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
polyhymnia_window_add_tracks_to_queue_button_clicked (PolyhymniaWindow *self,
                                                      GtkButton        *user_data);

static void
polyhymnia_window_clear_queue_button_clicked (PolyhymniaWindow *self,
                                              GtkButton        *user_data);

static void
polyhymnia_window_content_clear (PolyhymniaWindow *self);

static void
polyhymnia_window_content_init (PolyhymniaWindow *self);

static void
polyhymnia_window_mpd_client_initialized (PolyhymniaWindow    *self,
                                          GParamSpec          *pspec,
                                          PolyhymniaMpdClient *user_data);

static void
polyhymnia_window_mpd_database_updated (PolyhymniaWindow    *self,
                                        PolyhymniaMpdClient *user_data);

static void
polyhymnia_window_mpd_queue_modified (PolyhymniaWindow    *self,
                                      PolyhymniaMpdClient *user_data);

static void
polyhymnia_window_play_tracks_button_clicked (PolyhymniaWindow *self,
                                              GtkButton        *user_data);

static void
polyhymnia_window_queue_to_playlist_button_clicked (PolyhymniaWindow *self,
                                                    GtkButton        *user_data);

static void
polyhymnia_window_queue_pane_init (PolyhymniaWindow *self);

static void
polyhymnia_window_track_selection_changed (PolyhymniaWindow  *self,
                                           guint             position,
                                           guint             n_items,
                                           GtkSelectionModel *user_data);

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

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/com/github/pamugk/polyhymnia/ui/polyhymnia-window.ui");

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, content);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, no_mpd_connection_page);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, root_toast_overlay);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, player_bar);

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

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, queue_list_view);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, queue_page_content);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, queue_toolbar_view);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, queue_status_page);

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, mpd_client);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, settings);

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, album_selection_model);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, artist_selection_model);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, genre_model);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, genre_selection_model);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, track_selection_model);

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, queue_selection_model);

  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_window_add_tracks_to_queue_button_clicked);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_window_clear_queue_button_clicked);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_window_mpd_database_updated);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_window_mpd_client_initialized);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_window_mpd_queue_modified);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_window_play_tracks_button_clicked);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_window_queue_to_playlist_button_clicked);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_window_track_selection_changed);
}

static void
polyhymnia_window_init (PolyhymniaWindow *self)
{
  g_type_ensure (POLYHYMNIA_TYPE_PLAYER_BAR);

  gtk_widget_init_template (GTK_WIDGET (self));

  g_object_bind_property (polyhymnia_player_bar_get_queue_toggle_button (self->player_bar),
                          "active",
                          self->content, "show-sidebar",
                          G_BINDING_SYNC_CREATE);

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

  polyhymnia_window_mpd_client_initialized (self, NULL, self->mpd_client);
}

/* Event handler & other utility functions implementation */
static void
polyhymnia_window_clear_queue_button_clicked (PolyhymniaWindow *self,
                                              GtkButton        *user_data)
{
  GError           *error = NULL;

  g_assert (POLYHYMNIA_IS_WINDOW (self));

  polyhymnia_mpd_client_clear_queue (self->mpd_client, &error);

  if (error != NULL)
  {
    g_warning("Failed to clear queue: %s\n", error->message);
    g_error_free (error);
    error = NULL;
  }
}

static void
polyhymnia_window_add_tracks_to_queue_button_clicked (PolyhymniaWindow *self,
                                                      GtkButton        *user_data)
{
  GError            *error = NULL;
  GListModel        *track_list_model;
  GtkBitset         *selected_tracks;
  GtkSelectionModel *track_selection_model;

  g_assert (POLYHYMNIA_IS_WINDOW (self));

  track_list_model = G_LIST_MODEL (self->track_selection_model);
  track_selection_model = GTK_SELECTION_MODEL (self->track_selection_model);
  selected_tracks = gtk_selection_model_get_selection (track_selection_model);

  for (guint i = 0; i < gtk_bitset_get_size (selected_tracks); i++)
  {
    guint track_index = gtk_bitset_get_nth (selected_tracks, i);
    const PolyhymniaTrack *track = g_list_model_get_item (track_list_model,
                                                          track_index);
    const gchar *track_uri = polyhymnia_track_get_uri (track);
    polyhymnia_mpd_client_append_to_queue (self->mpd_client, track_uri,
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
polyhymnia_window_mpd_client_initialized (PolyhymniaWindow    *self,
                                          GParamSpec          *pspec,
                                          PolyhymniaMpdClient *user_data)
{
  g_assert (POLYHYMNIA_IS_WINDOW (self));

  if (polyhymnia_mpd_client_is_initialized (user_data))
  {
    adw_toast_overlay_set_child (self->root_toast_overlay,
                                 GTK_WIDGET (self->content));
    polyhymnia_window_content_init (self);
    polyhymnia_window_queue_pane_init (self);
  }
  else
  {
    adw_toast_overlay_set_child (self->root_toast_overlay,
                                 GTK_WIDGET (self->no_mpd_connection_page));
    g_list_store_remove_all (self->queue_model);
    polyhymnia_window_content_clear (self);
  }
}

static void
polyhymnia_window_mpd_database_updated (PolyhymniaWindow    *self,
                                        PolyhymniaMpdClient *user_data)
{
  AdwToast *database_updated_toast = adw_toast_new (_("Library has been updated"));

  g_assert (POLYHYMNIA_IS_WINDOW (self));

  adw_toast_overlay_add_toast (self->root_toast_overlay,
                               database_updated_toast);
  polyhymnia_window_content_clear (self);
  polyhymnia_window_content_init (self);
}

static void
polyhymnia_window_play_tracks_button_clicked (PolyhymniaWindow *self,
                                              GtkButton        *user_data)
{
  GError            *error = NULL;
  GListModel        *track_list_model;
  GtkBitset         *selected_tracks;
  GtkSelectionModel *track_selection_model;

  g_assert (POLYHYMNIA_IS_WINDOW (self));

  polyhymnia_mpd_client_clear_queue (self->mpd_client, &error);
  if (error != NULL)
  {
    g_warning("Failed to clear queue: %s\n", error->message);
    g_error_free (error);
    error = NULL;
  }

  track_list_model = G_LIST_MODEL (self->track_selection_model);
  track_selection_model = GTK_SELECTION_MODEL (self->track_selection_model);
  selected_tracks = gtk_selection_model_get_selection (track_selection_model);

  for (guint i = 0; i < gtk_bitset_get_size (selected_tracks); i++)
  {
    guint track_index = gtk_bitset_get_nth (selected_tracks, i);
    const PolyhymniaTrack *track = g_list_model_get_item (track_list_model,
                                                          track_index);
    const gchar *track_uri = polyhymnia_track_get_uri (track);
    polyhymnia_mpd_client_append_to_queue (self->mpd_client, track_uri,
                                           &error);
    if (error != NULL)
    {
      g_warning("Failed to add track %s into queue: %s\n", track_uri, error->message);
      g_error_free (error);
      error = NULL;
    }
  }

  polyhymnia_mpd_client_play (self->mpd_client, &error);
  if (error != NULL)
  {
    g_warning("Failed to start playback: %s\n", error->message);
    g_error_free (error);
    error = NULL;
  }

  gtk_selection_model_unselect_all (GTK_SELECTION_MODEL (self->track_selection_model));
}

static void
polyhymnia_window_queue_to_playlist_button_clicked (PolyhymniaWindow *self,
                                                    GtkButton        *user_data)
{

}

static void
polyhymnia_window_mpd_queue_modified (PolyhymniaWindow    *self,
                                      PolyhymniaMpdClient *user_data)
{
  g_assert (POLYHYMNIA_IS_WINDOW (self));

  g_list_store_remove_all (self->queue_model);
  polyhymnia_window_queue_pane_init (self);
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
    adw_toolbar_view_set_reveal_bottom_bars (self->queue_toolbar_view, FALSE);
    adw_toolbar_view_set_reveal_top_bars (self->queue_toolbar_view, FALSE);
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
    adw_toolbar_view_set_reveal_bottom_bars (self->queue_toolbar_view, FALSE);
    adw_toolbar_view_set_reveal_top_bars (self->queue_toolbar_view, FALSE);
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
    adw_toolbar_view_set_reveal_bottom_bars (self->queue_toolbar_view, TRUE);
    adw_toolbar_view_set_reveal_top_bars (self->queue_toolbar_view, TRUE);
  }
}

static void
polyhymnia_window_track_selection_changed (PolyhymniaWindow  *self,
                                           guint             position,
                                           guint             n_items,
                                           GtkSelectionModel *user_data)
{
  GtkBitset *selection = gtk_selection_model_get_selection (user_data);

  g_assert (POLYHYMNIA_IS_WINDOW (self));

  adw_toolbar_view_set_reveal_top_bars (self->track_toolbar_view,
                                        !gtk_bitset_is_empty (selection));
  gtk_bitset_unref (selection);
}
