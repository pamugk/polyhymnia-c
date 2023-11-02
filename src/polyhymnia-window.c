
#include "config.h"

#include "polyhymnia-mpd-client.h"
#include "polyhymnia-window.h"

struct _PolyhymniaWindow
{
  AdwApplicationWindow  parent_instance;

  /* Auxiliary objects  */
  PolyhymniaMpdClient *mpd_client;
  GSettings           *settings;

  /* Template widgets */
  GtkButton           *scan_button;
  GtkButton           *search_button;

  GtkStackSidebar     *library_stack_sidebar;
  GtkStack            *library_stack;
  GtkStackPage        *recent_stack_page;
  GtkStackPage        *artist_stack_page;
  GtkGridView         *artist_grid_view;
  GtkStackPage        *album_stack_page;
  GtkGridView         *album_grid_view;
  GtkStackPage        *track_stack_page;
  GtkListView         *track_list_view;
  GtkStackPage        *genre_stack_page;
  GtkGridView         *genre_grid_view;
  AdwStatusPage       *no_library_connection_page;

  GtkActionBar        *player_bar;
  GtkButton           *previous_track_button;
  GtkButton           *playback_button;
  GtkButton           *next_track_button;
  GtkScale            *track_position_scale;
  GtkScale            *volume_scale;

  /* Template models */
  GListStore          *artist_model;
  GtkSingleSelection  *artist_selection_model;
  GtkStringList       *genre_model;
  GtkSingleSelection  *genre_selection_model;
};

G_DEFINE_FINAL_TYPE (PolyhymniaWindow, polyhymnia_window, ADW_TYPE_APPLICATION_WINDOW)

static void
polyhymnia_window_dispose(GObject *gobject)
{
  PolyhymniaWindow *self = POLYHYMNIA_WINDOW (gobject);

  gtk_widget_dispose_template (GTK_WIDGET (self), POLYHYMNIA_TYPE_WINDOW);

  G_OBJECT_CLASS (polyhymnia_window_parent_class)->dispose (gobject);
}

static void
polyhymnia_window_finalize(GObject *gobject)
{
  PolyhymniaWindow *self = POLYHYMNIA_WINDOW (gobject);

  g_clear_object (&self->mpd_client);
  g_clear_object (&self->settings);

  G_OBJECT_CLASS (polyhymnia_window_parent_class)->finalize (gobject);
}

static void
polyhymnia_window_class_init (PolyhymniaWindowClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->dispose = polyhymnia_window_dispose;
  gobject_class->finalize = polyhymnia_window_finalize;

  gtk_widget_class_set_template_from_resource (widget_class, "/com/github/pamugk/polyhymnia/ui/polyhymnia-window.ui");

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, scan_button);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, search_button);

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, library_stack_sidebar);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, library_stack);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, recent_stack_page);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, artist_stack_page);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, artist_grid_view);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, album_stack_page);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, album_grid_view);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, track_stack_page);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, track_list_view);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, genre_stack_page);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, genre_grid_view);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, no_library_connection_page);

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, player_bar);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, previous_track_button);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, playback_button);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, next_track_button);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, track_position_scale);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, volume_scale);

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, artist_selection_model);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, genre_model);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, genre_selection_model);
}

static void
polyhymnia_window_content_init (PolyhymniaWindow *self)
{
  gboolean mpd_initialized = FALSE;
  GError *error = NULL;

  GPtrArray *albums;
  GPtrArray *artists;
  GPtrArray *genres;
  GPtrArray *tracks;

  g_object_get (self->mpd_client, "initialized", &mpd_initialized, NULL);
  if (!mpd_initialized)
  {
    return;
  }

  artists = polyhymnia_mpd_client_search_artists (self->mpd_client, &error);
  if (error != NULL)
  {
    g_warning("Search for artists failed: %s\n",
              error->message);
    g_error_free (error);
    error = NULL;
  }
  else
  {
    for (int i = 0; i < artists->len; i++)
    {
      PolyhymniaArtist *artist = g_ptr_array_index(artists, i);
      g_list_store_append (self->artist_model, artist);
    }
    g_ptr_array_free (artists, TRUE);
  }

  albums = polyhymnia_mpd_client_search_albums (self->mpd_client, &error);
  if (error != NULL)
  {
    g_warning("Search for albums failed: %s\n",
              error->message);
    g_error_free (error);
    error = NULL;
  }
  else
  {
    g_ptr_array_free (albums, TRUE);
  }

  tracks = polyhymnia_mpd_client_search_tracks (self->mpd_client, "", &error);
  if (error != NULL)
  {
    g_warning("Search for tracks failed: %s\n",
              error->message);
    g_error_free (error);
    error = NULL;
  }
  else
  {
    g_ptr_array_free (tracks, TRUE);
  }

  genres = polyhymnia_mpd_client_search_genres (self->mpd_client, &error);
  if (error != NULL)
  {
    g_warning("Search for genres failed: %s\n",
              error->message);
    g_error_free (error);
    error = NULL;
  }
  else
  {
    for (int i = 0; i < genres->len; i++)
    {
      const char *genre = g_ptr_array_index(genres, i);
      gtk_string_list_append (self->genre_model, genre);
    }
    g_ptr_array_free (genres, TRUE);
  }
}

static void
polyhymnia_window_init (PolyhymniaWindow *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  self->mpd_client = g_object_new (POLYHYMNIA_TYPE_MPD_CLIENT, NULL);
  g_object_bind_property(self->mpd_client, "initialized",
                         self->scan_button, "visible",
                         G_BINDING_DEFAULT | G_BINDING_SYNC_CREATE);
  g_object_bind_property(self->mpd_client, "initialized",
                         self->search_button, "visible",
                         G_BINDING_DEFAULT | G_BINDING_SYNC_CREATE);

  g_object_bind_property(self->mpd_client, "initialized",
                         self->library_stack_sidebar, "visible",
                         G_BINDING_DEFAULT | G_BINDING_SYNC_CREATE);
  g_object_bind_property(self->mpd_client, "initialized",
                         self->library_stack, "hexpand",
                         G_BINDING_DEFAULT | G_BINDING_SYNC_CREATE);
  g_object_bind_property(self->mpd_client, "initialized",
                         self->library_stack, "visible",
                         G_BINDING_DEFAULT | G_BINDING_SYNC_CREATE);

  g_object_bind_property(self->mpd_client, "initialized",
                         self->no_library_connection_page, "hexpand",
                         G_BINDING_DEFAULT | G_BINDING_SYNC_CREATE| G_BINDING_INVERT_BOOLEAN);
  g_object_bind_property(self->mpd_client, "initialized",
                         self->no_library_connection_page, "visible",
                         G_BINDING_DEFAULT | G_BINDING_SYNC_CREATE | G_BINDING_INVERT_BOOLEAN);

  g_object_bind_property(self->mpd_client, "initialized",
                         self->player_bar, "visible",
                         G_BINDING_DEFAULT | G_BINDING_SYNC_CREATE);

  self->artist_model = g_list_store_new (POLYHYMNIA_TYPE_ARTIST);
  gtk_single_selection_set_model (self->artist_selection_model,
                                  G_LIST_MODEL (self->artist_model));

  self->settings = g_settings_new ("com.github.pamugk.polyhymnia");

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

  polyhymnia_window_content_init(self);
}
