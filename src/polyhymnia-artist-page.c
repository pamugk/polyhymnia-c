
#include "app-features.h"
#include "config.h"

#include "polyhymnia-artist-page.h"

#include "polyhymnia-album-header.h"
#include "polyhymnia-mpd-client-api.h"
#include "polyhymnia-mpd-client-images.h"

#ifdef POLYHYMNIA_FEATURE_EXTERNAL_DATA
#include "polyhymnia-album-details-dialog.h"
#endif

#define _(x) g_dgettext (GETTEXT_PACKAGE, x)

/* Type metadata */
typedef enum
{
  PROP_ARTIST_NAME = 1,
  N_PROPERTIES,
} PolyhymniaArtistPageProperty;

typedef enum
{
  SIGNAL_DELETED = 1,
  SIGNAL_VIEW_TRACK_DETAILS,
  SIGNAL_ALBUMS_COVERS_READY,
  N_SIGNALS,
} PolyhymniaArtistPageSignal;

struct _PolyhymniaArtistPage
{
  AdwNavigationPage  parent_instance;

  /* Stored UI state */
  GHashTable          *album_covers;
  GCancellable        *album_covers_cancellable;
  GCancellable        *artist_discography_cancellable;

  /* Template widgets */
  AdwToolbarView      *root_toolbar_view;
  GtkScrolledWindow   *content;
  GtkColumnView       *tracks_column_view;
  GtkSpinner          *tracks_spinner;
  AdwStatusPage       *tracks_status_page;

  /* Template objects */
  PolyhymniaMpdClient *mpd_client;
  GListStore          *tracks_model;
  GtkNoSelection      *tracks_selection_model;
  GtkSortListModel    *tracks_sort_model;

  /* Instance properties */
  gchar *artist_name;
};

G_DEFINE_FINAL_TYPE (PolyhymniaArtistPage, polyhymnia_artist_page, ADW_TYPE_NAVIGATION_PAGE)

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

static unsigned int obj_signals[N_SIGNALS] = { 0, };

/* Event handler declarations */
static void
polyhymnia_artist_page_add_artist_to_queue_button_clicked (PolyhymniaArtistPage *self,
                                                           GtkButton            *user_data);

static void
polyhymnia_artist_page_album_enqueue (PolyhymniaArtistPage *self,
                                      PolyhymniaAlbumHeader *user_data);

static void
polyhymnia_artist_page_album_header_bind (PolyhymniaArtistPage    *self,
                                          GtkListHeader            *object,
                                          GtkSignalListItemFactory *user_data);

static void
polyhymnia_artist_page_album_header_covers_ready (PolyhymniaArtistPage *self,
                                                  GtkListHeader         *object);

static void
polyhymnia_artist_page_album_header_setup (PolyhymniaArtistPage    *self,
                                           GtkListHeader            *object,
                                           GtkSignalListItemFactory *user_data);

static void
polyhymnia_artist_page_album_header_teardown (PolyhymniaArtistPage    *self,
                                              GtkListHeader            *object,
                                              GtkSignalListItemFactory *user_data);

static void
polyhymnia_artist_page_album_header_unbind (PolyhymniaArtistPage    *self,
                                            GtkListHeader            *object,
                                            GtkSignalListItemFactory *user_data);

static void
polyhymnia_artist_page_album_play (PolyhymniaArtistPage *self,
                                    PolyhymniaAlbumHeader *user_data);

static void
polyhymnia_artist_page_get_albums_covers_callback (GObject      *source_object,
                                                   GAsyncResult *result,
                                                   void         *user_data);

static void
polyhymnia_artist_page_get_discography_callback (GObject      *source_object,
                                                 GAsyncResult *result,
                                                 void         *user_data);

static void
polyhymnia_artist_page_mpd_client_initialized (PolyhymniaArtistPage *self,
                                              GParamSpec            *pspec,
                                              PolyhymniaMpdClient   *user_data);

static void
polyhymnia_artist_page_mpd_database_updated (PolyhymniaArtistPage *self,
                                             PolyhymniaMpdClient  *user_data);

static void
polyhymnia_artist_page_play_artist_button_clicked (PolyhymniaArtistPage *self,
                                                   GtkButton            *user_data);

static void
polyhymnia_artist_page_track_activated (PolyhymniaArtistPage *self,
                                        unsigned int          position,
                                        GtkColumnView        *user_data);

#ifdef POLYHYMNIA_FEATURE_EXTERNAL_DATA
static void
polyhymnia_artist_page_album_show_additional_info (PolyhymniaArtistPage *self,
                                                   PolyhymniaAlbumHeader *user_data);
#endif

/* Private function declarations */
static void
polyhymnia_artist_page_get_albums_covers_async (PolyhymniaArtistPage *self,
                                                GPtrArray            *tracks,
                                                GCancellable         *cancellable,
                                                GAsyncReadyCallback    callback,
                                                void                 *user_data);

static void
polyhymnia_artist_page_get_albums_covers_async_thread (GTask         *task,
                                                       void          *source_object,
                                                       void          *task_data,
                                                       GCancellable  *cancellable);

static GHashTable *
polyhymnia_artist_page_get_albums_covers_finish (PolyhymniaArtistPage *self,
                                                 GAsyncResult         *result,
                                                 GError              **error);

/* Class stuff */
static void
polyhymnia_artist_page_constructed (GObject *gobject)
{
  PolyhymniaArtistPage *self = POLYHYMNIA_ARTIST_PAGE (gobject);

  adw_navigation_page_set_title (ADW_NAVIGATION_PAGE (self), self->artist_name);
  polyhymnia_artist_page_mpd_client_initialized (self, NULL, self->mpd_client);

  G_OBJECT_CLASS (polyhymnia_artist_page_parent_class)->constructed (gobject);
}

static void
polyhymnia_artist_page_dispose(GObject *gobject)
{
  PolyhymniaArtistPage *self = POLYHYMNIA_ARTIST_PAGE (gobject);

  g_cancellable_cancel (self->album_covers_cancellable);
  g_clear_object (&(self->album_covers_cancellable));
  g_cancellable_cancel (self->artist_discography_cancellable);
  g_clear_object (&(self->artist_discography_cancellable));
  g_clear_pointer (&(self->artist_name), g_free);
  adw_navigation_page_set_child (ADW_NAVIGATION_PAGE (self), NULL);
  gtk_widget_dispose_template (GTK_WIDGET (self), POLYHYMNIA_TYPE_ARTIST_PAGE);
  g_clear_pointer (&(self->album_covers), g_hash_table_unref);

  G_OBJECT_CLASS (polyhymnia_artist_page_parent_class)->dispose (gobject);
}

static void
polyhymnia_artist_page_get_property (GObject    *object,
                                     guint       property_id,
                                     GValue     *value,
                                     GParamSpec *pspec)
{
  PolyhymniaArtistPage *self = POLYHYMNIA_ARTIST_PAGE (object);

  switch ((PolyhymniaArtistPageProperty) property_id)
    {
    case PROP_ARTIST_NAME:
      g_value_set_string (value, self->artist_name);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
polyhymnia_artist_page_set_property (GObject      *object,
                                    guint         property_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
  PolyhymniaArtistPage *self = POLYHYMNIA_ARTIST_PAGE (object);

  switch ((PolyhymniaArtistPageProperty) property_id)
    {
    case PROP_ARTIST_NAME:
      g_set_str (&(self->artist_name), g_value_get_string (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
polyhymnia_artist_page_class_init (PolyhymniaArtistPageClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GType type = G_TYPE_FROM_CLASS (gobject_class);
  GType view_detail_types[] = { G_TYPE_STRING };

  gobject_class->constructed = polyhymnia_artist_page_constructed;
  gobject_class->dispose = polyhymnia_artist_page_dispose;
  gobject_class->get_property = polyhymnia_artist_page_get_property;
  gobject_class->set_property = polyhymnia_artist_page_set_property;

  obj_properties[PROP_ARTIST_NAME] =
    g_param_spec_string ("artist-name",
                         "Artist name",
                         "Name of a displayed artist.",
                         NULL,
                         G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_NAME);

  g_object_class_install_properties (gobject_class,
                                     N_PROPERTIES,
                                     obj_properties);

  obj_signals[SIGNAL_DELETED] =
     g_signal_newv ("deleted", type,
                    G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                    NULL, NULL, NULL, NULL,
                    G_TYPE_NONE,
                    0, NULL);
  obj_signals[SIGNAL_VIEW_TRACK_DETAILS] =
     g_signal_newv ("view-track-details", type,
                    G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                    NULL, NULL, NULL, NULL,
                    G_TYPE_NONE,
                    1, view_detail_types);
  obj_signals[SIGNAL_ALBUMS_COVERS_READY] =
     g_signal_newv ("albums-covers-ready", type,
                    G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                    NULL, NULL, NULL, NULL, G_TYPE_NONE, 0, NULL);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/com/github/pamugk/polyhymnia/ui/polyhymnia-artist-page.ui");

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaArtistPage, root_toolbar_view);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaArtistPage, content);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaArtistPage, tracks_column_view);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaArtistPage, tracks_spinner);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaArtistPage, tracks_status_page);

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaArtistPage, mpd_client);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaArtistPage, tracks_selection_model);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaArtistPage, tracks_sort_model);

  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_artist_page_add_artist_to_queue_button_clicked);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_artist_page_album_header_bind);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_artist_page_album_header_setup);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_artist_page_album_header_teardown);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_artist_page_album_header_unbind);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_artist_page_mpd_database_updated);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_artist_page_mpd_client_initialized);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_artist_page_play_artist_button_clicked);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_artist_page_track_activated);
}

static void
polyhymnia_artist_page_init (PolyhymniaArtistPage *self)
{
  self->tracks_model = g_list_store_new (POLYHYMNIA_TYPE_TRACK);

  gtk_widget_init_template (GTK_WIDGET (self));

  gtk_sort_list_model_set_model (self->tracks_sort_model,
                                 G_LIST_MODEL (self->tracks_model));
}

/* Event handler implementations */
static void
polyhymnia_artist_page_album_enqueue (PolyhymniaArtistPage  *self,
                                      PolyhymniaAlbumHeader *user_data)
{
  const gchar *album = polyhymnia_album_header_get_album_title (user_data);
  GError *error = NULL;

  g_assert (POLYHYMNIA_IS_ARTIST_PAGE (self));

  polyhymnia_mpd_client_append_album_to_queue (self->mpd_client, album, &error);

  if (error != NULL)
  {
    g_warning("Failed to add album into queue: %s\n", error->message);
    g_error_free (error);
    error = NULL;
  }
}

static void
polyhymnia_artist_page_album_header_covers_ready (PolyhymniaArtistPage *self,
                                                  GtkListHeader         *object)
{
  GdkTexture      *album_cover;
  const gchar     *album_title;
  PolyhymniaTrack *track;

  g_return_if_fail (POLYHYMNIA_IS_ARTIST_PAGE (self));

  track = gtk_list_header_get_item (object);
  album_title = polyhymnia_track_get_album (track);
  album_cover = album_title == NULL || self->album_covers == NULL
                  ? NULL
                  : g_hash_table_lookup (self->album_covers, album_title);
  g_object_set (G_OBJECT (gtk_list_header_get_child (object)),
                "album-cover", album_cover,
                NULL);
}

static void
polyhymnia_artist_page_album_header_bind (PolyhymniaArtistPage     *self,
                                          GtkListHeader            *object,
                                          GtkSignalListItemFactory *user_data)
{
  GdkTexture      *album_cover;
  const gchar     *album_title;
  PolyhymniaTrack *track;

  g_assert (POLYHYMNIA_IS_ARTIST_PAGE (self));

  track = gtk_list_header_get_item (object);
  album_title = polyhymnia_track_get_album (track);
  album_cover = album_title == NULL || self->album_covers == NULL
                  ? NULL
                  : g_hash_table_lookup (self->album_covers, album_title);
  g_object_set (G_OBJECT (gtk_list_header_get_child (object)),
                "album-cover", album_cover,
                "album-release", polyhymnia_track_get_date (track),
                "album-title", polyhymnia_track_get_album (track),
                "album-musicbrainz-id", polyhymnia_track_get_musicbrainz_album_id (track),
                NULL);
}

static void
polyhymnia_artist_page_album_header_setup (PolyhymniaArtistPage     *self,
                                           GtkListHeader            *object,
                                           GtkSignalListItemFactory *user_data)
{
  GtkWidget *album_header;

  g_assert (POLYHYMNIA_IS_ARTIST_PAGE (self));

  album_header = g_object_new (POLYHYMNIA_TYPE_ALBUM_HEADER, NULL);
  g_signal_connect (self, "albums-covers-ready",
                    G_CALLBACK (polyhymnia_artist_page_album_header_covers_ready),
                    object);
  g_signal_connect_swapped (album_header, "enqueued",
                            G_CALLBACK (polyhymnia_artist_page_album_enqueue),
                            self);
  g_signal_connect_swapped (album_header, "played",
                            G_CALLBACK (polyhymnia_artist_page_album_play),
                            self);
#ifdef POLYHYMNIA_FEATURE_EXTERNAL_DATA
  g_signal_connect_swapped (album_header, "info-requested",
                            G_CALLBACK (polyhymnia_artist_page_album_show_additional_info),
                            self);
#endif

  gtk_list_header_set_child (object, album_header);
}

static void
polyhymnia_artist_page_album_header_teardown (PolyhymniaArtistPage     *self,
                                              GtkListHeader            *object,
                                              GtkSignalListItemFactory *user_data)
{
  g_assert (POLYHYMNIA_IS_ARTIST_PAGE (self));
  g_signal_handlers_disconnect_by_data (self, object);
}

static void
polyhymnia_artist_page_album_header_unbind (PolyhymniaArtistPage     *self,
                                            GtkListHeader            *object,
                                            GtkSignalListItemFactory *user_data)
{
  g_assert (POLYHYMNIA_IS_ARTIST_PAGE (self));
  g_object_set (G_OBJECT (gtk_list_header_get_child (object)),
                "album-cover", NULL,
                "album-release", NULL,
                "album-title", NULL,
                "album-musicbrainz-id", NULL,
                NULL);
}

static void
polyhymnia_artist_page_album_play (PolyhymniaArtistPage  *self,
                                   PolyhymniaAlbumHeader *user_data)
{
  const gchar *album = polyhymnia_album_header_get_album_title (user_data);
  GError *error = NULL;

  g_assert (POLYHYMNIA_IS_ARTIST_PAGE (self));

  polyhymnia_mpd_client_play_album (self->mpd_client, album, &error);

  if (error != NULL)
  {
    g_warning("Failed to start playing album: %s\n", error->message);
    g_error_free (error);
    error = NULL;
  }
}

static void
polyhymnia_artist_page_get_albums_covers_callback (GObject      *source_object,
                                                   GAsyncResult *result,
                                                   void         *user_data)
{
  GHashTable           *album_covers;
  GError               *error = NULL;
  PolyhymniaArtistPage *self = POLYHYMNIA_ARTIST_PAGE (source_object);

  album_covers = polyhymnia_artist_page_get_albums_covers_finish (self, result,
                                                                  &error);

  if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
  {
    g_error_free (error);
    return;
  }

  if (error == NULL)
  {
    self->album_covers = album_covers;
    g_signal_emit (self, obj_signals[SIGNAL_ALBUMS_COVERS_READY], 0);
  }
  else
  {
    g_debug ("An error occurred on album covers loading: %s", error->message);
    g_error_free (error);
  }

  g_clear_object (&(self->album_covers_cancellable));
}

static void
polyhymnia_artist_page_get_discography_callback (GObject      *source_object,
                                                 GAsyncResult *result,
                                                 void         *user_data)
{
  GError               *error = NULL;
  PolyhymniaArtistPage *self;
  GPtrArray            *tracks;

  tracks = polyhymnia_mpd_client_get_artist_discography_finish (POLYHYMNIA_MPD_CLIENT (source_object),
                                                                result,
                                                                &error);
  if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
  {
    g_error_free (error);
    return;
  }

  self = POLYHYMNIA_ARTIST_PAGE (user_data);
  if (error != NULL)
  {
    g_list_store_remove_all (self->tracks_model);
    g_object_set (G_OBJECT (self->tracks_status_page),
                  "description", _("Failed to get an artist"),
                  NULL);
    gtk_scrolled_window_set_child (self->content,
                                    GTK_WIDGET (self->tracks_status_page));
    g_warning("Failed to find an artist: %s", error->message);
    g_error_free (error);
    error = NULL;
  }
  else
  {
    if (tracks->len == 0)
    {
      g_list_store_remove_all (self->tracks_model);
      g_object_set (G_OBJECT (self->tracks_status_page),
                    "description", _("Artist not found"),
                    NULL);
      gtk_scrolled_window_set_child (self->content,
                                      GTK_WIDGET (self->tracks_status_page));
      g_signal_emit (self, obj_signals[SIGNAL_DELETED], 0);
    }
    else
    {
      self->album_covers_cancellable = g_cancellable_new ();
      polyhymnia_artist_page_get_albums_covers_async (self, tracks,
                                                      self->album_covers_cancellable,
                                                      polyhymnia_artist_page_get_albums_covers_callback,
                                                      NULL);
      g_list_store_splice (self->tracks_model, 0,
                            g_list_model_get_n_items (G_LIST_MODEL (self->tracks_model)),
                            tracks->pdata, tracks->len);
      gtk_scrolled_window_set_child (self->content,
                                     GTK_WIDGET (self->tracks_column_view));
    }
    g_ptr_array_unref (tracks);
  }

  gtk_spinner_stop (self->tracks_spinner);
  g_clear_object (&(self->artist_discography_cancellable));
}

static void
polyhymnia_artist_page_mpd_client_initialized (PolyhymniaArtistPage *self,
                                               GParamSpec           *pspec,
                                               PolyhymniaMpdClient  *user_data)
{
  g_assert (POLYHYMNIA_IS_ARTIST_PAGE (self));

  if (polyhymnia_mpd_client_is_initialized (user_data))
  {
    polyhymnia_artist_page_mpd_database_updated (self, user_data);
  }
  else
  {
    g_cancellable_cancel (self->album_covers_cancellable);
    g_clear_object (&(self->album_covers_cancellable));
    g_cancellable_cancel (self->artist_discography_cancellable);
    g_clear_object (&(self->artist_discography_cancellable));
    g_clear_pointer (&(self->album_covers), g_hash_table_unref);
    g_list_store_remove_all (self->tracks_model);
  }
}

static void
polyhymnia_artist_page_mpd_database_updated (PolyhymniaArtistPage *self,
                                             PolyhymniaMpdClient  *user_data)
{
  g_assert (POLYHYMNIA_IS_ARTIST_PAGE (self));

  g_cancellable_cancel (self->album_covers_cancellable);
  g_clear_object (&(self->album_covers_cancellable));
  g_cancellable_cancel (self->artist_discography_cancellable);
  g_clear_object (&(self->artist_discography_cancellable));
  g_clear_pointer (&(self->album_covers), g_hash_table_unref);

  self->artist_discography_cancellable = g_cancellable_new ();
  polyhymnia_mpd_client_get_artist_discography_async (self->mpd_client,
                                                      self->artist_name,
                                                      self->artist_discography_cancellable,
                                                      polyhymnia_artist_page_get_discography_callback,
                                                      self);

  if (gtk_scrolled_window_get_child (self->content) != GTK_WIDGET (self->tracks_spinner))
  {
    gtk_scrolled_window_set_child (self->content, GTK_WIDGET (self->tracks_spinner));
  }
  gtk_spinner_start (self->tracks_spinner);
}

static void
polyhymnia_artist_page_add_artist_to_queue_button_clicked (PolyhymniaArtistPage *self,
                                                           GtkButton            *user_data)
{
  GError *error = NULL;

  g_assert (POLYHYMNIA_IS_ARTIST_PAGE (self));

  polyhymnia_mpd_client_append_artist_to_queue (self->mpd_client,
                                                self->artist_name, &error);

  if (error != NULL)
  {
    g_warning("Failed to add artist into queue: %s\n", error->message);
    g_error_free (error);
    error = NULL;
  }
}

static void
polyhymnia_artist_page_play_artist_button_clicked (PolyhymniaArtistPage *self,
                                                   GtkButton            *user_data)
{
  GError *error = NULL;

  g_assert (POLYHYMNIA_IS_ARTIST_PAGE (self));

  polyhymnia_mpd_client_play_artist (self->mpd_client, self->artist_name, &error);

  if (error != NULL)
  {
    g_warning("Failed to start playing artist: %s\n", error->message);
    g_error_free (error);
    error = NULL;
  }
}

static void
polyhymnia_artist_page_track_activated (PolyhymniaArtistPage *self,
                                        unsigned int          position,
                                        GtkColumnView        *user_data)
{
  PolyhymniaTrack *track;

  g_assert (POLYHYMNIA_IS_ARTIST_PAGE (self));

  track = g_list_model_get_item (G_LIST_MODEL (self->tracks_model), position);
  g_signal_emit (self, obj_signals[SIGNAL_VIEW_TRACK_DETAILS], 0,
                 polyhymnia_track_get_uri (track));
}

#ifdef POLYHYMNIA_FEATURE_EXTERNAL_DATA
static void
polyhymnia_artist_page_album_show_additional_info (PolyhymniaArtistPage *self,
                                                   PolyhymniaAlbumHeader *user_data)
{
  const gchar *album = polyhymnia_album_header_get_album_title (user_data);
  const gchar *album_musicbrainz_id = polyhymnia_album_header_get_album_musicbrainz_id (user_data);

  g_assert (POLYHYMNIA_IS_ARTIST_PAGE (self));

  adw_dialog_present (ADW_DIALOG (g_object_new (POLYHYMNIA_TYPE_ALBUM_DETAILS_DIALOG,
                                                "album-name", album,
                                                "album-artist-name", self->artist_name,
                                                "album-musicbrainz-id", album_musicbrainz_id,
                                                NULL)),
                      GTK_WIDGET (self));
}
#endif

/* Private function declarations */
static void
polyhymnia_artist_page_get_albums_covers_async (PolyhymniaArtistPage *self,
                                                GPtrArray            *tracks,
                                                GCancellable         *cancellable,
                                                GAsyncReadyCallback    callback,
                                                void                 *user_data)
{
  GTask *task;

  task = g_task_new (self, cancellable, callback, user_data);
  g_task_set_task_data (task, g_ptr_array_ref (tracks),
                        (GDestroyNotify) g_ptr_array_unref);
  g_task_set_source_tag (task, polyhymnia_artist_page_get_albums_covers_async);
  g_task_set_return_on_cancel (task, TRUE);
  g_task_run_in_thread (task, polyhymnia_artist_page_get_albums_covers_async_thread);
  g_object_unref (task);
}

static void
polyhymnia_artist_page_get_albums_covers_async_thread (GTask         *task,
                                                       void          *source_object,
                                                       void          *task_data,
                                                       GCancellable  *cancellable)
{
  GHashTable           *album_covers = g_hash_table_new_full (g_str_hash,
                                                              g_str_equal,
                                                              g_free,
                                                              g_object_unref);
  PolyhymniaArtistPage *self = source_object;
  GPtrArray            *tracks = task_data;
  unsigned int          tracks_count = tracks->len;

  for (unsigned int i = 0; i < tracks_count; i++)
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
polyhymnia_artist_page_get_albums_covers_finish (PolyhymniaArtistPage *self,
                                                 GAsyncResult          *result,
                                                 GError               **error)
{
  g_return_val_if_fail (g_task_is_valid (result, self), NULL);
  return g_task_propagate_pointer (G_TASK (result), error);
}
