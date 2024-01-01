
#include "config.h"

#include "polyhymnia-queue-pane.h"

#include <libadwaita-1/adwaita.h>

#include "polyhymnia-mpd-client-queue.h"
#include "polyhymnia-mpd-client-images.h"
#include "polyhymnia-mpd-client-playlists.h"
#include "polyhymnia-track.h"

#define _(x) g_dgettext (GETTEXT_PACKAGE, x)

/* Type metadata */
typedef enum
{
  SIGNAL_VIEW_TRACK_DETAILS = 1,
  N_SIGNALS,
} PolyhymniaTracksPageSignal;

struct _PolyhymniaQueuePane
{
  GtkWidget  parent_instance;

  /* Stored UI state */
  GHashTable             *album_covers;
  GHashTable             *known_playlists;

  /* Template widgets */
  AdwToolbarView      *root_toolbar_view;
  GtkPopover          *new_playlist_popover;
  GtkEntry            *new_playlist_title_entry;
  GtkActionBar        *queue_action_bar;
  GtkListView         *queue_list_view;
  GtkScrolledWindow   *queue_page_content;
  AdwStatusPage       *queue_status_page;
  GtkButton           *play_button;
  GtkButton           *up_button;
  GtkButton           *down_button;
  GtkLabel            *playlist_exists_label;
  GtkButton           *save_playlist_button;

  /* Template objects */
  GListStore          *queue_model;
  GtkMultiSelection   *queue_selection_model;

  PolyhymniaMpdClient *mpd_client;
};

G_DEFINE_FINAL_TYPE (PolyhymniaQueuePane, polyhymnia_queue_pane, GTK_TYPE_WIDGET)

static guint obj_signals[N_SIGNALS] = { 0, };

/* Event handlers*/
static void
polyhymnia_queue_pane_clear_button_clicked (PolyhymniaQueuePane *self,
                                            GtkButton           *user_data);

static void
polyhymnia_queue_pane_clear_selection_button_clicked (PolyhymniaQueuePane *self,
                                                      GtkButton           *user_data);

static void
polyhymnia_queue_pane_down_button_clicked (PolyhymniaQueuePane *self,
                                           GtkButton           *user_data);

static void
polyhymnia_queue_pane_mpd_client_initialized (PolyhymniaQueuePane *self,
                                              GParamSpec          *pspec,
                                              PolyhymniaMpdClient *user_data);

static void
polyhymnia_queue_pane_mpd_queue_modified (PolyhymniaQueuePane *self,
                                          PolyhymniaMpdClient *user_data);

static void
polyhymnia_queue_pane_mpd_playlists_changed (PolyhymniaQueuePane *self,
                                             PolyhymniaMpdClient *user_data);

static void
polyhymnia_queue_pane_new_playlist_popover_closed (PolyhymniaQueuePane *self,
                                                   GtkPopover          *user_data);

static void
polyhymnia_queue_pane_new_playlist_title_changed (PolyhymniaQueuePane *self,
                                                  GtkEditable         *user_data);

static void
polyhymnia_queue_pane_play_button_clicked (PolyhymniaQueuePane *self,
                                           GtkButton           *user_data);

static void
polyhymnia_queue_pane_queue_to_playlist_button_clicked (PolyhymniaQueuePane *self,
                                                        GtkButton           *user_data);

static void
polyhymnia_queue_pane_remove_button_clicked (PolyhymniaQueuePane *self,
                                             GtkButton           *user_data);

static void
polyhymnia_queue_pane_selection_changed (PolyhymniaQueuePane  *self,
                                         guint                position,
                                         guint                n_items,
                                         GtkSelectionModel    *user_data);

static void
polyhymnia_queue_pane_track_activated (PolyhymniaQueuePane *self,
                                       guint                position,
                                       GtkColumnView        *user_data);
static void
polyhymnia_queue_pane_track_bind (PolyhymniaQueuePane      *self,
                                  GtkListItem              *object,
                                  GtkSignalListItemFactory *user_data);

static void
polyhymnia_queue_pane_track_setup (PolyhymniaQueuePane      *self,
                                   GtkListItem              *object,
                                   GtkSignalListItemFactory *user_data);

static void
polyhymnia_queue_pane_track_teardown (PolyhymniaQueuePane      *self,
                                      GtkListItem              *object,
                                      GtkSignalListItemFactory *user_data);

static void
polyhymnia_queue_pane_track_unbind (PolyhymniaQueuePane      *self,
                                    GtkListItem              *object,
                                    GtkSignalListItemFactory *user_data);

static void
polyhymnia_queue_pane_up_button_clicked (PolyhymniaQueuePane *self,
                                         GtkButton           *user_data);

/* Private methods*/
static void
polyhymnia_queue_pane_fill (PolyhymniaQueuePane *self);

static void
polyhymnia_queue_pane_fill_covers (PolyhymniaQueuePane *self,
                                   GPtrArray             *tracks);

/* Class stuff */
static void
polyhymnia_queue_pane_dispose(GObject *gobject)
{
  PolyhymniaQueuePane *self = POLYHYMNIA_QUEUE_PANE (gobject);

  gtk_widget_unparent (GTK_WIDGET (self->root_toolbar_view));
  gtk_widget_dispose_template (GTK_WIDGET (self), POLYHYMNIA_TYPE_QUEUE_PANE);
  g_clear_pointer (&(self->album_covers), g_hash_table_unref);
  g_clear_pointer (&(self->known_playlists), g_hash_table_unref);

  G_OBJECT_CLASS (polyhymnia_queue_pane_parent_class)->dispose (gobject);
}

static void
polyhymnia_queue_pane_class_init (PolyhymniaQueuePaneClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GType type = G_TYPE_FROM_CLASS (gobject_class);
  GType view_detail_types[] = { G_TYPE_STRING };

  gobject_class->dispose = polyhymnia_queue_pane_dispose;

  obj_signals[SIGNAL_VIEW_TRACK_DETAILS] =
     g_signal_newv ("view-track-details", type,
                    G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                    NULL, NULL, NULL, NULL,
                    G_TYPE_NONE,
                    1, view_detail_types);

  gtk_widget_class_set_layout_manager_type (widget_class, GTK_TYPE_BIN_LAYOUT);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/com/github/pamugk/polyhymnia/ui/polyhymnia-queue-pane.ui");

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaQueuePane, root_toolbar_view);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaQueuePane, new_playlist_popover);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaQueuePane, new_playlist_title_entry);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaQueuePane, queue_action_bar);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaQueuePane, queue_list_view);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaQueuePane, queue_page_content);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaQueuePane, queue_status_page);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaQueuePane, play_button);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaQueuePane, up_button);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaQueuePane, down_button);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaQueuePane, playlist_exists_label);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaQueuePane, save_playlist_button);

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaQueuePane, queue_selection_model);

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaQueuePane, mpd_client);

  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_queue_pane_clear_button_clicked);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_queue_pane_clear_selection_button_clicked);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_queue_pane_down_button_clicked);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_queue_pane_new_playlist_popover_closed);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_queue_pane_new_playlist_title_changed);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_queue_pane_play_button_clicked);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_queue_pane_queue_to_playlist_button_clicked);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_queue_pane_remove_button_clicked);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_queue_pane_selection_changed);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_queue_pane_track_activated);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_queue_pane_track_bind);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_queue_pane_track_setup);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_queue_pane_track_teardown);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_queue_pane_track_unbind);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_queue_pane_up_button_clicked);

  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_queue_pane_mpd_client_initialized);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_queue_pane_mpd_queue_modified);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_queue_pane_mpd_playlists_changed);
}

static void
polyhymnia_queue_pane_init (PolyhymniaQueuePane *self)
{
  self->album_covers = g_hash_table_new_full (g_str_hash, g_str_equal,
                                              g_free, g_object_unref);
  self->known_playlists = g_hash_table_new_full (g_str_hash, g_str_equal,
                                                 g_free, NULL);
  self->queue_model = g_list_store_new (POLYHYMNIA_TYPE_TRACK);

  gtk_widget_init_template (GTK_WIDGET (self));

  gtk_multi_selection_set_model (self->queue_selection_model,
                                 G_LIST_MODEL (self->queue_model));

  polyhymnia_queue_pane_mpd_client_initialized (self, NULL, self->mpd_client);
}

/* Event handler implementation */
static void
polyhymnia_queue_pane_clear_button_clicked (PolyhymniaQueuePane *self,
                                            GtkButton           *user_data)
{
  GError           *error = NULL;

  g_assert (POLYHYMNIA_IS_QUEUE_PANE (self));

  polyhymnia_mpd_client_clear_queue (self->mpd_client, &error);

  if (error != NULL)
  {
    g_warning("Failed to clear queue: %s\n", error->message);
    g_error_free (error);
    error = NULL;
  }
}

static void
polyhymnia_queue_pane_clear_selection_button_clicked (PolyhymniaQueuePane *self,
                                                      GtkButton           *user_data)
{
  g_assert (POLYHYMNIA_IS_QUEUE_PANE (self));
  gtk_selection_model_unselect_all (GTK_SELECTION_MODEL (self->queue_selection_model));
}

static void
polyhymnia_queue_pane_down_button_clicked (PolyhymniaQueuePane *self,
                                           GtkButton           *user_data)
{
  GError          *error = NULL;
  PolyhymniaTrack *next_track;
  guint           selected_index;
  GtkBitset       *selection;
  PolyhymniaTrack *track;

  g_assert (POLYHYMNIA_IS_QUEUE_PANE (self));

  selection = gtk_selection_model_get_selection (GTK_SELECTION_MODEL (self->queue_selection_model));

  selected_index = gtk_bitset_get_nth (selection, 0);
  gtk_bitset_unref (selection);

  if (selected_index >= g_list_model_get_n_items (G_LIST_MODEL (self->queue_model)) - 1)
  {
    return;
  }

  track = g_list_model_get_item(G_LIST_MODEL (self->queue_selection_model),
                                selected_index);
  next_track = g_list_model_get_item(G_LIST_MODEL (self->queue_selection_model),
                                     selected_index + 1);

  polyhymnia_mpd_client_swap_songs_in_queue (self->mpd_client,
                                              polyhymnia_track_get_id (track),
                                              polyhymnia_track_get_id (next_track),
                                              &error);
  if (error != NULL)
  {
    g_warning("Failed to move track higher in queue: %s\n", error->message);
    g_error_free (error);
    error = NULL;
  }
}

static void
polyhymnia_queue_pane_mpd_client_initialized (PolyhymniaQueuePane *self,
                                              GParamSpec          *pspec,
                                              PolyhymniaMpdClient *user_data)
{
  g_assert (POLYHYMNIA_IS_QUEUE_PANE (self));

  if (polyhymnia_mpd_client_is_initialized (user_data))
  {
    polyhymnia_queue_pane_mpd_playlists_changed (self, self->mpd_client);
    polyhymnia_queue_pane_fill (self);
  }
  else
  {
    g_hash_table_remove_all (self->album_covers);
    g_hash_table_remove_all (self->known_playlists);
    g_list_store_remove_all (self->queue_model);
  }
}

static void
polyhymnia_queue_pane_mpd_queue_modified (PolyhymniaQueuePane *self,
                                          PolyhymniaMpdClient *user_data)
{
  g_assert (POLYHYMNIA_IS_QUEUE_PANE (self));

  polyhymnia_queue_pane_fill (self);
}

static void
polyhymnia_queue_pane_mpd_playlists_changed (PolyhymniaQueuePane *self,
                                             PolyhymniaMpdClient *user_data)
{
  GError *error = NULL;
  GPtrArray *playlists;

  g_assert (POLYHYMNIA_IS_QUEUE_PANE (self));

  g_hash_table_remove_all (self->known_playlists);
  playlists = polyhymnia_mpd_client_search_playlists (self->mpd_client, &error);
  if (error != NULL)
  {
    g_warning("Search for playlists failed: %s\n", error->message);
    g_error_free (error);
    error = NULL;
  }
  else
  {
    for (int i = 0; i < playlists->len; i++)
    {
      g_hash_table_add (self->known_playlists, g_ptr_array_index (playlists, i));
    }
    g_ptr_array_free (playlists, FALSE);
  }

  polyhymnia_queue_pane_new_playlist_title_changed (self,
                                                    GTK_EDITABLE (self->new_playlist_title_entry));
}

static void
polyhymnia_queue_pane_new_playlist_popover_closed (PolyhymniaQueuePane *self,
                                                   GtkPopover          *user_data)
{
  g_assert (POLYHYMNIA_IS_QUEUE_PANE (self));

  gtk_editable_set_text (GTK_EDITABLE (self->new_playlist_title_entry), "");
}

static void
polyhymnia_queue_pane_new_playlist_title_changed (PolyhymniaQueuePane *self,
                                                  GtkEditable          *user_data)
{
  const gchar *new_playlist_title;

  g_assert (POLYHYMNIA_IS_QUEUE_PANE (self));

  new_playlist_title = gtk_editable_get_text (user_data);
  if (g_str_equal (new_playlist_title, "")) {
    gtk_widget_set_sensitive (GTK_WIDGET (self->save_playlist_button), FALSE);
    gtk_widget_set_visible (GTK_WIDGET (self->playlist_exists_label), FALSE);
  }
  else if (g_hash_table_contains (self->known_playlists, new_playlist_title)) {
    gtk_widget_set_sensitive (GTK_WIDGET (self->save_playlist_button), FALSE);
    gtk_widget_set_visible (GTK_WIDGET (self->playlist_exists_label), TRUE);
  }
  else {
    gtk_widget_set_sensitive (GTK_WIDGET (self->save_playlist_button), TRUE);
    gtk_widget_set_visible (GTK_WIDGET (self->playlist_exists_label), FALSE);
  }
}

static void
polyhymnia_queue_pane_play_button_clicked (PolyhymniaQueuePane *self,
                                           GtkButton           *user_data)
{
  GError          *error = NULL;
  guint           selected_index;
  GtkBitset       *selection;
  PolyhymniaTrack *track;

  g_assert (POLYHYMNIA_IS_QUEUE_PANE (self));

  selection = gtk_selection_model_get_selection (GTK_SELECTION_MODEL (self->queue_selection_model));

  selected_index = gtk_bitset_get_nth (selection, 0);
  gtk_bitset_unref (selection);

  track = g_list_model_get_item(G_LIST_MODEL (self->queue_selection_model),
                                selected_index);

  polyhymnia_mpd_client_play_song_from_queue (self->mpd_client,
                                              polyhymnia_track_get_id (track),
                                              &error);
  if (error != NULL)
  {
    g_warning("Failed to play track from queue: %s\n", error->message);
    g_error_free (error);
    error = NULL;
  }
}

static void
polyhymnia_queue_pane_queue_to_playlist_button_clicked (PolyhymniaQueuePane *self,
                                                        GtkButton           *user_data)
{
  GError *error = NULL;
  polyhymnia_mpd_client_save_queue_as_playlist (self->mpd_client,
                                                gtk_editable_get_text (GTK_EDITABLE (self->new_playlist_title_entry)),
                                                &error);
  if (error != NULL)
  {
    g_warning("Failed to save queue as playlist: %s\n", error->message);
    g_error_free (error);
    error = NULL;
  }

  gtk_popover_popdown (self->new_playlist_popover);
}

static void
polyhymnia_queue_pane_remove_button_clicked (PolyhymniaQueuePane *self,
                                             GtkButton           *user_data)
{
  GError    *error = NULL;
  GArray    *removed_ids;
  GtkBitset *selection;

  g_assert (POLYHYMNIA_IS_QUEUE_PANE (self));

  selection = gtk_selection_model_get_selection (GTK_SELECTION_MODEL (self->queue_selection_model));
  removed_ids = g_array_sized_new (FALSE, FALSE, sizeof (guint),
                                   gtk_bitset_get_size (selection));
  for (guint i = 0; i < gtk_bitset_get_size (selection); i++)
  {
    guint track_id;
    guint track_index = gtk_bitset_get_nth (selection, i);
    const PolyhymniaTrack *track;
    track = g_list_model_get_item (G_LIST_MODEL (self->queue_selection_model),
                                   track_index);
    track_id = polyhymnia_track_get_id (track);
    g_array_append_val (removed_ids, track_id);
  }
  gtk_bitset_unref (selection);

  polyhymnia_mpd_client_delete_songs_from_queue (self->mpd_client, removed_ids,
                                                 &error);
  g_array_unref (removed_ids);
  if (error != NULL)
  {
    g_warning("Failed to play track from queue: %s\n", error->message);
    g_error_free (error);
    error = NULL;
  }
}

static void
polyhymnia_queue_pane_selection_changed (PolyhymniaQueuePane *self,
                                         guint               position,
                                         guint               n_items,
                                         GtkSelectionModel   *user_data)
{
  gboolean  not_first = FALSE;
  gboolean  not_last = FALSE;
  GtkBitset *selection = gtk_selection_model_get_selection (user_data);
  gboolean  selected_one = gtk_bitset_get_size (selection) == 1;

  if (selected_one)
  {
    guint idx = gtk_bitset_get_nth (selection, 0);
    not_first = idx > 0;
    not_last = idx < g_list_model_get_n_items (G_LIST_MODEL (user_data)) - 1;
  }

  g_assert (POLYHYMNIA_IS_QUEUE_PANE (self));

  gtk_action_bar_set_revealed (self->queue_action_bar, !gtk_bitset_is_empty (selection));
  gtk_widget_set_sensitive (GTK_WIDGET (self->play_button), selected_one);
  gtk_widget_set_sensitive (GTK_WIDGET (self->up_button), not_first);
  gtk_widget_set_sensitive (GTK_WIDGET (self->down_button), not_last);
  gtk_bitset_unref (selection);
}

static void
polyhymnia_queue_pane_track_activated (PolyhymniaQueuePane *self,
                                       guint                position,
                                       GtkColumnView        *user_data)
{
  PolyhymniaTrack *track;

  g_assert (POLYHYMNIA_IS_QUEUE_PANE (self));

  track = g_list_model_get_item (G_LIST_MODEL (self->queue_model), position);
  g_signal_emit (self, obj_signals[SIGNAL_VIEW_TRACK_DETAILS], 0,
                 polyhymnia_track_get_uri (track));
}

static void
polyhymnia_queue_pane_track_bind (PolyhymniaQueuePane      *self,
                                  GtkListItem              *object,
                                  GtkSignalListItemFactory *user_data)
{
  const gchar *album;
  GtkGrid  *track_root;
  PolyhymniaTrack *track;

  g_assert (POLYHYMNIA_IS_QUEUE_PANE (self));

  track_root = GTK_GRID (gtk_list_item_get_child (object));
  track = gtk_list_item_get_item (object);

  album = polyhymnia_track_get_album (track);
  if (album != NULL && g_hash_table_contains (self->album_covers, album))
  {
    gtk_image_set_from_paintable (GTK_IMAGE (gtk_grid_get_child_at (track_root, 0, 0)),
                                  g_hash_table_lookup (self->album_covers, album));
  }
  else
  {
    gtk_image_set_from_icon_name (GTK_IMAGE (gtk_grid_get_child_at (track_root, 0, 0)),
                                  "image-missing-symbolic");
  }
  gtk_label_set_text (GTK_LABEL (gtk_grid_get_child_at (track_root, 1, 0)),
                      polyhymnia_track_get_title (track));
  gtk_label_set_text (GTK_LABEL (gtk_grid_get_child_at (track_root, 1, 1)),
                      polyhymnia_track_get_artist (track));
  gtk_label_set_text (GTK_LABEL (gtk_grid_get_child_at (track_root, 2, 1)),
                      polyhymnia_track_get_album (track));
  gtk_label_set_text (GTK_LABEL (gtk_grid_get_child_at (track_root, 3, 0)),
                      polyhymnia_track_get_duration_readable (track));
}

static void
polyhymnia_queue_pane_track_setup (PolyhymniaQueuePane      *self,
                                   GtkListItem              *object,
                                   GtkSignalListItemFactory *user_data)
{
  GtkGrid  *track_root;
  GtkImage *album_cover;
  GtkLabel *track_label;
  GtkLabel *artist_label;
  GtkLabel *album_label;
  GtkLabel *duration_label;

  g_assert (POLYHYMNIA_IS_QUEUE_PANE (self));

  track_root = GTK_GRID (gtk_grid_new ());
  gtk_grid_set_column_spacing (track_root, 4);
  gtk_grid_set_row_spacing (track_root, 2);

  album_cover = GTK_IMAGE (gtk_image_new ());
  gtk_image_set_pixel_size (album_cover, 50);
  gtk_widget_set_valign (GTK_WIDGET (album_cover), GTK_ALIGN_CENTER);

  track_label = GTK_LABEL (gtk_label_new (NULL));
  gtk_label_set_ellipsize (track_label, PANGO_ELLIPSIZE_END);
  gtk_widget_add_css_class (GTK_WIDGET (track_label), "heading");
  gtk_label_set_xalign (track_label, 0);
  gtk_widget_set_hexpand (GTK_WIDGET (track_label), TRUE);

  artist_label = GTK_LABEL (gtk_label_new (NULL));
  gtk_label_set_ellipsize (artist_label, PANGO_ELLIPSIZE_END);
  gtk_widget_add_css_class (GTK_WIDGET (artist_label), "caption");
  gtk_label_set_xalign (artist_label, 0);

  album_label = GTK_LABEL (gtk_label_new (NULL));
  gtk_label_set_ellipsize (album_label, PANGO_ELLIPSIZE_END);
  gtk_widget_add_css_class (GTK_WIDGET (album_label), "caption");
  gtk_label_set_xalign (album_label, 0);
  gtk_widget_set_hexpand (GTK_WIDGET (album_label), TRUE);

  duration_label = GTK_LABEL (gtk_label_new (NULL));
  gtk_label_set_ellipsize (duration_label, PANGO_ELLIPSIZE_END);
  gtk_widget_add_css_class (GTK_WIDGET (duration_label), "numeric");
  gtk_label_set_xalign (duration_label, 1);
  gtk_label_set_yalign (duration_label, 0.5);

  gtk_grid_attach (track_root, GTK_WIDGET (album_cover), 0, 0, 1, 2);
  gtk_grid_attach (track_root, GTK_WIDGET (track_label), 1, 0, 2, 1);
  gtk_grid_attach (track_root, GTK_WIDGET (artist_label), 1, 1, 1, 1);
  gtk_grid_attach (track_root, GTK_WIDGET (album_label), 2, 1, 1, 1);
  gtk_grid_attach (track_root, GTK_WIDGET (duration_label), 3, 0, 1, 2);

  gtk_list_item_set_child (object, GTK_WIDGET (track_root));
}

static void
polyhymnia_queue_pane_track_teardown (PolyhymniaQueuePane      *self,
                                      GtkListItem              *object,
                                      GtkSignalListItemFactory *user_data)
{
  g_assert (POLYHYMNIA_IS_QUEUE_PANE (self));
}

static void
polyhymnia_queue_pane_track_unbind (PolyhymniaQueuePane      *self,
                                    GtkListItem              *object,
                                    GtkSignalListItemFactory *user_data)
{
  GtkGrid  *track_root;

  g_assert (POLYHYMNIA_IS_QUEUE_PANE (self));

  track_root = GTK_GRID (gtk_list_item_get_child (object));

  gtk_image_set_from_paintable (GTK_IMAGE (gtk_grid_get_child_at (track_root, 0, 0)),
                                NULL);
  gtk_label_set_text (GTK_LABEL (gtk_grid_get_child_at (track_root, 1, 0)),
                      NULL);
  gtk_label_set_text (GTK_LABEL (gtk_grid_get_child_at (track_root, 1, 1)),
                      NULL);
  gtk_label_set_text (GTK_LABEL (gtk_grid_get_child_at (track_root, 2, 1)),
                      NULL);
  gtk_label_set_text (GTK_LABEL (gtk_grid_get_child_at (track_root, 3, 0)),
                      NULL);
}

static void
polyhymnia_queue_pane_up_button_clicked (PolyhymniaQueuePane *self,
                                         GtkButton           *user_data)
{
  GError          *error = NULL;
  PolyhymniaTrack *previous_track;
  guint           selected_index;
  GtkBitset       *selection;
  PolyhymniaTrack *track;

  g_assert (POLYHYMNIA_IS_QUEUE_PANE (self));

  selection = gtk_selection_model_get_selection (GTK_SELECTION_MODEL (self->queue_selection_model));

  selected_index = gtk_bitset_get_nth (selection, 0);
  gtk_bitset_unref (selection);

  if (selected_index == 0)
  {
    return;
  }

  previous_track = g_list_model_get_item(G_LIST_MODEL (self->queue_selection_model),
                                         selected_index - 1);
  track = g_list_model_get_item(G_LIST_MODEL (self->queue_selection_model),
                                selected_index);

  polyhymnia_mpd_client_swap_songs_in_queue (self->mpd_client,
                                              polyhymnia_track_get_id (previous_track),
                                              polyhymnia_track_get_id (track),
                                              &error);
  if (error != NULL)
  {
    g_warning("Failed to move track higher in queue: %s\n", error->message);
    g_error_free (error);
    error = NULL;
  }
}

/* Private methods implementation */
static void
polyhymnia_queue_pane_fill (PolyhymniaQueuePane *self)
{
  GError *error = NULL;
  GPtrArray *queue = polyhymnia_mpd_client_get_queue (self->mpd_client,
                                                      &error);
  g_hash_table_remove_all (self->album_covers);
  gtk_selection_model_unselect_all (GTK_SELECTION_MODEL (self->queue_selection_model));
  if (error != NULL)
  {
    g_object_set (G_OBJECT (self->queue_status_page),
                  "description", _("Failed to fetch queue"),
                  NULL);
    adw_toolbar_view_set_reveal_bottom_bars (self->root_toolbar_view, FALSE);
    adw_toolbar_view_set_reveal_top_bars (self->root_toolbar_view, FALSE);
    gtk_scrolled_window_set_child (self->queue_page_content,
                                   GTK_WIDGET (self->queue_status_page));
    g_warning("Queue fetch failed: %s\n", error->message);
    g_error_free (error);
    error = NULL;
    g_list_store_remove_all (self->queue_model);
  }
  else if (queue->len == 0)
  {
    g_ptr_array_free (queue, FALSE);
    g_object_set (G_OBJECT (self->queue_status_page),
                  "description", _("Queue is empty"),
                  NULL);
    adw_toolbar_view_set_reveal_bottom_bars (self->root_toolbar_view, FALSE);
    adw_toolbar_view_set_reveal_top_bars (self->root_toolbar_view, FALSE);
    gtk_scrolled_window_set_child (self->queue_page_content,
                                   GTK_WIDGET (self->queue_status_page));
    g_list_store_remove_all (self->queue_model);
  }
  else
  {
    polyhymnia_queue_pane_fill_covers (self, queue);
    g_list_store_splice (self->queue_model, 0,
                          g_list_model_get_n_items (G_LIST_MODEL (self->queue_model)),
                          queue->pdata, queue->len);
    g_ptr_array_free (queue, TRUE);
    gtk_scrolled_window_set_child (self->queue_page_content,
                                   GTK_WIDGET (self->queue_list_view));
    adw_toolbar_view_set_reveal_bottom_bars (self->root_toolbar_view, TRUE);
    adw_toolbar_view_set_reveal_top_bars (self->root_toolbar_view, TRUE);
  }
}

static void
polyhymnia_queue_pane_fill_covers (PolyhymniaQueuePane *self,
                                   GPtrArray             *tracks)
{
  GError *error = NULL;
  for (int i = 0; i < tracks->len; i++)
  {
    const PolyhymniaTrack *track = g_ptr_array_index (tracks, i);
    const gchar *album = polyhymnia_track_get_album (track);
    if (album != NULL && !g_hash_table_contains(self->album_covers, album))
    {
      GBytes *cover;
      cover = polyhymnia_mpd_client_get_song_album_cover (self->mpd_client,
                                                          polyhymnia_track_get_uri (track),
                                                          &error);
      if (error != NULL)
      {
        g_warning ("Failed to get album cover: %s\n", error->message);
        g_error_free (error);
        error = NULL;
      }
      else if (cover != NULL)
      {
        GdkTexture *album_cover;
        album_cover = gdk_texture_new_from_bytes (cover, &error);
        if (error != NULL)
        {
          g_warning ("Failed to convert album cover: %s\n", error->message);
          g_error_free (error);
          error = NULL;
        }
        else
        {
          g_hash_table_insert (self->album_covers,
                               g_strdup (album),
                               album_cover);
        }
        g_bytes_unref (cover);
      }
    }
  }
}

