
#include "config.h"

#include "polyhymnia-last-modified-page.h"

#include "polyhymnia-mpd-client-api.h"

#define _(x) g_dgettext (GETTEXT_PACKAGE, x)

/* Type metadata */
typedef enum
{
  SIGNAL_VIEW_DETAILS = 1,
  N_SIGNALS,
} PolyhymniaLastModifiedPageSignal;

struct _PolyhymniaLastModifiedPage
{
  AdwNavigationPage  parent_instance;

  /* Template widgets */
  AdwToolbarView      *track_toolbar_view;
  AdwStatusPage       *tracks_status_page;

  /* Template objects */
  PolyhymniaMpdClient *mpd_client;
  GListStore          *tracks_model;
  GtkMultiSelection   *tracks_selection_model;
};

G_DEFINE_FINAL_TYPE (PolyhymniaLastModifiedPage, polyhymnia_last_modified_page, ADW_TYPE_NAVIGATION_PAGE)

static guint obj_signals[N_SIGNALS] = { 0, };

/* Event handler declarations */
static void
polyhymnia_last_modified_page_add_tracks_to_queue_button_clicked (PolyhymniaLastModifiedPage *self,
                                                                  GtkButton            *user_data);

static void
polyhymnia_last_modified_page_clear_selection_button_clicked (PolyhymniaLastModifiedPage *self,
                                                              GtkButton            *user_data);

static void
polyhymnia_last_modified_page_mpd_client_initialized (PolyhymniaLastModifiedPage *self,
                                                      GParamSpec           *pspec,
                                                      PolyhymniaMpdClient  *user_data);

static void
polyhymnia_last_modified_page_mpd_database_updated (PolyhymniaLastModifiedPage *self,
                                             PolyhymniaMpdClient  *user_data);

static void
polyhymnia_last_modified_page_play_tracks_button_clicked (PolyhymniaLastModifiedPage *self,
                                                          GtkButton            *user_data);

static void
polyhymnia_last_modified_page_selection_changed (PolyhymniaLastModifiedPage *self,
                                                 guint                position,
                                                 guint                n_items,
                                                 GtkSelectionModel    *user_data);

static void
polyhymnia_last_modified_page_track_activated (PolyhymniaLastModifiedPage *self,
                                               guint                position,
                                               GtkColumnView        *user_data);

/* Private function declaration */
static void
polyhymnia_last_modified_page_fill (PolyhymniaLastModifiedPage *self);

static GPtrArray *
polyhymnia_last_modified_page_get_selected_tracks (PolyhymniaLastModifiedPage *self);

/* Class stuff */
static void
polyhymnia_last_modified_page_dispose(GObject *gobject)
{
  PolyhymniaLastModifiedPage *self = POLYHYMNIA_LAST_MODIFIED_PAGE (gobject);

  adw_navigation_page_set_child (ADW_NAVIGATION_PAGE (self), NULL);
  gtk_widget_dispose_template (GTK_WIDGET (self), POLYHYMNIA_TYPE_LAST_MODIFIED_PAGE);

  G_OBJECT_CLASS (polyhymnia_last_modified_page_parent_class)->dispose (gobject);
}

static void
polyhymnia_last_modified_page_class_init (PolyhymniaLastModifiedPageClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GType type = G_TYPE_FROM_CLASS (gobject_class);
  GType view_detail_types[] = { G_TYPE_STRING };

  gobject_class->dispose = polyhymnia_last_modified_page_dispose;

  obj_signals[SIGNAL_VIEW_DETAILS] =
     g_signal_newv ("view-details", type,
                    G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                    NULL, NULL, NULL, NULL,
                    G_TYPE_NONE,
                    1, view_detail_types);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/com/github/pamugk/polyhymnia/ui/polyhymnia-last-modified-page.ui");

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaLastModifiedPage, track_toolbar_view);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaLastModifiedPage, tracks_status_page);

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaLastModifiedPage, mpd_client);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaLastModifiedPage, tracks_selection_model);

  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_last_modified_page_add_tracks_to_queue_button_clicked);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_last_modified_page_clear_selection_button_clicked);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_last_modified_page_mpd_client_initialized);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_last_modified_page_mpd_database_updated);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_last_modified_page_play_tracks_button_clicked);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_last_modified_page_selection_changed);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_last_modified_page_track_activated);
}

static void
polyhymnia_last_modified_page_init (PolyhymniaLastModifiedPage *self)
{
  self->tracks_model = g_list_store_new (POLYHYMNIA_TYPE_TRACK);

  gtk_widget_init_template (GTK_WIDGET (self));

  gtk_multi_selection_set_model (self->tracks_selection_model,
                                 G_LIST_MODEL (self->tracks_model));

  polyhymnia_last_modified_page_mpd_client_initialized (self, NULL, self->mpd_client);
}

/* Event handler implementations */
static void
polyhymnia_last_modified_page_add_tracks_to_queue_button_clicked (PolyhymniaLastModifiedPage *self,
                                                                  GtkButton            *user_data)
{
  GError    *error = NULL;
  GPtrArray *songs_uri;

  g_assert (POLYHYMNIA_IS_LAST_MODIFIED_PAGE (self));

  songs_uri = polyhymnia_last_modified_page_get_selected_tracks (self);
  polyhymnia_mpd_client_append_songs_to_queue (self->mpd_client, songs_uri, &error);
  g_ptr_array_unref (songs_uri);

  if (error != NULL)
  {
    g_warning("Failed to add tracks into queue: %s\n", error->message);
    g_error_free (error);
    error = NULL;
  }

  gtk_selection_model_unselect_all (GTK_SELECTION_MODEL (self->tracks_selection_model));
}

static void
polyhymnia_last_modified_page_clear_selection_button_clicked (PolyhymniaLastModifiedPage *self,
                                                              GtkButton            *user_data)
{
  gtk_selection_model_unselect_all (GTK_SELECTION_MODEL (self->tracks_selection_model));
}

static void
polyhymnia_last_modified_page_mpd_client_initialized (PolyhymniaLastModifiedPage *self,
                                                      GParamSpec          *pspec,
                                                      PolyhymniaMpdClient *user_data)
{
  g_assert (POLYHYMNIA_IS_LAST_MODIFIED_PAGE (self));

  if (polyhymnia_mpd_client_is_initialized (user_data))
  {
    polyhymnia_last_modified_page_fill (self);
  }
  else
  {
    g_list_store_remove_all (self->tracks_model);
  }
}

static void
polyhymnia_last_modified_page_mpd_database_updated (PolyhymniaLastModifiedPage *self,
                                                    PolyhymniaMpdClient  *user_data)
{
  g_assert (POLYHYMNIA_IS_LAST_MODIFIED_PAGE (self));

  polyhymnia_last_modified_page_fill (self);
}

static void
polyhymnia_last_modified_page_play_tracks_button_clicked (PolyhymniaLastModifiedPage *self,
                                                          GtkButton            *user_data)
{
  GError    *error = NULL;
  GPtrArray *songs_uri;

  g_assert (POLYHYMNIA_IS_LAST_MODIFIED_PAGE (self));

  songs_uri = polyhymnia_last_modified_page_get_selected_tracks (self);
  polyhymnia_mpd_client_play_songs (self->mpd_client, songs_uri, &error);
  g_ptr_array_unref (songs_uri);

  if (error != NULL)
  {
    g_warning("Failed to play tracks: %s\n", error->message);
    g_error_free (error);
    error = NULL;
  }

  gtk_selection_model_unselect_all (GTK_SELECTION_MODEL (self->tracks_selection_model));
}

static void
polyhymnia_last_modified_page_selection_changed (PolyhymniaLastModifiedPage *self,
                                                 guint                position,
                                                 guint                n_items,
                                                 GtkSelectionModel    *user_data)
{
  GtkBitset *selection = gtk_selection_model_get_selection (user_data);

  g_assert (POLYHYMNIA_IS_LAST_MODIFIED_PAGE (self));

  adw_toolbar_view_set_reveal_bottom_bars (self->track_toolbar_view,
                                           !gtk_bitset_is_empty (selection));
  gtk_bitset_unref (selection);
}

static void
polyhymnia_last_modified_page_track_activated (PolyhymniaLastModifiedPage *self,
                                               guint                position,
                                               GtkColumnView        *user_data)
{
  PolyhymniaTrack *track;

  g_assert (POLYHYMNIA_IS_LAST_MODIFIED_PAGE (self));

  track = g_list_model_get_item (G_LIST_MODEL (self->tracks_model), position);
  g_signal_emit (self, obj_signals[SIGNAL_VIEW_DETAILS], 0,
                 polyhymnia_track_get_uri (track));
}

/* Private function implementation */
static void
polyhymnia_last_modified_page_fill (PolyhymniaLastModifiedPage *self)
{
  GError    *error = NULL;
  GDateTime *last_modified_since;
  GtkWidget *new_child;
  GDateTime *now;
  GtkWidget *previous_child;
  GPtrArray *tracks;

  previous_child = adw_navigation_page_get_child (ADW_NAVIGATION_PAGE (self));

  now = g_date_time_new_now_local ();
  last_modified_since = g_date_time_add_days (now, -7);
  gtk_selection_model_unselect_all (GTK_SELECTION_MODEL (self->tracks_selection_model));
  tracks = polyhymnia_mpd_client_get_last_modified_tracks (self->mpd_client,
                                                           last_modified_since,
                                                           &error);
  g_date_time_unref (last_modified_since);
  g_date_time_unref (now);

  if (error != NULL)
  {
    g_object_set (G_OBJECT (self->tracks_status_page),
                  "description", NULL,
                  "icon-name", "error-symbolic",
                  "title", _("Search for songs failed"),
                  NULL);
    new_child = GTK_WIDGET (self->tracks_status_page);
    g_list_store_remove_all (self->tracks_model);
    g_warning("Search for tracks failed: %s\n", error->message);
    g_error_free (error);
    error = NULL;
  }
  else if (tracks->len == 0)
  {
    g_ptr_array_free (tracks, FALSE);
    g_object_set (G_OBJECT (self->tracks_status_page),
                  "description", _("Songs modified in a last week will be displayed here"),
                  "icon-name", "list-symbolic",
                  "title", _("No recently modified songs"),
                  NULL);
    new_child = GTK_WIDGET (self->tracks_status_page);
    g_list_store_remove_all (self->tracks_model);
  }
  else
  {
    g_list_store_splice (self->tracks_model,
                         0, g_list_model_get_n_items (G_LIST_MODEL (self->tracks_model)),
                         tracks->pdata, tracks->len);
    g_ptr_array_free (tracks, TRUE);
    new_child = GTK_WIDGET (self->track_toolbar_view);
  }

  if (new_child != previous_child)
  {
    adw_navigation_page_set_child (ADW_NAVIGATION_PAGE (self), new_child);
    if (previous_child != NULL)
    {
      gtk_widget_unparent (previous_child);
    }
  }
}

static GPtrArray *
polyhymnia_last_modified_page_get_selected_tracks (PolyhymniaLastModifiedPage *self)
{
  GtkBitset         *selected_tracks;
  GPtrArray         *songs_uri;

  selected_tracks = gtk_selection_model_get_selection (
                      GTK_SELECTION_MODEL (self->tracks_selection_model));
  songs_uri = g_ptr_array_sized_new (gtk_bitset_get_size (selected_tracks));

  for (guint i = 0; i < gtk_bitset_get_size (selected_tracks); i++)
  {
    guint track_index = gtk_bitset_get_nth (selected_tracks, i);
    const PolyhymniaTrack *track;
    track = g_list_model_get_item (G_LIST_MODEL (self->tracks_selection_model),
                                   track_index);
    g_ptr_array_add (songs_uri, (gpointer) polyhymnia_track_get_uri (track));
  }
  gtk_bitset_unref (selected_tracks);

  return songs_uri;
}
