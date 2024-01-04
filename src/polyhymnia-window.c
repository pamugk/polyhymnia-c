
#include "config.h"

#include "polyhymnia-window.h"

#include "polyhymnia-album-page.h"
#include "polyhymnia-albums-page.h"
#include "polyhymnia-artist-page.h"
#include "polyhymnia-artists-page.h"
#include "polyhymnia-last-modified-page.h"
#include "polyhymnia-mpd-client-api.h"
#include "polyhymnia-player-bar.h"
#include "polyhymnia-playlist-page.h"
#include "polyhymnia-playlists-page.h"
#include "polyhymnia-queue-pane.h"
#include "polyhymnia-search-page.h"
#include "polyhymnia-tracks-page.h"

#define _(x) g_dgettext (GETTEXT_PACKAGE, x)

/* Type metadata */
struct _PolyhymniaWindow
{
  AdwApplicationWindow parent_instance;

  /* UI state */
  GtkListBoxRow        *last_selected_sidebar_row;

  /* Template widgets */
  AdwOverlaySplitView  *content;
  AdwStatusPage        *no_mpd_connection_page;
  AdwToastOverlay      *root_toast_overlay;
  GtkSearchBar         *search_bar;
  GtkSearchEntry       *search_entry;
  GtkListBox           *sidebar_box;
  AdwNavigationView    *library_navigation_view;
  PolyhymniaPlayerBar  *player_bar;

  GtkListBoxRow        *artists_sidebar_row;
  GtkListBoxRow        *last_modified_sidebar_row;
  GtkListBoxRow        *albums_sidebar_row;
  GtkListBoxRow        *tracks_sidebar_row;
  GtkListBoxRow        *playlists_sidebar_row;

  AdwNavigationPage    *last_modified_page;
  AdwNavigationPage    *artists_page;
  AdwNavigationPage    *albums_page;
  AdwNavigationPage    *tracks_page;
  AdwNavigationPage    *playlists_page;
  AdwNavigationPage    *search_page;

  /* Template objects */
  PolyhymniaMpdClient  *mpd_client;
  GSettings            *settings;
};

G_DEFINE_FINAL_TYPE (PolyhymniaWindow, polyhymnia_window, ADW_TYPE_APPLICATION_WINDOW)

/* Event handler & other utility functions declaration */
static void
polyhymnia_window_album_deleted (PolyhymniaWindow    *self,
                                 PolyhymniaAlbumPage *album_page);

static void
polyhymnia_window_mpd_client_initialized (PolyhymniaWindow    *self,
                                          GParamSpec          *pspec,
                                          PolyhymniaMpdClient *user_data);

static void
polyhymnia_window_mpd_database_updated (PolyhymniaWindow    *self,
                                        PolyhymniaMpdClient *user_data);

static void
polyhymnia_window_mpd_playlists_changed (PolyhymniaWindow    *self,
                                         PolyhymniaMpdClient *user_data);

static void
polyhymnia_window_navigate_album (PolyhymniaWindow     *self,
                                  const gchar          *album_title,
                                  PolyhymniaAlbumsPage *user_data);

static void
polyhymnia_window_navigate_artist (PolyhymniaWindow      *self,
                                   const gchar           *artist_name,
                                   PolyhymniaArtistsPage *user_data);

static void
polyhymnia_window_navigate_playlist (PolyhymniaWindow        *self,
                                     const gchar             *playlist_title,
                                     PolyhymniaPlaylistsPage *user_data);

static void
polyhymnia_window_playlist_deleted (PolyhymniaWindow        *self,
                                    PolyhymniaPlaylistPage  *playlist_page);

static void
polyhymnia_window_scan_button_clicked (PolyhymniaWindow *self,
                                       AdwSplitButton   *user_data);

static void
polyhymnia_window_search_changed (PolyhymniaWindow *self,
                                  GtkSearchEntry   *user_data);

static void
polyhymnia_window_search_started (PolyhymniaWindow *self,
                                  GtkSearchEntry   *user_data);

static void
polyhymnia_window_search_stopped (PolyhymniaWindow *self,
                                  GtkSearchEntry   *user_data);

static void
polyhymnia_window_track_show_details (PolyhymniaWindow *self,
                                      const gchar      *track_uri,
                                      GObject          *user_data);

static void
polyhymnia_window_sidebar_row_selected (PolyhymniaWindow    *self,
                                        GtkListBoxRow       *selected_row,
                                        GtkListBox          *sidebar);

/* Class stuff */
static void
polyhymnia_window_dispose(GObject *gobject)
{
  PolyhymniaWindow *self = POLYHYMNIA_WINDOW (gobject);

  self->last_selected_sidebar_row = NULL;
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
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, search_bar);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, search_entry);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, sidebar_box);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, library_navigation_view);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, player_bar);

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, last_modified_sidebar_row);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, artists_sidebar_row);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, albums_sidebar_row);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, tracks_sidebar_row);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, playlists_sidebar_row);

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, last_modified_page);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, artists_page);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, albums_page);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, tracks_page);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, playlists_page);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, search_page);

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, mpd_client);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, settings);

  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_window_mpd_client_initialized);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_window_mpd_database_updated);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_window_mpd_playlists_changed);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_window_navigate_album);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_window_navigate_artist);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_window_navigate_playlist);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_window_scan_button_clicked);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_window_search_changed);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_window_search_started);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_window_search_stopped);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_window_track_show_details);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_window_sidebar_row_selected);
}

static void
polyhymnia_window_init (PolyhymniaWindow *self)
{
  g_type_ensure (POLYHYMNIA_TYPE_ALBUMS_PAGE);
  g_type_ensure (POLYHYMNIA_TYPE_ARTISTS_PAGE);
  g_type_ensure (POLYHYMNIA_TYPE_LAST_MODIFIED_PAGE);
  g_type_ensure (POLYHYMNIA_TYPE_PLAYER_BAR);
  g_type_ensure (POLYHYMNIA_TYPE_PLAYLISTS_PAGE);
  g_type_ensure (POLYHYMNIA_TYPE_QUEUE_PANE);
  g_type_ensure (POLYHYMNIA_TYPE_SEARCH_PAGE);
  g_type_ensure (POLYHYMNIA_TYPE_TRACKS_PAGE);

  gtk_widget_init_template (GTK_WIDGET (self));
  gtk_list_box_select_row (self->sidebar_box, self->last_modified_sidebar_row);

  g_object_bind_property (polyhymnia_player_bar_get_queue_toggle_button (self->player_bar),
                          "active",
                          self->content, "show-sidebar",
                          G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);

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
polyhymnia_window_album_deleted (PolyhymniaWindow    *self,
                                 PolyhymniaAlbumPage *album_page)
{
  g_assert (POLYHYMNIA_IS_WINDOW (self));
  adw_navigation_view_pop_to_tag (self->library_navigation_view, "albums-list");
}

static void
polyhymnia_window_mpd_client_initialized (PolyhymniaWindow    *self,
                                          GParamSpec          *pspec,
                                          PolyhymniaMpdClient *user_data)
{
  GtkWidget *new_child;
  GtkWidget *previous_child;

  g_assert (POLYHYMNIA_IS_WINDOW (self));

  previous_child = adw_toast_overlay_get_child (self->root_toast_overlay);

  if (polyhymnia_mpd_client_is_initialized (user_data))
  {
    new_child = GTK_WIDGET (self->content);
  }
  else
  {
    new_child = GTK_WIDGET (self->no_mpd_connection_page);
  }

  if (new_child != previous_child)
  {
    adw_toast_overlay_set_child (self->root_toast_overlay, new_child);
    if (previous_child != NULL)
    {
      gtk_widget_unparent (previous_child);
    }
  }
}

static void
polyhymnia_window_mpd_playlists_changed (PolyhymniaWindow    *self,
                                         PolyhymniaMpdClient *user_data)
{
  AdwToast *playlists_updated_toast;

  g_assert (POLYHYMNIA_IS_WINDOW (self));

  playlists_updated_toast = adw_toast_new (_("Stored playlists have changed"));
  adw_toast_overlay_add_toast (self->root_toast_overlay,
                               playlists_updated_toast);
}

static void
polyhymnia_window_mpd_database_updated (PolyhymniaWindow    *self,
                                        PolyhymniaMpdClient *user_data)
{
  AdwToast *database_updated_toast;

  g_assert (POLYHYMNIA_IS_WINDOW (self));

  database_updated_toast = adw_toast_new (_("Library has been updated"));
  adw_toast_overlay_add_toast (self->root_toast_overlay,
                               database_updated_toast);
}

static void
polyhymnia_window_navigate_album (PolyhymniaWindow     *self,
                                  const gchar          *album_title,
                                  PolyhymniaAlbumsPage *user_data)
{
  PolyhymniaAlbumPage *album_page;

  g_assert (POLYHYMNIA_IS_WINDOW (self));

  album_page = g_object_new (POLYHYMNIA_TYPE_ALBUM_PAGE,
                             "album-title", album_title,
                             NULL);
  g_signal_connect_swapped (album_page, "deleted",
                            (GCallback) polyhymnia_window_album_deleted,
                            self);
  g_signal_connect_swapped (album_page, "view-track-details",
                            (GCallback) polyhymnia_window_track_show_details,
                            self);
  adw_navigation_view_push (self->library_navigation_view,
                            ADW_NAVIGATION_PAGE (album_page));
}

static void
polyhymnia_window_navigate_artist (PolyhymniaWindow      *self,
                                   const gchar           *artist_name,
                                   PolyhymniaArtistsPage *user_data)
{
  g_assert (POLYHYMNIA_IS_WINDOW (self));
}

static void
polyhymnia_window_navigate_playlist (PolyhymniaWindow        *self,
                                     const gchar             *playlist_title,
                                     PolyhymniaPlaylistsPage *user_data)
{
  PolyhymniaPlaylistPage *playlist_page;

  g_assert (POLYHYMNIA_IS_WINDOW (self));

  playlist_page = g_object_new (POLYHYMNIA_TYPE_PLAYLIST_PAGE,
                                "playlist-title", playlist_title,
                                NULL);
  g_signal_connect_swapped (playlist_page, "deleted",
                            (GCallback) polyhymnia_window_playlist_deleted,
                            self);
  g_signal_connect_swapped (playlist_page, "view-track-details",
                            (GCallback) polyhymnia_window_track_show_details,
                            self);
  adw_navigation_view_push (self->library_navigation_view,
                            ADW_NAVIGATION_PAGE (playlist_page));
}

static void
polyhymnia_window_playlist_deleted (PolyhymniaWindow        *self,
                                    PolyhymniaPlaylistPage  *playlist_page)
{
  g_assert (POLYHYMNIA_IS_WINDOW (self));
  adw_navigation_view_pop_to_tag (self->library_navigation_view, "playlists-list");
}

static void
polyhymnia_window_scan_button_clicked (PolyhymniaWindow *self,
                                       AdwSplitButton   *user_data)
{
  GtkApplication *app;

  g_assert (POLYHYMNIA_IS_WINDOW (self));

  app = gtk_window_get_application (GTK_WINDOW (self));
  g_action_group_activate_action (G_ACTION_GROUP (app), "scan", NULL);
}

static void
polyhymnia_window_search_changed (PolyhymniaWindow *self,
                                  GtkSearchEntry   *user_data)
{
  g_assert (POLYHYMNIA_IS_WINDOW (self));

  if (adw_navigation_view_get_visible_page (self->library_navigation_view) != self->search_page)
  {
    polyhymnia_window_search_started (self, user_data);
  }

  polyhymnia_search_page_set_search_query (POLYHYMNIA_SEARCH_PAGE (self->search_page),
                                           gtk_editable_get_text (GTK_EDITABLE (self->search_entry)));
}

static void
polyhymnia_window_search_started (PolyhymniaWindow *self,
                                  GtkSearchEntry   *user_data)
{
  g_assert (POLYHYMNIA_IS_WINDOW (self));

  gtk_list_box_unselect_all (self->sidebar_box);
  adw_navigation_view_replace (self->library_navigation_view,
                               &(self->search_page), 1);
}

static void
polyhymnia_window_search_stopped (PolyhymniaWindow *self,
                                  GtkSearchEntry   *user_data)
{
  g_assert (POLYHYMNIA_IS_WINDOW (self));

  gtk_list_box_select_row (self->sidebar_box, self->last_selected_sidebar_row);
}

static void
polyhymnia_window_track_show_details (PolyhymniaWindow *self,
                                      const gchar      *track_uri,
                                      GObject          *user_data)
{
  GtkApplication *app;

  g_assert (POLYHYMNIA_IS_WINDOW (self));

  app = gtk_window_get_application (GTK_WINDOW (self));
  g_action_group_activate_action (G_ACTION_GROUP (app), "track-details",
                                  g_variant_new_string (track_uri));
}

static void
polyhymnia_window_sidebar_row_selected (PolyhymniaWindow    *self,
                                        GtkListBoxRow       *selected_row,
                                        GtkListBox          *sidebar)
{
  AdwNavigationPage *destination;

  if (selected_row == NULL || self->last_selected_sidebar_row == selected_row)
  {
    return;
  }

  g_assert (POLYHYMNIA_IS_WINDOW (self));

  self->last_selected_sidebar_row = selected_row;
  if (selected_row == self->last_modified_sidebar_row)
  {
    destination = self->last_modified_page;
  }
  else if (selected_row == self->artists_sidebar_row)
  {
    destination = self->artists_page;
  }
  else if (selected_row == self->albums_sidebar_row)
  {
    destination = self->albums_page;
  }
  else if (selected_row == self->tracks_sidebar_row)
  {
    destination = self->tracks_page;
  }
  else if (selected_row == self->playlists_sidebar_row)
  {
    destination = self->playlists_page;
  }

  adw_navigation_view_replace (self->library_navigation_view, &destination, 1);
  gtk_search_bar_set_search_mode (self->search_bar, FALSE);
}
