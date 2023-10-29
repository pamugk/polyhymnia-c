
#include "config.h"

#include "polyhymnia-mpd-client.h"
#include "polyhymnia-window.h"

#include "polyhymnia-genre-card.h"

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
  GtkStackPage        *artists_stack_page;
  GtkScrolledWindow   *artists_scrolled_window;
  GtkStackPage        *albums_stack_page;
  GtkScrolledWindow   *albums_scrolled_window;
  GtkStackPage        *tracks_stack_page;
  GtkScrolledWindow   *tracks_scrolled_window;
  GtkStackPage        *genres_stack_page;
  GtkScrolledWindow   *genres_scrolled_window;
  AdwStatusPage       *no_library_connection_page;

  GtkActionBar        *player_bar;
  GtkButton           *previous_track_button;
  GtkButton           *playback_button;
  GtkButton           *next_track_button;
  GtkScale            *track_position_scale;
  GtkScale            *volume_scale;
};

G_DEFINE_FINAL_TYPE (PolyhymniaWindow, polyhymnia_window, ADW_TYPE_APPLICATION_WINDOW)

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

  gobject_class->finalize = polyhymnia_window_finalize;

  gtk_widget_class_set_template_from_resource (widget_class, "/com/github/pamugk/polyhymnia/ui/polyhymnia-window.ui");

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, scan_button);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, search_button);

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, library_stack_sidebar);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, library_stack);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, recent_stack_page);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, artists_stack_page);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, albums_stack_page);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, tracks_stack_page);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, genres_stack_page);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, no_library_connection_page);

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, player_bar);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, previous_track_button);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, playback_button);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, next_track_button);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, track_position_scale);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, volume_scale);
}

static void
polyhymnia_window_content_init (PolyhymniaWindow *self)
{
  gboolean mpd_initialized = FALSE;
  GError *error = NULL;

  GArray *albums;
  GArray *artists;
  GArray *genres;

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
    if (artists != NULL)
    {
      for (guint i = 0; i < artists->len; i++)
      {
        gchar * artist = g_array_index (artists, gchar *, i);
        g_free(artist);
      }
      g_array_free (artists, TRUE);
    }
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
    if (albums != NULL)
    {
      for (guint i = 0; i < albums->len; i++)
      {
        gchar *album = g_array_index (albums, gchar *, i);
        g_free(album);
      }
      g_array_free (albums, TRUE);
    }
  }

  polyhymnia_mpd_client_search_tracks (self->mpd_client, "", &error);
  if (error != NULL)
  {
    g_warning("Search for tracks failed: %s\n",
              error->message);
    g_error_free (error);
    error = NULL;
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
    if (genres != NULL)
    {
      for (guint i = 0; i < genres->len; i++)
      {
        gchar *genre = g_array_index (genres, gchar *, i);
        g_free(genre);
      }
      g_array_free (genres, TRUE);
    }
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
