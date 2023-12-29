
#include "config.h"

#include "polyhymnia-tracks-page.h"

#include "polyhymnia-mpd-client-api.h"

#define _(x) g_dgettext (GETTEXT_PACKAGE, x)

/* Type metadata */
struct _PolyhymniaTracksPage
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

G_DEFINE_FINAL_TYPE (PolyhymniaTracksPage, polyhymnia_tracks_page, ADW_TYPE_NAVIGATION_PAGE)

/* Event handler declarations */
static void
polyhymnia_tracks_page_add_tracks_to_queue_button_clicked (PolyhymniaTracksPage *self,
                                                           GtkButton            *user_data);

static void
polyhymnia_tracks_page_clear_selection_button_clicked (PolyhymniaTracksPage *self,
                                                       GtkButton            *user_data);

static void
polyhymnia_tracks_page_mpd_client_initialized (PolyhymniaTracksPage *self,
                                               GParamSpec           *pspec,
                                               PolyhymniaMpdClient  *user_data);

static void
polyhymnia_tracks_page_mpd_database_updated (PolyhymniaTracksPage *self,
                                             PolyhymniaMpdClient  *user_data);

static void
polyhymnia_tracks_page_play_tracks_button_clicked (PolyhymniaTracksPage *self,
                                                   GtkButton            *user_data);

static void
polyhymnia_tracks_page_selection_changed (PolyhymniaTracksPage *self,
                                          guint                position,
                                          guint                n_items,
                                          GtkSelectionModel    *user_data);

/* Private function declaration */
static void
polyhymnia_tracks_page_fill (PolyhymniaTracksPage *self);

static GPtrArray *
polyhymnia_tracks_page_get_selected_tracks (PolyhymniaTracksPage *self);

/* Class stuff */
static void
polyhymnia_tracks_page_dispose(GObject *gobject)
{
  PolyhymniaTracksPage *self = POLYHYMNIA_TRACKS_PAGE (gobject);

  adw_navigation_page_set_child (ADW_NAVIGATION_PAGE (self), NULL);
  gtk_widget_dispose_template (GTK_WIDGET (self), POLYHYMNIA_TYPE_TRACKS_PAGE);

  G_OBJECT_CLASS (polyhymnia_tracks_page_parent_class)->dispose (gobject);
}

static void
polyhymnia_tracks_page_class_init (PolyhymniaTracksPageClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->dispose = polyhymnia_tracks_page_dispose;

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/com/github/pamugk/polyhymnia/ui/polyhymnia-tracks-page.ui");

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTracksPage, track_toolbar_view);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTracksPage, tracks_status_page);

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTracksPage, mpd_client);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTracksPage, tracks_selection_model);

  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_tracks_page_add_tracks_to_queue_button_clicked);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_tracks_page_clear_selection_button_clicked);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_tracks_page_mpd_client_initialized);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_tracks_page_mpd_database_updated);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_tracks_page_play_tracks_button_clicked);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_tracks_page_selection_changed);
}

static void
polyhymnia_tracks_page_init (PolyhymniaTracksPage *self)
{
  self->tracks_model = g_list_store_new (POLYHYMNIA_TYPE_TRACK);

  gtk_widget_init_template (GTK_WIDGET (self));

  gtk_multi_selection_set_model (self->tracks_selection_model,
                                 G_LIST_MODEL (self->tracks_model));

  polyhymnia_tracks_page_mpd_client_initialized (self, NULL, self->mpd_client);
}

/* Event handler implementations */
static void
polyhymnia_tracks_page_add_tracks_to_queue_button_clicked (PolyhymniaTracksPage *self,
                                                           GtkButton            *user_data)
{
  GError    *error = NULL;
  GPtrArray *songs_uri;

  g_assert (POLYHYMNIA_IS_TRACKS_PAGE (self));

  songs_uri = polyhymnia_tracks_page_get_selected_tracks (self);
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
polyhymnia_tracks_page_clear_selection_button_clicked (PolyhymniaTracksPage *self,
                                                       GtkButton            *user_data)
{
  gtk_selection_model_unselect_all (GTK_SELECTION_MODEL (self->tracks_selection_model));
}

static void
polyhymnia_tracks_page_mpd_client_initialized (PolyhymniaTracksPage *self,
                                               GParamSpec          *pspec,
                                               PolyhymniaMpdClient *user_data)
{
  g_assert (POLYHYMNIA_IS_TRACKS_PAGE (self));

  if (polyhymnia_mpd_client_is_initialized (user_data))
  {
    polyhymnia_tracks_page_fill (self);
  }
  else
  {
    g_list_store_remove_all (self->tracks_model);
  }
}

static void
polyhymnia_tracks_page_mpd_database_updated (PolyhymniaTracksPage *self,
                                             PolyhymniaMpdClient  *user_data)
{
  g_assert (POLYHYMNIA_IS_TRACKS_PAGE (self));

  polyhymnia_tracks_page_fill (self);
}

static void
polyhymnia_tracks_page_play_tracks_button_clicked (PolyhymniaTracksPage *self,
                                                   GtkButton            *user_data)
{
  GError    *error = NULL;
  GPtrArray *songs_uri;

  g_assert (POLYHYMNIA_IS_TRACKS_PAGE (self));

  songs_uri = polyhymnia_tracks_page_get_selected_tracks (self);
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
polyhymnia_tracks_page_selection_changed (PolyhymniaTracksPage *self,
                                          guint                position,
                                          guint                n_items,
                                          GtkSelectionModel    *user_data)
{
  GtkBitset *selection = gtk_selection_model_get_selection (user_data);

  g_assert (POLYHYMNIA_IS_TRACKS_PAGE (self));

  adw_toolbar_view_set_reveal_bottom_bars (self->track_toolbar_view,
                                           !gtk_bitset_is_empty (selection));
  gtk_bitset_unref (selection);
}

/* Private function implementation */
static void
polyhymnia_tracks_page_fill (PolyhymniaTracksPage *self)
{
  GError    *error = NULL;
  GtkWidget *new_child;
  GtkWidget *previous_child;
  GPtrArray *tracks;

  previous_child = adw_navigation_page_get_child (ADW_NAVIGATION_PAGE (self));

  gtk_selection_model_unselect_all (GTK_SELECTION_MODEL (self->tracks_selection_model));
  tracks = polyhymnia_mpd_client_search_tracks (self->mpd_client, "", &error);
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
                  "description", _("If something is missing, try launching library scanning"),
                  "icon-name", "question-round-symbolic",
                  "title", _("No songs found"),
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
polyhymnia_tracks_page_get_selected_tracks (PolyhymniaTracksPage *self)
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
