
#include "config.h"

#include "polyhymnia-window.h"

#include "polyhymnia-album-page.h"
#include "polyhymnia-artists-page.h"
#include "polyhymnia-mpd-client-api.h"
#include "polyhymnia-player-bar.h"
#include "polyhymnia-queue-pane.h"
#include "polyhymnia-tracks-page.h"

#define _(x) g_dgettext (GETTEXT_PACKAGE, x)

/* Type metadata */
struct _PolyhymniaWindow
{
  AdwApplicationWindow     parent_instance;

  /* Template widgets */
  AdwOverlaySplitView      *content;
  AdwStatusPage            *no_mpd_connection_page;
  AdwToastOverlay          *root_toast_overlay;
  PolyhymniaPlayerBar      *player_bar;

  AdwBin                   *album_stack_page_content;
  AdwNavigationView        *album_navigation_view;
  AdwStatusPage            *albums_status_page;

  AdwBin                   *genre_stack_page_content;
  AdwNavigationView        *genre_navigation_view;
  AdwStatusPage            *genres_status_page;

  /* Template objects */
  PolyhymniaMpdClient      *mpd_client;
  GSettings                *settings;

  GListStore               *album_model;
  GtkNoSelection           *album_selection_model;
  GtkStringList            *genre_model;
  GtkNoSelection           *genre_selection_model;
};

G_DEFINE_FINAL_TYPE (PolyhymniaWindow, polyhymnia_window, ADW_TYPE_APPLICATION_WINDOW)

/* Event handler & other utility functions declaration */
static void
polyhymnia_window_album_clicked (PolyhymniaWindow *self,
                                 guint            position,
                                 GtkGridView      *user_data);

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

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, album_stack_page_content);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, album_navigation_view);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, albums_status_page);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, genre_stack_page_content);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, genre_navigation_view);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, genres_status_page);

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, mpd_client);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, settings);

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, album_selection_model);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, genre_model);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, genre_selection_model);

  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_window_album_clicked);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_window_mpd_database_updated);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_window_mpd_client_initialized);
}

static void
polyhymnia_window_init (PolyhymniaWindow *self)
{
  self->album_model = g_list_store_new (POLYHYMNIA_TYPE_ALBUM);

  g_type_ensure (POLYHYMNIA_TYPE_ARTISTS_PAGE);
  g_type_ensure (POLYHYMNIA_TYPE_PLAYER_BAR);
  g_type_ensure (POLYHYMNIA_TYPE_QUEUE_PANE);
  g_type_ensure (POLYHYMNIA_TYPE_TRACKS_PAGE);

  gtk_widget_init_template (GTK_WIDGET (self));

  g_object_bind_property (polyhymnia_player_bar_get_queue_toggle_button (self->player_bar),
                          "active",
                          self->content, "show-sidebar",
                          G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);

  gtk_no_selection_set_model (self->album_selection_model,
                              G_LIST_MODEL (self->album_model));

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
polyhymnia_window_album_clicked (PolyhymniaWindow *self,
                                 guint            position,
                                 GtkGridView      *user_data)
{
  const PolyhymniaAlbum *album;
  PolyhymniaAlbumPage *album_page;

  g_assert (POLYHYMNIA_IS_WINDOW (self));

  album = g_list_model_get_item (G_LIST_MODEL (self->album_model), position);
  album_page = g_object_new (POLYHYMNIA_TYPE_ALBUM_PAGE,
                             "album-title", polyhymnia_album_get_title (album),
                             NULL);
  adw_navigation_view_push (self->album_navigation_view,
                            ADW_NAVIGATION_PAGE (album_page));
}

static void
polyhymnia_window_content_clear (PolyhymniaWindow *self)
{
  GListModel *genres_list = G_LIST_MODEL (self->genre_model);
  guint genres_count = g_list_model_get_n_items (genres_list);

  g_list_store_remove_all (self->album_model);

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
  GPtrArray *genres;

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
  }
  else
  {
    adw_toast_overlay_set_child (self->root_toast_overlay,
                                 GTK_WIDGET (self->no_mpd_connection_page));
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
