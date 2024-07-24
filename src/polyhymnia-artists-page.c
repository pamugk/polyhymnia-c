
#include "config.h"

#include "polyhymnia-artists-page.h"

#include "polyhymnia-album-header.h"
#include "polyhymnia-mpd-client-api.h"
#include "polyhymnia-mpd-client-images.h"

#define _(x) g_dgettext (GETTEXT_PACKAGE, x)

/* Type metadata */
typedef enum
{
  PROP_COLLAPSED_VIEW = 1,
  N_PROPERTIES,
} PolyhymniaArtistsPageProperty;

typedef enum
{
  SIGNAL_ALBUMS_COVERS_READY = 1,
  SIGNAL_NAVIGATE,
  SIGNAL_VIEW_TRACK_DETAILS,
  N_SIGNALS,
} PolyhymniaArtistsPageSignal;

struct _PolyhymniaArtistsPage
{
  AdwNavigationPage  parent_instance;

  /* Stored UI state */
  GHashTable             *album_covers;
  GCancellable           *album_covers_cancellable;
  GCancellable           *artists_cancellable;
  GCancellable           *artist_discography_cancellable;

  /* Template widgets */
  GtkSpinner             *artists_spinner;
  AdwNavigationSplitView *artists_split_view;
  AdwStatusPage          *artists_status_page;

  AdwNavigationPage      *artist_discography_navigation_page;
  AdwToolbarView         *artist_discography_toolbar_view;
  GtkScrolledWindow      *artist_discography_scrolled_window;
  GtkColumnView          *artist_discography_column_view;
  GtkSpinner             *artist_discography_spinner;
  AdwStatusPage          *artist_discography_status_page;

  /* Template objects */
  PolyhymniaMpdClient    *mpd_client;
  GListStore             *artists_model;
  GtkSingleSelection     *artists_selection_model;
  GListStore             *artist_tracks_model;
  GtkNoSelection         *artist_tracks_selection_model;
  GtkSortListModel       *artist_tracks_sort_model;
};

G_DEFINE_FINAL_TYPE (PolyhymniaArtistsPage, polyhymnia_artists_page, ADW_TYPE_NAVIGATION_PAGE)

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

static guint obj_signals[N_SIGNALS] = { 0, };

/* Event handler declarations */
static void
polyhymnia_artists_page_album_enqueue (PolyhymniaArtistsPage *self,
                                       PolyhymniaAlbumHeader *user_data);

static void
polyhymnia_artists_page_album_header_bind (PolyhymniaArtistsPage    *self,
                                           GtkListHeader            *object,
                                           GtkSignalListItemFactory *user_data);

static void
polyhymnia_artists_page_album_header_covers_ready (PolyhymniaArtistsPage *self,
                                                   GtkListHeader         *object);

static void
polyhymnia_artists_page_album_header_setup (PolyhymniaArtistsPage    *self,
                                            GtkListHeader            *object,
                                            GtkSignalListItemFactory *user_data);

static void
polyhymnia_artists_page_album_header_teardown (PolyhymniaArtistsPage    *self,
                                               GtkListHeader            *object,
                                               GtkSignalListItemFactory *user_data);

static void
polyhymnia_artists_page_album_header_unbind (PolyhymniaArtistsPage    *self,
                                             GtkListHeader            *object,
                                             GtkSignalListItemFactory *user_data);

static void
polyhymnia_artists_page_album_play (PolyhymniaArtistsPage *self,
                                    PolyhymniaAlbumHeader *user_data);

static void
polyhymnia_artists_page_artist_clicked (PolyhymniaArtistsPage *self,
                                        unsigned int           position,
                                        GtkListView           *user_data);

static void
polyhymnia_artists_page_artist_selection_changed (PolyhymniaArtistsPage *self,
                                                  unsigned int           position,
                                                  unsigned int           n_items,
                                                  GtkSelectionModel     *user_data);

static void
polyhymnia_artists_page_get_albums_covers_callback (GObject      *source_object,
                                                    GAsyncResult *result,
                                                    void         *user_data);

static void
polyhymnia_artists_page_get_artist_discography_callback (GObject      *source_object,
                                                         GAsyncResult *result,
                                                         void         *user_data);

static void
polyhymnia_artists_page_mpd_client_initialized (PolyhymniaArtistsPage *self,
                                                GParamSpec            *pspec,
                                                PolyhymniaMpdClient   *user_data);

static void
polyhymnia_artists_page_mpd_database_updated (PolyhymniaArtistsPage *self,
                                              PolyhymniaMpdClient   *user_data);

static void
polyhymnia_artists_page_search_artists_callback (GObject      *source_object,
                                                 GAsyncResult *result,
                                                 void         *user_data);

static void
polyhymnia_artists_page_track_activated (PolyhymniaArtistsPage *self,
                                         unsigned int           position,
                                         GtkColumnView         *user_data);

/* Private function declaration */
static void
polyhymnia_artists_page_get_albums_covers_async (PolyhymniaArtistsPage *self,
                                                 GPtrArray             *tracks,
                                                 GCancellable          *cancellable,
                                                 GAsyncReadyCallback    callback,
                                                 void                  *user_data);

static void
polyhymnia_artists_page_get_albums_covers_async_thread (GTask         *task,
                                                        void          *source_object,
                                                        void          *task_data,
                                                        GCancellable  *cancellable);

static GHashTable *
polyhymnia_artists_page_get_albums_covers_finish (PolyhymniaArtistsPage *self,
                                                  GAsyncResult          *result,
                                                  GError               **error);

/* Class stuff */
static void
polyhymnia_artists_page_dispose(GObject *gobject)
{
  PolyhymniaArtistsPage *self = POLYHYMNIA_ARTISTS_PAGE (gobject);

  g_cancellable_cancel (self->album_covers_cancellable);
  g_cancellable_cancel (self->artists_cancellable);
  g_cancellable_cancel (self->artist_discography_cancellable);
  adw_navigation_page_set_child (ADW_NAVIGATION_PAGE (self), NULL);
  gtk_widget_dispose_template (GTK_WIDGET (self), POLYHYMNIA_TYPE_ARTISTS_PAGE);
  g_clear_pointer (&(self->album_covers), g_hash_table_unref);

  G_OBJECT_CLASS (polyhymnia_artists_page_parent_class)->dispose (gobject);
}

static void
polyhymnia_artists_page_get_property (GObject     *object,
                                      unsigned int property_id,
                                      GValue      *value,
                                      GParamSpec  *pspec)
{
  PolyhymniaArtistsPage *self = POLYHYMNIA_ARTISTS_PAGE (object);

  switch ((PolyhymniaArtistsPageProperty) property_id)
    {
    case PROP_COLLAPSED_VIEW:
      g_value_set_boolean (value,
                           adw_navigation_split_view_get_collapsed (self->artists_split_view));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
polyhymnia_artists_page_set_property (GObject      *object,
                                      unsigned int  property_id,
                                      const GValue *value,
                                      GParamSpec   *pspec)
{
  PolyhymniaArtistsPage *self = POLYHYMNIA_ARTISTS_PAGE (object);

  switch ((PolyhymniaArtistsPageProperty) property_id)
    {
    case PROP_COLLAPSED_VIEW:
      adw_navigation_split_view_set_collapsed (self->artists_split_view,
                                               g_value_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
polyhymnia_artists_page_class_init (PolyhymniaArtistsPageClass *klass)
{
  GObjectClass   *gobject_class = G_OBJECT_CLASS (klass);
  GType          type = G_TYPE_FROM_CLASS (gobject_class);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  GType navigate_param_types[1] = { G_TYPE_STRING };
  GType view_detail_types[] = { G_TYPE_STRING };

  gobject_class->dispose = polyhymnia_artists_page_dispose;
  gobject_class->get_property = polyhymnia_artists_page_get_property;
  gobject_class->set_property = polyhymnia_artists_page_set_property;

  obj_properties[PROP_COLLAPSED_VIEW] =
    g_param_spec_boolean ("collapsed-view",
                         "Collapsed view",
                         "Whether root should be collapsed.",
                         FALSE,
                         G_PARAM_READWRITE | G_PARAM_STATIC_NAME);

  g_object_class_install_properties (gobject_class,
                                     N_PROPERTIES,
                                     obj_properties);

  obj_signals[SIGNAL_ALBUMS_COVERS_READY] =
     g_signal_newv ("albums-covers-ready", type,
                    G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                    NULL, NULL, NULL, NULL, G_TYPE_NONE, 0, NULL);
  obj_signals[SIGNAL_NAVIGATE] =
     g_signal_newv ("navigate", type,
                    G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                    NULL, NULL, NULL, NULL, G_TYPE_NONE, 1, navigate_param_types);
  obj_signals[SIGNAL_VIEW_TRACK_DETAILS] =
     g_signal_newv ("view-track-details", type,
                    G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                    NULL, NULL, NULL, NULL, G_TYPE_NONE, 1, view_detail_types);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/com/github/pamugk/polyhymnia/ui/polyhymnia-artists-page.ui");

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaArtistsPage, artists_spinner);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaArtistsPage, artists_split_view);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaArtistsPage, artists_status_page);

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaArtistsPage, artist_discography_toolbar_view);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaArtistsPage, artist_discography_navigation_page);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaArtistsPage, artist_discography_scrolled_window);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaArtistsPage, artist_discography_column_view);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaArtistsPage, artist_discography_spinner);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaArtistsPage, artist_discography_status_page);

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaArtistsPage, mpd_client);

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaArtistsPage, artists_selection_model);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaArtistsPage, artist_tracks_selection_model);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaArtistsPage, artist_tracks_sort_model);

  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_artists_page_album_header_bind);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_artists_page_album_header_setup);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_artists_page_album_header_teardown);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_artists_page_album_header_unbind);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_artists_page_artist_clicked);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_artists_page_artist_selection_changed);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_artists_page_mpd_client_initialized);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_artists_page_mpd_database_updated);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_artists_page_track_activated);
}

static void
polyhymnia_artists_page_init (PolyhymniaArtistsPage *self)
{
  self->artists_model = g_list_store_new (POLYHYMNIA_TYPE_ARTIST);
  self->artist_tracks_model = g_list_store_new (POLYHYMNIA_TYPE_TRACK);

  gtk_widget_init_template (GTK_WIDGET (self));

  gtk_single_selection_set_model (self->artists_selection_model,
                                  G_LIST_MODEL (self->artists_model));
  gtk_sort_list_model_set_model (self->artist_tracks_sort_model,
                                 G_LIST_MODEL (self->artist_tracks_model));

  polyhymnia_artists_page_mpd_client_initialized (self, NULL, self->mpd_client);
}

/* Event handler functions implementation */
static void
polyhymnia_artists_page_album_enqueue (PolyhymniaArtistsPage *self,
                                       PolyhymniaAlbumHeader *user_data)
{
  const gchar *album = polyhymnia_album_header_get_album_title (user_data);
  GError *error = NULL;

  g_assert (POLYHYMNIA_IS_ARTISTS_PAGE (self));

  polyhymnia_mpd_client_append_album_to_queue (self->mpd_client, album, &error);

  if (error != NULL)
  {
    g_warning("Failed to add album into queue: %s\n", error->message);
    g_error_free (error);
    error = NULL;
  }
}

static void
polyhymnia_artists_page_album_header_covers_ready (PolyhymniaArtistsPage *self,
                                                   GtkListHeader         *object)
{
  const gchar     *album_title;
  PolyhymniaTrack *track;

  g_return_if_fail (POLYHYMNIA_IS_ARTISTS_PAGE (self));

  track = gtk_list_header_get_item (object);
  album_title = polyhymnia_track_get_album (track);
  g_object_set (G_OBJECT (gtk_list_header_get_child (object)),
                "album-cover",
                album_title == NULL || self->album_covers == NULL
                  ? NULL
                  : g_hash_table_lookup (self->album_covers, album_title),
                NULL);
}

static void
polyhymnia_artists_page_album_header_bind (PolyhymniaArtistsPage    *self,
                                           GtkListHeader            *object,
                                           GtkSignalListItemFactory *user_data)
{
  const gchar     *album_title;
  PolyhymniaTrack *track;

  g_assert (POLYHYMNIA_IS_ARTISTS_PAGE (self));

  track = gtk_list_header_get_item (object);
  album_title = polyhymnia_track_get_album (track);
  g_object_set (G_OBJECT (gtk_list_header_get_child (object)),
                "album-cover",
                album_title == NULL || self->album_covers == NULL
                  ? NULL
                  : g_hash_table_lookup (self->album_covers, album_title),
                "album-release", polyhymnia_track_get_date (track),
                "album-title", polyhymnia_track_get_album (track),
                NULL);
}

static void
polyhymnia_artists_page_album_header_setup (PolyhymniaArtistsPage    *self,
                                            GtkListHeader            *object,
                                            GtkSignalListItemFactory *user_data)
{
  GtkWidget *album_header;

  g_assert (POLYHYMNIA_IS_ARTISTS_PAGE (self));

  album_header = g_object_new (POLYHYMNIA_TYPE_ALBUM_HEADER, NULL);
  g_signal_connect (self, "albums-covers-ready",
                    G_CALLBACK (polyhymnia_artists_page_album_header_covers_ready),
                    object);
  g_signal_connect_swapped (album_header, "enqueued",
                            G_CALLBACK (polyhymnia_artists_page_album_enqueue),
                            self);
  g_signal_connect_swapped (album_header, "played",
                            G_CALLBACK (polyhymnia_artists_page_album_play),
                            self);

  gtk_list_header_set_child (object, album_header);
}

static void
polyhymnia_artists_page_album_header_teardown (PolyhymniaArtistsPage    *self,
                                               GtkListHeader            *object,
                                               GtkSignalListItemFactory *user_data)
{
  g_assert (POLYHYMNIA_IS_ARTISTS_PAGE (self));
  g_signal_handlers_disconnect_by_data (self, object);
}

static void
polyhymnia_artists_page_album_header_unbind (PolyhymniaArtistsPage    *self,
                                             GtkListHeader            *object,
                                             GtkSignalListItemFactory *user_data)
{
  g_assert (POLYHYMNIA_IS_ARTISTS_PAGE (self));
  g_object_set (G_OBJECT (gtk_list_header_get_child (object)),
                "album-cover", NULL,
                "album-release", NULL,
                "album-title", NULL,
                NULL);
}

static void
polyhymnia_artists_page_album_play (PolyhymniaArtistsPage *self,
                                    PolyhymniaAlbumHeader *user_data)
{
  const gchar *album = polyhymnia_album_header_get_album_title (user_data);
  GError *error = NULL;

  g_assert (POLYHYMNIA_IS_ARTISTS_PAGE (self));

  polyhymnia_mpd_client_play_album (self->mpd_client, album, &error);

  if (error != NULL)
  {
    g_warning("Failed to start playing album: %s\n", error->message);
    g_error_free (error);
    error = NULL;
  }
}

static void
polyhymnia_artists_page_artist_clicked (PolyhymniaArtistsPage *self,
                                        unsigned               position,
                                        GtkListView           *user_data)
{
  PolyhymniaArtist *artist;

  g_assert (POLYHYMNIA_IS_ARTISTS_PAGE (self));

  artist = g_list_model_get_item (G_LIST_MODEL (self->artists_model), position);
  g_signal_emit (self, obj_signals[SIGNAL_NAVIGATE], 0,
                 polyhymnia_artist_get_name (artist));
}

static void
polyhymnia_artists_page_artist_selection_changed (PolyhymniaArtistsPage *self,
                                                  unsigned int           position,
                                                  unsigned int           n_items,
                                                  GtkSelectionModel     *user_data)
{
  GtkBitset *selected_artists;

  g_assert (POLYHYMNIA_IS_ARTISTS_PAGE (self));

  selected_artists = gtk_selection_model_get_selection (user_data);

  g_clear_pointer (&(self->album_covers), g_hash_table_unref);
  g_cancellable_cancel (self->album_covers_cancellable);
  if (gtk_bitset_get_size (selected_artists) == 0)
  {
    g_cancellable_cancel (self->artist_discography_cancellable);
    g_object_set (G_OBJECT (self->artist_discography_status_page),
                  "description", _("Discography of an artist will be displayed here"),
                  "icon-name", "list-symbolic",
                  "title", _("No artist selected"),
                  NULL);
    adw_navigation_page_set_title (ADW_NAVIGATION_PAGE (self), "-");
    gtk_scrolled_window_set_child (self->artist_discography_scrolled_window,
                                   GTK_WIDGET(self->artist_discography_status_page));
    g_list_store_remove_all (self->artist_tracks_model);
    adw_navigation_split_view_set_show_content (self->artists_split_view, FALSE);
  }
  else
  {
    GtkAdjustment    *albums_vadjustment;
    PolyhymniaArtist *selected_artist;
    unsigned int      selected_artist_index;
    const gchar      *selected_artist_name;

    if (self->artist_discography_cancellable != NULL)
    {
      return;
    }

    self->artist_discography_cancellable = g_cancellable_new ();
    selected_artist_index = gtk_bitset_get_nth (selected_artists, 0);
    selected_artist = g_list_model_get_item (G_LIST_MODEL (self->artists_model),
                                             selected_artist_index);
    selected_artist_name = polyhymnia_artist_get_name (selected_artist);
    adw_navigation_page_set_title (self->artist_discography_navigation_page,
                                   selected_artist_name);
    albums_vadjustment = gtk_scrolled_window_get_vadjustment (self->artist_discography_scrolled_window);
    gtk_adjustment_set_value (albums_vadjustment, 0);
    adw_navigation_split_view_set_show_content (self->artists_split_view, TRUE);

    polyhymnia_mpd_client_get_artist_discography_async (self->mpd_client,
                                                        selected_artist_name,
                                                        self->artist_discography_cancellable,
                                                        polyhymnia_artists_page_get_artist_discography_callback,
                                                        self);
  }
  gtk_bitset_unref (selected_artists);
}

static void
polyhymnia_artists_page_get_albums_covers_callback (GObject      *source_object,
                                                    GAsyncResult *result,
                                                    void         *user_data)
{
  GHashTable            *album_covers;
  GError                *error = NULL;
  PolyhymniaArtistsPage *self = POLYHYMNIA_ARTISTS_PAGE (source_object);

  album_covers = polyhymnia_artists_page_get_albums_covers_finish (self, result,
                                                                   &error);

  if (error == NULL)
  {
    self->album_covers = album_covers;
    g_signal_emit (self, obj_signals[SIGNAL_ALBUMS_COVERS_READY], 0);
  }

  g_clear_object (&(self->album_covers_cancellable));
}

static void
polyhymnia_artists_page_get_artist_discography_callback (GObject      *source_object,
                                                         GAsyncResult *result,
                                                         void         *user_data)
{
  GError                *error = NULL;
  PolyhymniaMpdClient   *mpd_client = POLYHYMNIA_MPD_CLIENT (source_object);
  GPtrArray             *selected_artist_tracks;
  PolyhymniaArtistsPage *self = user_data;

  selected_artist_tracks = polyhymnia_mpd_client_get_artist_discography_finish (mpd_client,
                                                                                result,
                                                                                &error);
  if (error == NULL)
  {
    if (selected_artist_tracks->len == 0)
    {
        g_object_set (G_OBJECT (self->artist_discography_status_page),
                      "description", _("If something is missing, try launching library scanning"),
                      "icon-name", "question-round-symbolic",
                      "title", _("No artist albums found"),
                      NULL);
        gtk_scrolled_window_set_child (self->artist_discography_scrolled_window,
                                       GTK_WIDGET(self->artist_discography_status_page));
        g_list_store_remove_all (self->artist_tracks_model);
    }
    else
    {
      self->album_covers_cancellable = g_cancellable_new ();
      polyhymnia_artists_page_get_albums_covers_async (self, selected_artist_tracks,
                                                       self->album_covers_cancellable,
                                                       polyhymnia_artists_page_get_albums_covers_callback,
                                                       NULL);
      g_list_store_splice (self->artist_tracks_model, 0,
                           g_list_model_get_n_items (G_LIST_MODEL (self->artist_tracks_model)),
                           selected_artist_tracks->pdata,
                           selected_artist_tracks->len);
      gtk_scrolled_window_set_child (self->artist_discography_scrolled_window,
                                     GTK_WIDGET(self->artist_discography_column_view));
    }
    g_ptr_array_unref (selected_artist_tracks);
  }
  else if (!g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
  {
    g_object_set (G_OBJECT (self->artist_discography_status_page),
                  "description", NULL,
                  "icon-name", "error-symbolic",
                  "title", _("Search for artist discography failed"),
                  NULL);
    gtk_scrolled_window_set_child (self->artist_discography_scrolled_window,
                                   GTK_WIDGET(self->artist_discography_status_page));
    g_list_store_remove_all (self->artist_tracks_model);
    g_warning("Search for artists discography failed: %s\n", error->message);
  }

  gtk_spinner_stop (self->artists_spinner);
  g_clear_object (&(self->artist_discography_cancellable));
}

static void
polyhymnia_artists_page_mpd_client_initialized (PolyhymniaArtistsPage *self,
                                                GParamSpec            *pspec,
                                                PolyhymniaMpdClient   *user_data)
{
  g_assert (POLYHYMNIA_IS_ARTISTS_PAGE (self));

  if (polyhymnia_mpd_client_is_initialized (user_data))
  {
    polyhymnia_artists_page_mpd_database_updated (self, user_data);
    polyhymnia_artists_page_artist_selection_changed (self, 0, 0,
                                                      GTK_SELECTION_MODEL (self->artists_selection_model));
  }
  else
  {
    g_cancellable_cancel (self->album_covers_cancellable);
    g_cancellable_cancel (self->artists_cancellable);
    g_cancellable_cancel (self->artist_discography_cancellable);
    g_clear_pointer (&(self->album_covers), g_hash_table_unref);
    g_list_store_remove_all (self->artist_tracks_model);
    g_list_store_remove_all (self->artists_model);
  }
}

static void
polyhymnia_artists_page_mpd_database_updated (PolyhymniaArtistsPage *self,
                                              PolyhymniaMpdClient   *user_data)
{
  g_assert (POLYHYMNIA_IS_ARTISTS_PAGE (self));

  if (self->artists_cancellable != NULL)
  {
    return;
  }

  g_cancellable_cancel (self->artist_discography_cancellable);
  self->artists_cancellable = g_cancellable_new ();
  polyhymnia_mpd_client_search_artists_async (user_data,
                                              self->artists_cancellable,
                                              polyhymnia_artists_page_search_artists_callback,
                                              self);

  gtk_selection_model_unselect_all (GTK_SELECTION_MODEL (self->artist_tracks_selection_model));
  gtk_selection_model_unselect_all (GTK_SELECTION_MODEL (self->artists_selection_model));
  gtk_spinner_start (self->artists_spinner);

  if (adw_navigation_page_get_child (ADW_NAVIGATION_PAGE (self)) != GTK_WIDGET (self->artists_spinner))
  {
    adw_navigation_page_set_child (ADW_NAVIGATION_PAGE (self), GTK_WIDGET (self->artists_spinner));
  }
}

static void
polyhymnia_artists_page_search_artists_callback (GObject      *source_object,
                                                 GAsyncResult *result,
                                                 void         *user_data)
{
  GPtrArray             *artists;
  GError                *error = NULL;
  PolyhymniaMpdClient   *mpd_client = POLYHYMNIA_MPD_CLIENT (source_object);
  GtkWidget             *new_child;
  PolyhymniaArtistsPage *self = user_data;

  artists = polyhymnia_mpd_client_search_artists_finish (mpd_client, result, &error);
  if (error == NULL)
  {
    if (artists->len == 0)
    {
      g_object_set (G_OBJECT (self->artists_status_page),
                    "description", _("If something is missing, try launching library scanning"),
                    "icon-name", "question-round-symbolic",
                    "title", _("No artists found"),
                    NULL);
      new_child = GTK_WIDGET (self->artists_status_page);
      g_list_store_remove_all (self->artists_model);
    }
    else
    {
      gtk_selection_model_unselect_all (GTK_SELECTION_MODEL (self->artist_tracks_selection_model));
      g_list_store_splice (self->artists_model, 0,
                            g_list_model_get_n_items (G_LIST_MODEL (self->artists_model)),
                            artists->pdata, artists->len);
      new_child = GTK_WIDGET (self->artists_split_view);
    }
    g_ptr_array_unref (artists);
  }
  else if (!g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
  {
    g_object_set (G_OBJECT (self->artists_status_page),
                  "description", NULL,
                  "icon-name", "error-symbolic",
                  "title", _("Search for artists failed"),
                  NULL);
    new_child = GTK_WIDGET (self->artists_status_page);
    g_warning("Search for artists failed: %s\n", error->message);
    g_list_store_remove_all (self->artists_model);
  }
  else
  {
    g_clear_object (&(self->artists_cancellable));
    return;
  }

  if (adw_navigation_page_get_child (ADW_NAVIGATION_PAGE (self)) != new_child)
  {
    adw_navigation_page_set_child (ADW_NAVIGATION_PAGE (self), new_child);
  }

  gtk_spinner_stop (self->artists_spinner);
  g_clear_object (&(self->artists_cancellable));
}

static void
polyhymnia_artists_page_track_activated (PolyhymniaArtistsPage *self,
                                         guint                  position,
                                         GtkColumnView         *user_data)
{
  PolyhymniaTrack *track;

  g_assert (POLYHYMNIA_IS_ARTISTS_PAGE (self));

  track = g_list_model_get_item (G_LIST_MODEL (self->artist_tracks_selection_model), position);
  g_signal_emit (self, obj_signals[SIGNAL_VIEW_TRACK_DETAILS], 0,
                 polyhymnia_track_get_uri (track));
}

/* Private function implementation */
static void
polyhymnia_artists_page_get_albums_covers_async (PolyhymniaArtistsPage *self,
                                                 GPtrArray             *tracks,
                                                 GCancellable          *cancellable,
                                                 GAsyncReadyCallback    callback,
                                                 void                  *user_data)
{
  GTask *task;

  task = g_task_new (self, cancellable, callback, user_data);
  g_task_set_task_data (task, g_ptr_array_ref (tracks),
                        (GDestroyNotify) g_ptr_array_unref);
  g_task_set_source_tag (task, polyhymnia_artists_page_get_albums_covers_async);
  g_task_set_return_on_cancel (task, TRUE);
  g_task_run_in_thread (task, polyhymnia_artists_page_get_albums_covers_async_thread);
  g_object_unref (task);
}

static void
polyhymnia_artists_page_get_albums_covers_async_thread (GTask         *task,
                                                        void          *source_object,
                                                        void          *task_data,
                                                        GCancellable  *cancellable)
{
  GHashTable            *album_covers = g_hash_table_new_full (g_str_hash,
                                                               g_str_equal,
                                                               g_free,
                                                               g_object_unref);
  PolyhymniaArtistsPage *self = source_object;
  GPtrArray             *tracks = task_data;

  for (unsigned int i = 0; i < tracks->len; i++)
  {
    const PolyhymniaTrack *track = g_ptr_array_index (tracks, i);
    const gchar *album = polyhymnia_track_get_album (track);
    if (album != NULL && !g_hash_table_contains (album_covers, album))
    {
      GBytes *cover;
      GError *error = NULL;
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
          g_hash_table_insert (album_covers,
                               g_strdup (album),
                               album_cover);
        }
        g_bytes_unref (cover);
      }
    }
  }

  if (g_task_set_return_on_cancel (task, FALSE))
  {
    g_task_return_pointer (task, album_covers, (GDestroyNotify) g_hash_table_unref);
  }
  else
  {
    g_hash_table_unref (album_covers);
  }
}

static GHashTable *
polyhymnia_artists_page_get_albums_covers_finish (PolyhymniaArtistsPage *self,
                                                  GAsyncResult          *result,
                                                  GError               **error)
{
  g_return_val_if_fail (g_task_is_valid (result, self), NULL);
  return g_task_propagate_pointer (G_TASK (result), error);
}
