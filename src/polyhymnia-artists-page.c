
#include "config.h"

#include "polyhymnia-artists-page.h"

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
  SIGNAL_NAVIGATE = 1,
  N_SIGNALS,
} PolyhymniaArtistsPageSignal;

struct _PolyhymniaArtistsPage
{
  AdwNavigationPage  parent_instance;

  /* Stored UI state */
  GHashTable             *album_covers;

  /* Template widgets */
  AdwNavigationSplitView *artists_split_view;
  AdwNavigationPage      *artist_discography_navigation_page;
  AdwToolbarView         *artist_discography_toolbar_view;
  GtkScrolledWindow      *artist_discography_scrolled_window;
  GtkColumnView          *artist_discography_column_view;
  AdwStatusPage          *artist_discography_status_page;
  AdwStatusPage          *artists_status_page;

  /* Template objects */
  PolyhymniaMpdClient    *mpd_client;
  GListStore             *artist_model;
  GtkSingleSelection     *artist_selection_model;
  GListStore             *artist_tracks_model;
  GtkNoSelection         *artist_tracks_selection_model;
  GtkSortListModel       *artist_tracks_sort_model;
};

G_DEFINE_FINAL_TYPE (PolyhymniaArtistsPage, polyhymnia_artists_page, ADW_TYPE_NAVIGATION_PAGE)

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

static guint obj_signals[N_SIGNALS] = { 0, };

/* Event handler declarations */
static void
polyhymnia_artists_page_album_header_bind (PolyhymniaArtistsPage    *self,
                                           GtkListHeader            *object,
                                           GtkSignalListItemFactory *user_data);

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
polyhymnia_artists_page_artist_clicked (PolyhymniaArtistsPage *self,
                                        guint                 position,
                                        GtkListView           *user_data);

static void
polyhymnia_artists_page_artist_selection_changed (PolyhymniaArtistsPage *self,
                                                  guint               position,
                                                  guint               n_items,
                                                  GtkSelectionModel   *user_data);

static void
polyhymnia_artists_page_mpd_client_initialized (PolyhymniaArtistsPage *self,
                                                GParamSpec            *pspec,
                                                PolyhymniaMpdClient   *user_data);

static void
polyhymnia_artists_page_mpd_database_updated (PolyhymniaArtistsPage *self,
                                              PolyhymniaMpdClient *user_data);

/* Private function declaration */
static void
polyhymnia_artists_page_fill (PolyhymniaArtistsPage *self);

static void
polyhymnia_artists_page_fill_covers (PolyhymniaArtistsPage *self,
                                     GPtrArray             *tracks);

/* Class stuff */
static void
polyhymnia_artists_page_dispose(GObject *gobject)
{
  PolyhymniaArtistsPage *self = POLYHYMNIA_ARTISTS_PAGE (gobject);

  adw_navigation_page_set_child (ADW_NAVIGATION_PAGE (self), NULL);
  gtk_widget_dispose_template (GTK_WIDGET (self), POLYHYMNIA_TYPE_ARTISTS_PAGE);
  g_clear_pointer (&(self->album_covers), g_hash_table_unref);

  G_OBJECT_CLASS (polyhymnia_artists_page_parent_class)->dispose (gobject);
}

static void
polyhymnia_artists_page_get_property (GObject    *object,
                                      guint       property_id,
                                      GValue     *value,
                                      GParamSpec *pspec)
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
                                      guint         property_id,
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

  obj_signals[SIGNAL_NAVIGATE] =
     g_signal_newv ("navigate", type,
                    G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                    NULL, NULL, NULL, NULL,
                    G_TYPE_NONE,
                    1, navigate_param_types);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/com/github/pamugk/polyhymnia/ui/polyhymnia-artists-page.ui");

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaArtistsPage, artists_split_view);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaArtistsPage, artist_discography_toolbar_view);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaArtistsPage, artist_discography_navigation_page);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaArtistsPage, artist_discography_scrolled_window);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaArtistsPage, artist_discography_column_view);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaArtistsPage, artist_discography_status_page);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaArtistsPage, artists_status_page);

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaArtistsPage, mpd_client);

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaArtistsPage, artist_selection_model);
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
}

static void
polyhymnia_artists_page_init (PolyhymniaArtistsPage *self)
{
  self->album_covers = g_hash_table_new_full (g_str_hash, g_str_equal,
                                              g_free, g_object_unref);
  self->artist_model = g_list_store_new (POLYHYMNIA_TYPE_ARTIST);
  self->artist_tracks_model = g_list_store_new (POLYHYMNIA_TYPE_TRACK);

  gtk_widget_init_template (GTK_WIDGET (self));

  gtk_single_selection_set_model (self->artist_selection_model,
                                  G_LIST_MODEL (self->artist_model));
  gtk_sort_list_model_set_model (self->artist_tracks_sort_model,
                                 G_LIST_MODEL (self->artist_tracks_model));

  polyhymnia_artists_page_mpd_client_initialized (self, NULL, self->mpd_client);
}

/* Event handler functions implementation */
static void
polyhymnia_artists_page_album_header_bind (PolyhymniaArtistsPage    *self,
                                           GtkListHeader            *object,
                                           GtkSignalListItemFactory *user_data)
{
  GtkWidget *album_header;
  GtkWidget *album_cover;
  GtkWidget *album_label;
  GtkWidget *date_label;
  PolyhymniaTrack *track;

  g_assert (POLYHYMNIA_IS_ARTISTS_PAGE (self));

  album_header = gtk_list_header_get_child (object);
  track = gtk_list_header_get_item (object);

  if (track != NULL)
  {
    const gchar *album = polyhymnia_track_get_album (track);
    album_cover = gtk_widget_get_first_child (album_header);
    album_label = gtk_widget_get_next_sibling (album_cover);
    date_label = gtk_widget_get_next_sibling (album_label);

    if (album != NULL && g_hash_table_contains (self->album_covers, album))
    {
      gtk_image_set_from_paintable (GTK_IMAGE (album_cover),
                                    g_hash_table_lookup (self->album_covers, album));
    }
    else
    {
      gtk_image_set_from_icon_name (GTK_IMAGE (album_cover), "cd-symbolic");
    }

    gtk_label_set_text (GTK_LABEL (album_label), album);
    gtk_label_set_text (GTK_LABEL (date_label),
                        polyhymnia_track_get_date (track));
  }
}

static void
polyhymnia_artists_page_album_header_setup (PolyhymniaArtistsPage    *self,
                                            GtkListHeader            *object,
                                            GtkSignalListItemFactory *user_data)
{
  GtkBox    *album_header;
  GtkImage  *album_cover;
  GtkWidget *album_label;
  GtkWidget *date_label;

  g_assert (POLYHYMNIA_IS_ARTISTS_PAGE (self));

  album_header = GTK_BOX (gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 16));

  album_cover = GTK_IMAGE (gtk_image_new ());
  gtk_image_set_pixel_size (album_cover, 250);
  album_label = gtk_label_new (NULL);
  gtk_widget_set_valign (album_label, GTK_ALIGN_END);
  date_label = gtk_label_new (NULL);
  gtk_widget_set_valign (date_label, GTK_ALIGN_END);

  gtk_box_append (album_header, GTK_WIDGET (album_cover));
  gtk_box_append (album_header, album_label);
  gtk_box_append (album_header, date_label);

  gtk_list_header_set_child (object, GTK_WIDGET (album_header));
}

static void
polyhymnia_artists_page_album_header_teardown (PolyhymniaArtistsPage    *self,
                                               GtkListHeader            *object,
                                               GtkSignalListItemFactory *user_data)
{
  g_assert (POLYHYMNIA_IS_ARTISTS_PAGE (self));

  gtk_list_header_set_child (object, NULL);
}

static void
polyhymnia_artists_page_album_header_unbind (PolyhymniaArtistsPage    *self,
                                             GtkListHeader            *object,
                                             GtkSignalListItemFactory *user_data)
{
  GtkWidget *album_header;
  GtkWidget *album_cover;
  GtkWidget *album_label;
  GtkWidget *date_label;
  PolyhymniaTrack *track;

  g_assert (POLYHYMNIA_IS_ARTISTS_PAGE (self));

  album_header = gtk_list_header_get_child (object);
  album_cover = gtk_widget_get_first_child (album_header);
  album_label = gtk_widget_get_next_sibling (album_cover);
  date_label = gtk_widget_get_next_sibling (album_label);

  gtk_image_set_from_paintable (GTK_IMAGE (album_cover), NULL);
  gtk_label_set_text (GTK_LABEL (album_label), NULL);
  gtk_label_set_text (GTK_LABEL (date_label), NULL);
}

static void
polyhymnia_artists_page_artist_clicked (PolyhymniaArtistsPage *self,
                                        guint                 position,
                                        GtkListView           *user_data)
{
  PolyhymniaArtist *artist;

  g_assert (POLYHYMNIA_IS_ARTISTS_PAGE (self));

  artist = g_list_model_get_item (G_LIST_MODEL (self->artist_model), position);
  g_signal_emit (self, obj_signals[SIGNAL_NAVIGATE], 0,
                 polyhymnia_artist_get_name (artist));
}

static void
polyhymnia_artists_page_artist_selection_changed (PolyhymniaArtistsPage *self,
                                                  guint                 position,
                                                  guint                 n_items,
                                                  GtkSelectionModel     *user_data)
{
  GtkBitset *selected_artists;

  g_assert (POLYHYMNIA_IS_ARTISTS_PAGE (self));

  selected_artists = gtk_selection_model_get_selection (user_data);

  g_hash_table_remove_all (self->album_covers);
  if (gtk_bitset_get_size (selected_artists) == 0)
  {
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
    GError           *error = NULL;
    PolyhymniaArtist *selected_artist;
    guint            selected_artist_index = gtk_bitset_get_nth (selected_artists, 0);
    GPtrArray        *selected_artist_tracks;
    const gchar      *selected_artist_name;

    selected_artist = g_list_model_get_item (G_LIST_MODEL (self->artist_model),
                                             selected_artist_index);
    selected_artist_name = polyhymnia_artist_get_name (selected_artist);
    adw_navigation_page_set_title (self->artist_discography_navigation_page,
                                   selected_artist_name);
    selected_artist_tracks = polyhymnia_mpd_client_get_artist_discography (self->mpd_client,
                                                                           selected_artist_name,
                                                                           &error);
    adw_navigation_split_view_set_show_content (self->artists_split_view, TRUE);
    if (error != NULL)
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
      g_error_free (error);
      error = NULL;
    }
    else if (selected_artist_tracks->len == 0)
    {
      g_object_set (G_OBJECT (self->artist_discography_status_page),
                    "description", _("If something is missing, try launching library scanning"),
                    "icon-name", "question-round-symbolic",
                    "title", _("No artist albums found"),
                    NULL);
      gtk_scrolled_window_set_child (self->artist_discography_scrolled_window,
                                     GTK_WIDGET(self->artist_discography_status_page));
      g_list_store_remove_all (self->artist_tracks_model);
      g_ptr_array_free (selected_artist_tracks, FALSE);
    }
    else
    {
      polyhymnia_artists_page_fill_covers (self, selected_artist_tracks);
      g_list_store_splice (self->artist_tracks_model, 0,
                            g_list_model_get_n_items (G_LIST_MODEL (self->artist_tracks_model)),
                            selected_artist_tracks->pdata,
                            selected_artist_tracks->len);
      gtk_scrolled_window_set_child (self->artist_discography_scrolled_window,
                                     GTK_WIDGET(self->artist_discography_column_view));
      gtk_column_view_scroll_to (self->artist_discography_column_view, 0, NULL,
                                 GTK_LIST_SCROLL_FOCUS, NULL);
      g_ptr_array_free (selected_artist_tracks, TRUE);
    }
  }
  gtk_bitset_unref (selected_artists);
}

static void
polyhymnia_artists_page_mpd_client_initialized (PolyhymniaArtistsPage *self,
                                                GParamSpec            *pspec,
                                                PolyhymniaMpdClient   *user_data)
{
  g_assert (POLYHYMNIA_IS_ARTISTS_PAGE (self));

  if (polyhymnia_mpd_client_is_initialized (user_data))
  {
    polyhymnia_artists_page_fill (self);
    polyhymnia_artists_page_artist_selection_changed (self, 0, 0,
                                                      GTK_SELECTION_MODEL (self->artist_tracks_selection_model));
  }
  else
  {
    g_hash_table_remove_all (self->album_covers);
    g_list_store_remove_all (self->artist_tracks_model);
    g_list_store_remove_all (self->artist_model);
  }
}

static void
polyhymnia_artists_page_mpd_database_updated (PolyhymniaArtistsPage    *self,
                                              PolyhymniaMpdClient *user_data)
{
  g_assert (POLYHYMNIA_IS_ARTISTS_PAGE (self));

  gtk_selection_model_unselect_all (GTK_SELECTION_MODEL (self->artist_tracks_selection_model));
  polyhymnia_artists_page_fill (self);
}

/* Private function implementation */
static void
polyhymnia_artists_page_fill (PolyhymniaArtistsPage *self)
{
  GError *error = NULL;
  GPtrArray *artists;

  artists = polyhymnia_mpd_client_search_artists (self->mpd_client, &error);
  if (error != NULL)
  {
    g_object_set (G_OBJECT (self->artists_status_page),
                  "description", NULL,
                  "icon-name", "error-symbolic",
                  "title", _("Search for artists failed"),
                  NULL);
    adw_navigation_page_set_child (ADW_NAVIGATION_PAGE (self),
                                   GTK_WIDGET (self->artists_status_page));
    g_warning("Search for artists failed: %s\n", error->message);
    g_error_free (error);
    error = NULL;
  }
  else if (artists->len == 0)
  {
    g_ptr_array_free (artists, FALSE);
    g_object_set (G_OBJECT (self->artists_status_page),
                  "description", _("If something is missing, try launching library scanning"),
                  "icon-name", "question-round-symbolic",
                  "title", _("No artists found"),
                  NULL);
    adw_navigation_page_set_child (ADW_NAVIGATION_PAGE (self),
                                   GTK_WIDGET (self->artists_status_page));
  }
  else
  {
    gtk_selection_model_unselect_all (GTK_SELECTION_MODEL (self->artist_tracks_selection_model));
    g_list_store_splice (self->artist_model, 0,
                          g_list_model_get_n_items (G_LIST_MODEL (self->artist_model)),
                          artists->pdata, artists->len);
    g_ptr_array_free (artists, TRUE);
    adw_navigation_page_set_child (ADW_NAVIGATION_PAGE (self),
                                   GTK_WIDGET (self->artists_split_view));
  }
}

static void
polyhymnia_artists_page_fill_covers (PolyhymniaArtistsPage *self,
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
          g_bytes_unref (cover);
        }
        else
        {
          g_hash_table_insert (self->album_covers,
                               g_strdup (album),
                               album_cover);
        }
      }
    }
  }
}

