
#include "config.h"

#include "polyhymnia-album-page.h"

#include "polyhymnia-mpd-client-api.h"

#define _(x) g_dgettext (GETTEXT_PACKAGE, x)


/* Type metadata */
typedef enum
{
  PROP_ALBUM_TITLE = 1,
  N_PROPERTIES,
} PolyhymniaAlbumPageProperty;

struct _PolyhymniaAlbumPage
{
  AdwNavigationPage  parent_instance;

  /* Template widgets */
  AdwToolbarView            *root_toolbar_view;
  GtkScrolledWindow         *tracks_content;
  GtkColumnView             *tracks_column_view;
  AdwStatusPage             *tracks_status_page;

  /* Template objects */
  PolyhymniaMpdClient       *mpd_client;

  GtkBuilderListItemFactory *disc_header_factory;
  GListStore                *tracks_model;
  GtkNoSelection            *tracks_selection_model;

  /* Instance properties */
  gchar *album_title;
};

G_DEFINE_FINAL_TYPE (PolyhymniaAlbumPage, polyhymnia_album_page, ADW_TYPE_NAVIGATION_PAGE)

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

/* Event handler declarations */
static void
polyhymnia_album_page_mpd_client_initialized (PolyhymniaAlbumPage    *self,
                                              GParamSpec          *pspec,
                                              PolyhymniaMpdClient *user_data);

static void
polyhymnia_album_page_mpd_database_updated (PolyhymniaAlbumPage    *self,
                                            PolyhymniaMpdClient *user_data);

/* Private function declarations */
static gchar *
get_disc_title (GtkListHeader *header, PolyhymniaTrack *item);

static void
polyhymnia_album_page_fill (PolyhymniaAlbumPage *self);

/* Class stuff */
static void
polyhymnia_album_page_constructed (GObject *gobject)
{
  PolyhymniaAlbumPage *self = POLYHYMNIA_ALBUM_PAGE (gobject);

  adw_navigation_page_set_title (ADW_NAVIGATION_PAGE (self), self->album_title);
  polyhymnia_album_page_mpd_client_initialized (self, NULL, self->mpd_client);

  G_OBJECT_CLASS (polyhymnia_album_page_parent_class)->constructed (gobject);
}

static void
polyhymnia_album_page_dispose(GObject *gobject)
{
  PolyhymniaAlbumPage *self = POLYHYMNIA_ALBUM_PAGE (gobject);

  g_clear_pointer (&(self->album_title), g_free);
  adw_navigation_page_set_child (ADW_NAVIGATION_PAGE (self), NULL);
  gtk_widget_dispose_template (GTK_WIDGET (self), POLYHYMNIA_TYPE_ALBUM_PAGE);

  G_OBJECT_CLASS (polyhymnia_album_page_parent_class)->dispose (gobject);
}

static void
polyhymnia_album_page_get_property (GObject    *object,
                                    guint       property_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
  PolyhymniaAlbumPage *self = POLYHYMNIA_ALBUM_PAGE (object);

  switch ((PolyhymniaAlbumPageProperty) property_id)
    {
    case PROP_ALBUM_TITLE:
      g_value_set_string (value, self->album_title);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
polyhymnia_album_page_set_property (GObject      *object,
                                    guint         property_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
  PolyhymniaAlbumPage *self = POLYHYMNIA_ALBUM_PAGE (object);

  switch ((PolyhymniaAlbumPageProperty) property_id)
    {
    case PROP_ALBUM_TITLE:
      g_set_str (&(self->album_title), g_value_get_string (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
polyhymnia_album_page_class_init (PolyhymniaAlbumPageClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->constructed = polyhymnia_album_page_constructed;
  gobject_class->dispose = polyhymnia_album_page_dispose;
  gobject_class->get_property = polyhymnia_album_page_get_property;
  gobject_class->set_property = polyhymnia_album_page_set_property;

  obj_properties[PROP_ALBUM_TITLE] =
    g_param_spec_string ("album-title",
                         "Album title",
                         "Title of a displayed album.",
                         NULL,
                         G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_NAME);

  g_object_class_install_properties (gobject_class,
                                     N_PROPERTIES,
                                     obj_properties);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/com/github/pamugk/polyhymnia/ui/polyhymnia-album-page.ui");

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaAlbumPage, root_toolbar_view);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaAlbumPage, tracks_content);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaAlbumPage, tracks_column_view);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaAlbumPage, tracks_status_page);

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaAlbumPage, disc_header_factory);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaAlbumPage, mpd_client);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaAlbumPage, tracks_selection_model);

  gtk_widget_class_bind_template_callback (widget_class,
                                           get_disc_title);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_album_page_mpd_database_updated);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_album_page_mpd_client_initialized);
}

static void
polyhymnia_album_page_init (PolyhymniaAlbumPage *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  self->tracks_model = g_list_store_new (POLYHYMNIA_TYPE_TRACK);
  gtk_no_selection_set_model (self->tracks_selection_model,
                              G_LIST_MODEL (self->tracks_model));
}

/* Event handler implementations */
static void
polyhymnia_album_page_mpd_client_initialized (PolyhymniaAlbumPage   *self,
                                              GParamSpec            *pspec,
                                              PolyhymniaMpdClient   *user_data)
{
  g_assert (POLYHYMNIA_IS_ALBUM_PAGE (self));

  if (polyhymnia_mpd_client_is_initialized (user_data))
  {
    polyhymnia_album_page_fill (self);
  }
  else
  {
    g_list_store_remove_all (self->tracks_model);
  }
}

static void
polyhymnia_album_page_mpd_database_updated (PolyhymniaAlbumPage    *self,
                                            PolyhymniaMpdClient *user_data)
{
  g_assert (POLYHYMNIA_IS_ALBUM_PAGE (self));

  polyhymnia_album_page_fill (self);
}

/* Private function declarations */
static gchar *
get_disc_title (GtkListHeader *header, PolyhymniaTrack *item)
{
  if (item == NULL)
  {
    return NULL;
  }
  else
  {
    guint disc = polyhymnia_track_get_disc (item);
    return disc == 0
      ? g_strdup (_("Disc  —"))
      : g_strdup_printf (_("Disc %d"), disc);
  }
}

static void
polyhymnia_album_page_fill (PolyhymniaAlbumPage *self)
{
  GError    *error = NULL;
  GPtrArray *tracks;

  tracks = polyhymnia_mpd_client_get_album_tracks (self->mpd_client,
                                                     self->album_title,
                                                     &error);
  if (error != NULL)
  {
    g_list_store_remove_all (self->tracks_model);
    g_object_set (G_OBJECT (self->tracks_status_page),
                  "description", _("Failed to get an album"),
                  NULL);
    gtk_scrolled_window_set_child (self->tracks_content,
                                    GTK_WIDGET (self->tracks_status_page));
    g_warning("Failed to find an album: %s", error->message);
    g_error_free (error);
    error = NULL;
  }
  else if (tracks->len == 0)
  {
    g_ptr_array_free (tracks, TRUE);
    g_list_store_remove_all (self->tracks_model);
    g_object_set (G_OBJECT (self->tracks_status_page),
                  "description", _("Album not found"),
                  NULL);
    gtk_scrolled_window_set_child (self->tracks_content,
                                    GTK_WIDGET (self->tracks_status_page));
  }
  else
  {
    guint last_seen_disc = polyhymnia_track_get_disc (g_ptr_array_index (tracks, 0));
    gboolean multidisc_album = FALSE;
    for (guint i = 1; i < tracks->len; i++)
    {
      guint current_disc = polyhymnia_track_get_disc (g_ptr_array_index (tracks, i));
      multidisc_album = multidisc_album || last_seen_disc != current_disc;
      last_seen_disc = current_disc;
    }
    if (multidisc_album)
    {
      gtk_column_view_set_header_factory (self->tracks_column_view,
                                          GTK_LIST_ITEM_FACTORY (self->disc_header_factory));
    }
    else
    {
      gtk_column_view_set_header_factory (self->tracks_column_view, NULL);
    }

    g_list_store_splice (self->tracks_model, 0,
                          g_list_model_get_n_items (G_LIST_MODEL (self->tracks_model)),
                          tracks->pdata, tracks->len);
    g_ptr_array_free (tracks, TRUE);
    gtk_scrolled_window_set_child (self->tracks_content,
                                   GTK_WIDGET (self->tracks_column_view));
  }
}
