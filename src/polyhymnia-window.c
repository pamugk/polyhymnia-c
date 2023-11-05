
#include "config.h"

#include "polyhymnia-mpd-client.h"
#include "polyhymnia-window.h"

struct _PolyhymniaWindow
{
  AdwApplicationWindow  parent_instance;

  /* Template widgets */
  AdwViewStack        *content_stack;
  AdwStatusPage       *no_mpd_connection_page;
  AdwToolbarView      *root_toolbar_view;

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
polyhymnia_window_class_init (PolyhymniaWindowClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->dispose = polyhymnia_window_dispose;

  gtk_widget_class_set_template_from_resource (widget_class, "/com/github/pamugk/polyhymnia/ui/polyhymnia-window.ui");

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, content_stack);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, no_mpd_connection_page);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, root_toolbar_view);

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, mpd_client);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, settings);

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, album_selection_model);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, artist_selection_model);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, genre_model);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, genre_selection_model);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, track_selection_model);
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
    for (int i = 0; i < albums->len; i++)
    {
      PolyhymniaAlbum *album = g_ptr_array_index(albums, i);
      g_list_store_append (self->album_model, album);
    }
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
    for (int i = 0; i < tracks->len; i++)
    {
      PolyhymniaTrack *track = g_ptr_array_index(tracks, i);
      g_list_store_append (self->track_model, track);
    }
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
polyhymnia_window_mpd_initialized(GObject* self,
                                  GParamSpec* pspec,
                                  gpointer user_data)
{
  gboolean mpd_initialized = FALSE;
  PolyhymniaWindow *window_self = user_data;

  g_assert (POLYHYMNIA_IS_WINDOW (window_self));

  g_object_get (window_self->mpd_client, "initialized", &mpd_initialized, NULL);
  if (mpd_initialized)
  {
    adw_toolbar_view_set_content (window_self->root_toolbar_view,
                                  GTK_WIDGET (window_self->content_stack));
    polyhymnia_window_content_init (window_self);
  }
  else
  {
    adw_toolbar_view_set_content (window_self->root_toolbar_view,
                                  GTK_WIDGET (window_self->no_mpd_connection_page));
  }
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

  polyhymnia_window_mpd_initialized (G_OBJECT(self->mpd_client), NULL, self);
  g_signal_connect (self->mpd_client, "notify::initialized",
                    G_CALLBACK (polyhymnia_window_mpd_initialized),
                    self);
}
