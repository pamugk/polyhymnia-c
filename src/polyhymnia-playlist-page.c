
#include "config.h"

#include "polyhymnia-playlist-page.h"

#include "polyhymnia-mpd-client-images.h"
#include "polyhymnia-mpd-client-playlists.h"
#include "polyhymnia-track.h"

#define _(x) g_dgettext (GETTEXT_PACKAGE, x)

/* Type metadata */
typedef enum
{
  PROP_PLAYLIST_TITLE = 1,
  N_PROPERTIES,
} PolyhymniaPlaylistPageProperty;

typedef enum
{
  SIGNAL_ALBUMS_COVERS_READY = 1,
  SIGNAL_DELETED,
  SIGNAL_VIEW_TRACK_DETAILS,
  N_SIGNALS,
} PolyhymniaPlaylistPageSignal;

struct _PolyhymniaPlaylistPage
{
  AdwNavigationPage  parent_instance;

  /* Stored UI state */
  GHashTable          *album_covers;
  GCancellable        *album_covers_cancellable;
  GCancellable        *tracks_cancellable;

  /* Template widgets */
  AdwAlertDialog      *delete_dialog;
  AdwToolbarView      *root_toolbar_view;
  AdwBreakpointBin    *root_content;

  GtkLabel            *statistics_label;
  GtkLabel            *duration_label;

  GtkSpinner          *spinner;
  AdwStatusPage       *tracks_status_page;

  /* Template objects */
  PolyhymniaMpdClient *mpd_client;

  GListStore          *tracks_model;
  GtkNoSelection      *tracks_selection_model;

  /* Instance properties */
  gboolean deleted;
  gchar *playlist_title;
};

G_DEFINE_FINAL_TYPE (PolyhymniaPlaylistPage, polyhymnia_playlist_page, ADW_TYPE_NAVIGATION_PAGE)

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

static unsigned int obj_signals[N_SIGNALS] = { 0, };

/* Event handler declarations */
static void
polyhymnia_playlist_page_add_playlist_to_queue_button_clicked (PolyhymniaPlaylistPage *self,
                                                               GtkButton              *user_data);

static void
polyhymnia_playlist_page_delete_dialog_completed (AdwAlertDialog         *dialog,
                                                  GAsyncResult           *result,
                                                  PolyhymniaPlaylistPage *self);

static void
polyhymnia_playlist_page_delete_playlist_button_clicked (PolyhymniaPlaylistPage *self,
                                                         GtkButton              *user_data);

static void
polyhymnia_playlist_page_get_albums_covers_callback (GObject      *source_object,
                                                     GAsyncResult *result,
                                                     void         *user_data);

static void
polyhymnia_playlist_page_get_playlist_tracks_callback (GObject      *source_object,
                                                       GAsyncResult *result,
                                                       void         *user_data);

static void
polyhymnia_playlist_page_mpd_client_initialized (PolyhymniaPlaylistPage *self,
                                                 GParamSpec             *pspec,
                                                 PolyhymniaMpdClient    *user_data);

static void
polyhymnia_playlist_page_mpd_playlists_changed (PolyhymniaPlaylistPage *self,
                                                PolyhymniaMpdClient    *user_data);

static void
polyhymnia_playlist_page_play_playlist_button_clicked (PolyhymniaPlaylistPage *self,
                                                       GtkButton              *user_data);

static void
polyhymnia_playlist_page_track_activated (PolyhymniaPlaylistPage *self,
                                          unsigned int            position,
                                          GtkColumnView          *user_data);

static void
polyhymnia_playlist_page_track_title_column_bind (PolyhymniaPlaylistPage    *self,
                                                  GtkListItem              *object,
                                                  GtkSignalListItemFactory *user_data);

static void
polyhymnia_playlist_page_track_title_column_covers_ready (PolyhymniaPlaylistPage *self,
                                                          GtkListItem            *object);

static void
polyhymnia_playlist_page_track_title_column_setup (PolyhymniaPlaylistPage   *self,
                                                    GtkListItem              *object,
                                                    GtkSignalListItemFactory *user_data);

static void
polyhymnia_playlist_page_track_title_column_teardown (PolyhymniaPlaylistPage    *self,
                                                      GtkListItem              *object,
                                                      GtkSignalListItemFactory *user_data);

static void
polyhymnia_playlist_page_track_title_column_unbind (PolyhymniaPlaylistPage    *self,
                                                    GtkListItem              *object,
                                                    GtkSignalListItemFactory *user_data);

/* Private function declaration */
static void
polyhymnia_playlist_page_get_albums_covers_async (PolyhymniaPlaylistPage *self,
                                                  GPtrArray              *tracks,
                                                  GCancellable           *cancellable,
                                                  GAsyncReadyCallback     callback,
                                                  void                   *user_data);

static void
polyhymnia_playlist_page_get_albums_covers_async_thread (GTask         *task,
                                                         void          *source_object,
                                                         void          *task_data,
                                                         GCancellable  *cancellable);

static GHashTable *
polyhymnia_playlist_page_get_albums_covers_finish (PolyhymniaPlaylistPage *self,
                                                   GAsyncResult           *result,
                                                   GError                **error);

/* Class stuff */
static void
polyhymnia_playlist_page_constructed (GObject *gobject)
{
  PolyhymniaPlaylistPage *self = POLYHYMNIA_PLAYLIST_PAGE (gobject);

  adw_navigation_page_set_title (ADW_NAVIGATION_PAGE (self), self->playlist_title);
  polyhymnia_playlist_page_mpd_client_initialized (self, NULL, self->mpd_client);

  G_OBJECT_CLASS (polyhymnia_playlist_page_parent_class)->constructed (gobject);
}

static void
polyhymnia_playlist_page_dispose(GObject *gobject)
{
  PolyhymniaPlaylistPage *self = POLYHYMNIA_PLAYLIST_PAGE (gobject);

  g_cancellable_cancel (self->album_covers_cancellable);
  g_cancellable_cancel (self->tracks_cancellable);
  adw_navigation_page_set_child (ADW_NAVIGATION_PAGE (self), NULL);
  gtk_widget_dispose_template (GTK_WIDGET (self), POLYHYMNIA_TYPE_PLAYLIST_PAGE);
  g_clear_pointer (&(self->playlist_title), g_free);
  g_clear_pointer (&(self->album_covers), g_hash_table_unref);

  G_OBJECT_CLASS (polyhymnia_playlist_page_parent_class)->dispose (gobject);
}

static void
polyhymnia_playlist_page_get_property (GObject     *object,
                                       unsigned int property_id,
                                       GValue      *value,
                                       GParamSpec  *pspec)
{
  PolyhymniaPlaylistPage *self = POLYHYMNIA_PLAYLIST_PAGE (object);

  switch ((PolyhymniaPlaylistPageProperty) property_id)
    {
    case PROP_PLAYLIST_TITLE:
      g_value_set_string (value, self->playlist_title);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
polyhymnia_playlist_page_set_property (GObject      *object,
                                       unsigned int  property_id,
                                       const GValue *value,
                                       GParamSpec   *pspec)
{
  PolyhymniaPlaylistPage *self = POLYHYMNIA_PLAYLIST_PAGE (object);

  switch ((PolyhymniaPlaylistPageProperty) property_id)
    {
    case PROP_PLAYLIST_TITLE:
      g_set_str (&(self->playlist_title), g_value_get_string (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
polyhymnia_playlist_page_class_init (PolyhymniaPlaylistPageClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GType type = G_TYPE_FROM_CLASS (gobject_class);
  GType view_detail_types[] = { G_TYPE_STRING };

  gobject_class->constructed = polyhymnia_playlist_page_constructed;
  gobject_class->dispose = polyhymnia_playlist_page_dispose;
  gobject_class->get_property = polyhymnia_playlist_page_get_property;
  gobject_class->set_property = polyhymnia_playlist_page_set_property;

  obj_properties[PROP_PLAYLIST_TITLE] =
    g_param_spec_string ("playlist-title",
                         "Playlist title",
                         "Title of a displayed playlist.",
                         NULL,
                         G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_NAME);

  g_object_class_install_properties (gobject_class,
                                     N_PROPERTIES,
                                     obj_properties);

  obj_signals[SIGNAL_ALBUMS_COVERS_READY] =
     g_signal_newv ("albums-covers-ready", type,
                    G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                    NULL, NULL, NULL, NULL, G_TYPE_NONE, 0, NULL);
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

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/com/github/pamugk/polyhymnia/ui/polyhymnia-playlist-page.ui");

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaPlaylistPage, delete_dialog);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaPlaylistPage, root_toolbar_view);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaPlaylistPage, root_content);

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaPlaylistPage, statistics_label);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaPlaylistPage, duration_label);

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaPlaylistPage, spinner);

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaPlaylistPage, tracks_status_page);

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaPlaylistPage, mpd_client);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaPlaylistPage, tracks_selection_model);

  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_playlist_page_add_playlist_to_queue_button_clicked);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_playlist_page_delete_playlist_button_clicked);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_playlist_page_play_playlist_button_clicked);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_playlist_page_track_activated);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_playlist_page_track_title_column_bind);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_playlist_page_track_title_column_setup);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_playlist_page_track_title_column_teardown);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_playlist_page_track_title_column_unbind);

  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_playlist_page_mpd_client_initialized);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_playlist_page_mpd_playlists_changed);
}

static void
polyhymnia_playlist_page_init (PolyhymniaPlaylistPage *self)
{
  self->tracks_model = g_list_store_new (POLYHYMNIA_TYPE_TRACK);
  gtk_widget_init_template (GTK_WIDGET (self));

  gtk_no_selection_set_model (self->tracks_selection_model,
                              G_LIST_MODEL (self->tracks_model));
}

/* Event handler implementations */
static void
polyhymnia_playlist_page_add_playlist_to_queue_button_clicked (PolyhymniaPlaylistPage *self,
                                                               GtkButton              *user_data)
{
  GError *error = NULL;

  g_assert (POLYHYMNIA_IS_PLAYLIST_PAGE (self));

  polyhymnia_mpd_client_append_playlist_to_queue (self->mpd_client,
                                                  self->playlist_title, &error);

  if (error != NULL)
  {
    g_warning("Failed to add playlist into queue: %s\n", error->message);
    g_error_free (error);
    error = NULL;
  }
}

static void
polyhymnia_playlist_page_delete_dialog_completed (AdwAlertDialog         *dialog,
                                                  GAsyncResult           *result,
                                                  PolyhymniaPlaylistPage *self)
{
  const char *response = adw_alert_dialog_choose_finish (dialog, result);

  if (g_strcmp0 (response, "delete") == 0)
  {
    GError *error = NULL;
    polyhymnia_mpd_client_delete_playlist (self->mpd_client,
                                           self->playlist_title, &error);
    if (error == NULL)
    {
      g_signal_emit (self, obj_signals[SIGNAL_DELETED], 0);
      self->deleted = TRUE;
    } else {
      g_warning("Failed to delete a playlist: %s", error->message);
      g_error_free (error);
      error = NULL;
    }
  }
}

static void
polyhymnia_playlist_page_delete_playlist_button_clicked (PolyhymniaPlaylistPage *self,
                                                         GtkButton              *user_data)
{
  g_assert (POLYHYMNIA_IS_PLAYLIST_PAGE (self));
  adw_alert_dialog_choose (self->delete_dialog, GTK_WIDGET (self), NULL,
                           (GAsyncReadyCallback) polyhymnia_playlist_page_delete_dialog_completed,
                           self);
}

static void
polyhymnia_playlist_page_get_albums_covers_callback (GObject      *source_object,
                                                     GAsyncResult *result,
                                                     void         *user_data)
{
  GHashTable             *album_covers;
  GError                 *error = NULL;
  PolyhymniaPlaylistPage *self = POLYHYMNIA_PLAYLIST_PAGE (source_object);

  album_covers = polyhymnia_playlist_page_get_albums_covers_finish (self, result,
                                                                    &error);

  if (error == NULL)
  {
    self->album_covers = album_covers;
    g_signal_emit (self, obj_signals[SIGNAL_ALBUMS_COVERS_READY], 0);
  }

  g_clear_object (&(self->album_covers_cancellable));
}

static void
polyhymnia_playlist_page_get_playlist_tracks_callback (GObject      *source_object,
                                                       GAsyncResult *result,
                                                       void         *user_data)
{
  GError                 *error = NULL;
  PolyhymniaMpdClient    *mpd_client = POLYHYMNIA_MPD_CLIENT (source_object);
  PolyhymniaPlaylistPage *self = user_data;
  GPtrArray              *tracks;

  tracks = polyhymnia_mpd_client_get_playlist_tracks_finish (mpd_client,
                                                             result,
                                                             &error);
  if (error == NULL)
  {
    if (tracks->len == 0)
    {
      g_list_store_remove_all (self->tracks_model);
      g_object_set (G_OBJECT (self->tracks_status_page),
                    "description", _("Playlist is empty"),
                    "icon-name", "playlist2-symbolic",
                    NULL);
      adw_toolbar_view_set_content (self->root_toolbar_view,
                                    GTK_WIDGET (self->tracks_status_page));
    }
    else
    {
      unsigned int total_duration = 0;
      char        *total_duration_translated;
      unsigned int hours;
      unsigned int minutes;
      char        *statistics = g_strdup_printf (g_dngettext (GETTEXT_PACKAGE, "%d song", "%d songs", tracks->len),
                                                 tracks->len);
      for (unsigned int i = 0; i < tracks->len; i++)
      {
        total_duration += polyhymnia_track_get_duration (g_ptr_array_index (tracks, i));
      }

      minutes = (total_duration % 3600) / 60;
      hours = total_duration / 3600;
      if (hours > 0)
      {
        total_duration_translated = g_strdup_printf (_("%d h. %d min."),
                                                     hours, minutes);
      }
      else
      {
        total_duration_translated = g_strdup_printf (_("%d min."), minutes);
      }

      gtk_label_set_text (self->statistics_label, statistics);
      gtk_label_set_text (self->duration_label, total_duration_translated);
      g_free (total_duration_translated);
      g_free (statistics);

      self->album_covers_cancellable = g_cancellable_new ();
      polyhymnia_playlist_page_get_albums_covers_async (self, tracks,
                                                        self->album_covers_cancellable,
                                                        polyhymnia_playlist_page_get_albums_covers_callback,
                                                        NULL);
      g_list_store_splice (self->tracks_model, 0,
                            g_list_model_get_n_items (G_LIST_MODEL (self->tracks_model)),
                            tracks->pdata, tracks->len);
      adw_toolbar_view_set_content (self->root_toolbar_view,
                                    GTK_WIDGET (self->root_content));
    }
    g_ptr_array_unref (tracks);
  }
  else if (!g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
  {
    g_list_store_remove_all (self->tracks_model);
    g_object_set (G_OBJECT (self->tracks_status_page),
                  "description", _("Failed to get a playlist"),
                  "icon-name", NULL,
                  NULL);
    adw_toolbar_view_set_content (self->root_toolbar_view,
                                  GTK_WIDGET (self->tracks_status_page));
    g_warning ("Failed to find a playlist: %s", error->message);
  }

  gtk_spinner_stop (self->spinner);
  g_clear_object (&(self->tracks_cancellable));
}

static void
polyhymnia_playlist_page_mpd_client_initialized (PolyhymniaPlaylistPage *self,
                                                 GParamSpec             *pspec,
                                                 PolyhymniaMpdClient    *user_data)
{
  g_assert (POLYHYMNIA_IS_PLAYLIST_PAGE (self));

  if (polyhymnia_mpd_client_is_initialized (user_data))
  {
    polyhymnia_playlist_page_mpd_playlists_changed (self, user_data);
  }
  else
  {
    g_cancellable_cancel (self->album_covers_cancellable);
    g_cancellable_cancel (self->tracks_cancellable);
    g_list_store_remove_all (self->tracks_model);
    g_clear_pointer (&(self->album_covers), g_hash_table_unref);
  }
}

static void
polyhymnia_playlist_page_mpd_playlists_changed (PolyhymniaPlaylistPage *self,
                                                PolyhymniaMpdClient    *user_data)
{
  g_assert (POLYHYMNIA_IS_PLAYLIST_PAGE (self));
  if (!self->deleted && self->tracks_cancellable == NULL)
  {
    g_clear_pointer (&(self->album_covers), g_hash_table_unref);
    g_cancellable_cancel (self->album_covers_cancellable);
    self->tracks_cancellable = g_cancellable_new ();
    polyhymnia_mpd_client_get_playlist_tracks_async (user_data,
                                                     self->playlist_title,
                                                     self->tracks_cancellable,
                                                     polyhymnia_playlist_page_get_playlist_tracks_callback,
                                                     self);

    adw_toolbar_view_set_content (self->root_toolbar_view,
                                  GTK_WIDGET (self->spinner));
    gtk_spinner_start (self->spinner);
  }
}

static void
polyhymnia_playlist_page_play_playlist_button_clicked (PolyhymniaPlaylistPage *self,
                                                       GtkButton              *user_data)
{
  GError *error = NULL;

  g_assert (POLYHYMNIA_IS_PLAYLIST_PAGE (self));

  polyhymnia_mpd_client_play_playlist (self->mpd_client,
                                       self->playlist_title, &error);

  if (error != NULL)
  {
    g_warning ("Failed to start playing playlist: %s\n", error->message);
    g_error_free (error);
    error = NULL;
  }
}

static void
polyhymnia_playlist_page_track_activated (PolyhymniaPlaylistPage *self,
                                          unsigned int            position,
                                          GtkColumnView          *user_data)
{
  PolyhymniaTrack *track;

  g_assert (POLYHYMNIA_IS_PLAYLIST_PAGE (self));

  track = g_list_model_get_item (G_LIST_MODEL (self->tracks_model), position);
  g_signal_emit (self, obj_signals[SIGNAL_VIEW_TRACK_DETAILS], 0,
                 polyhymnia_track_get_uri (track));
}

static void
polyhymnia_playlist_page_track_title_column_bind (PolyhymniaPlaylistPage   *self,
                                                  GtkListItem              *object,
                                                  GtkSignalListItemFactory *user_data)
{
  const char      *album;
  PolyhymniaTrack *track;

  GtkWidget       *album_cover;
  GtkWidget       *track_root;
  GtkWidget       *track_label;

  g_assert (POLYHYMNIA_IS_PLAYLIST_PAGE (self));

  track_root = gtk_list_item_get_child (object);
  album_cover = gtk_widget_get_first_child (track_root);
  track_label = gtk_widget_get_next_sibling (album_cover);

  track = gtk_list_item_get_item (object);

  album = polyhymnia_track_get_album (track);
  if (album != NULL && self->album_covers != NULL
      && g_hash_table_contains (self->album_covers, album))
  {
    gtk_image_set_from_paintable (GTK_IMAGE (album_cover),
                                  g_hash_table_lookup (self->album_covers, album));
  }
  else
  {
    gtk_image_set_from_icon_name (GTK_IMAGE (album_cover),
                                  "image-missing-symbolic");
  }
  gtk_label_set_text (GTK_LABEL (track_label),
                      polyhymnia_track_get_title (track));
}

static void
polyhymnia_playlist_page_track_title_column_covers_ready (PolyhymniaPlaylistPage *self,
                                                          GtkListItem            *object)
{
  const char      *album;
  PolyhymniaTrack *track;

  GtkWidget       *album_cover;
  GtkWidget       *track_root;

  g_assert (POLYHYMNIA_IS_PLAYLIST_PAGE (self));

  track_root = gtk_list_item_get_child (object);
  album_cover = gtk_widget_get_first_child (track_root);

  track = gtk_list_item_get_item (object);

  album = polyhymnia_track_get_album (track);
  if (album != NULL && self->album_covers != NULL
      && g_hash_table_contains (self->album_covers, album))
  {
    gtk_image_set_from_paintable (GTK_IMAGE (album_cover),
                                  g_hash_table_lookup (self->album_covers, album));
  }
  else
  {
    gtk_image_set_from_icon_name (GTK_IMAGE (album_cover),
                                  "image-missing-symbolic");
  }
}

static void
polyhymnia_playlist_page_track_title_column_setup (PolyhymniaPlaylistPage    *self,
                                                   GtkListItem              *object,
                                                   GtkSignalListItemFactory *user_data)
{
  GtkBox    *track_root;
  GtkImage  *album_cover;
  GtkLabel  *track_label;

  g_assert (POLYHYMNIA_IS_PLAYLIST_PAGE (self));

  track_root = GTK_BOX (gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 20));

  album_cover = GTK_IMAGE (gtk_image_new ());
  gtk_image_set_pixel_size (album_cover, 50);
  gtk_widget_set_valign (GTK_WIDGET (album_cover), GTK_ALIGN_CENTER);

  track_label = GTK_LABEL (gtk_label_new (NULL));
  gtk_label_set_ellipsize (track_label, PANGO_ELLIPSIZE_END);
  gtk_label_set_xalign (track_label, 0);

  gtk_box_append (track_root, GTK_WIDGET (album_cover));
  gtk_box_append (track_root, GTK_WIDGET (track_label));

  gtk_list_item_set_child (object, GTK_WIDGET (track_root));

  g_signal_connect (self, "albums-covers-ready",
                    G_CALLBACK (polyhymnia_playlist_page_track_title_column_covers_ready),
                    object);
}

static void
polyhymnia_playlist_page_track_title_column_teardown (PolyhymniaPlaylistPage   *self,
                                                      GtkListItem              *object,
                                                      GtkSignalListItemFactory *user_data)
{
  g_assert (POLYHYMNIA_IS_PLAYLIST_PAGE (self));
  g_signal_handlers_disconnect_by_data (self, object);
}

static void
polyhymnia_playlist_page_track_title_column_unbind (PolyhymniaPlaylistPage   *self,
                                                    GtkListItem              *object,
                                                    GtkSignalListItemFactory *user_data)
{
  GtkWidget *track_root;
  GtkWidget *album_cover;

  g_assert (POLYHYMNIA_IS_PLAYLIST_PAGE (self));

  track_root = gtk_list_item_get_child (object);
  album_cover = gtk_widget_get_first_child (track_root);
  gtk_image_set_from_paintable (GTK_IMAGE (album_cover), NULL);
  gtk_label_set_text (GTK_LABEL (gtk_widget_get_next_sibling (album_cover)), NULL);
}

/* Private function declaration */
static void
polyhymnia_playlist_page_get_albums_covers_async (PolyhymniaPlaylistPage *self,
                                                  GPtrArray              *tracks,
                                                  GCancellable           *cancellable,
                                                  GAsyncReadyCallback     callback,
                                                  void                   *user_data)
{
  GTask *task;

  task = g_task_new (self, cancellable, callback, user_data);
  g_task_set_task_data (task, g_ptr_array_ref (tracks),
                        (GDestroyNotify) g_ptr_array_unref);
  g_task_set_source_tag (task, polyhymnia_playlist_page_get_albums_covers_async);
  g_task_set_return_on_cancel (task, TRUE);
  g_task_run_in_thread (task, polyhymnia_playlist_page_get_albums_covers_async_thread);
  g_object_unref (task);
}

static void
polyhymnia_playlist_page_get_albums_covers_async_thread (GTask         *task,
                                                         void          *source_object,
                                                         void          *task_data,
                                                         GCancellable  *cancellable)
{
  GHashTable             *album_covers = g_hash_table_new_full (g_str_hash,
                                                                g_str_equal,
                                                                g_free,
                                                                g_object_unref);
  PolyhymniaPlaylistPage *self = source_object;
  GPtrArray              *tracks = task_data;
  for (unsigned int i = 0; i < tracks->len; i++)
  {
    const PolyhymniaTrack *track = g_ptr_array_index (tracks, i);
    const char            *album = polyhymnia_track_get_album (track);
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
      }
      else if (cover != NULL)
      {
        GdkTexture *album_cover;
        album_cover = gdk_texture_new_from_bytes (cover, &error);
        if (error != NULL)
        {
          g_warning ("Failed to convert album cover: %s\n", error->message);
          g_error_free (error);
        }
        else
        {
          g_hash_table_insert (album_covers, g_strdup (album), album_cover);
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
polyhymnia_playlist_page_get_albums_covers_finish (PolyhymniaPlaylistPage *self,
                                                   GAsyncResult           *result,
                                                   GError                **error)
{
  g_return_val_if_fail (g_task_is_valid (result, self), NULL);
  return g_task_propagate_pointer (G_TASK (result), error);
}
