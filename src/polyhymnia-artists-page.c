
#include "config.h"

#include "polyhymnia-artists-page.h"

#include "polyhymnia-mpd-client-api.h"

#define _(x) g_dgettext (GETTEXT_PACKAGE, x)

/* Type metadata */
typedef enum
{
  PROP_COLLAPSED_VIEW = 1,
  N_PROPERTIES,
} PolyhymniaArtistsPageProperty;

struct _PolyhymniaArtistsPage
{
  AdwNavigationPage  parent_instance;

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
};

G_DEFINE_FINAL_TYPE (PolyhymniaArtistsPage, polyhymnia_artists_page, ADW_TYPE_NAVIGATION_PAGE)

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

/* Event handler declarations */
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

/* Class stuff */
static void
polyhymnia_artists_page_dispose(GObject *gobject)
{
  PolyhymniaArtistsPage *self = POLYHYMNIA_ARTISTS_PAGE (gobject);

  adw_navigation_page_set_child (ADW_NAVIGATION_PAGE (self), NULL);
  gtk_widget_dispose_template (GTK_WIDGET (self), POLYHYMNIA_TYPE_ARTISTS_PAGE);

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
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

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

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/com/github/pamugk/polyhymnia/ui/polyhymnia-artists-page.ui");

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaArtistsPage, artists_split_view);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaArtistsPage, artist_discography_toolbar_view);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaArtistsPage, artist_discography_navigation_page);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaArtistsPage, artist_discography_scrolled_window);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaArtistsPage, artist_discography_column_view);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaArtistsPage, artist_discography_status_page);

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaArtistsPage, mpd_client);

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaArtistsPage, artist_selection_model);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaArtistsPage, artist_tracks_selection_model);

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
  self->artist_model = g_list_store_new (POLYHYMNIA_TYPE_ARTIST);
  self->artist_tracks_model = g_list_store_new (POLYHYMNIA_TYPE_TRACK);

  gtk_widget_init_template (GTK_WIDGET (self));

  gtk_single_selection_set_model (self->artist_selection_model,
                                  G_LIST_MODEL (self->artist_model));
  gtk_no_selection_set_model (self->artist_tracks_selection_model,
                              G_LIST_MODEL (self->artist_tracks_model));

  polyhymnia_artists_page_mpd_client_initialized (self, NULL, self->mpd_client);
}

/* Event handler functions implementation */
static void
polyhymnia_artists_page_artist_selection_changed (PolyhymniaArtistsPage *self,
                                                  guint                 position,
                                                  guint                 n_items,
                                                  GtkSelectionModel     *user_data)
{
  GtkBitset *selected_artists;

  g_assert (POLYHYMNIA_IS_ARTISTS_PAGE (self));

  selected_artists = gtk_selection_model_get_selection (user_data);

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