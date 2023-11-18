
#include "config.h"

#include "polyhymnia-artist-page.h"

#include "polyhymnia-mpd-client-api.h"

#define _(x) g_dgettext (GETTEXT_PACKAGE, x)

/* Type metadata */
typedef enum
{
  PROP_ARTIST_NAME = 1,
  N_PROPERTIES,
} PolyhymniaArtistPageProperty;

struct _PolyhymniaArtistPage
{
  AdwNavigationPage  parent_instance;

  /* Template widgets */
  AdwToolbarView            *root_toolbar_view;
  GtkScrolledWindow         *content;
  AdwStatusPage             *status_page;

  /* Template objects */
  PolyhymniaMpdClient       *mpd_client;

  /* Instance properties */
  gchar *artist_name;
};

G_DEFINE_FINAL_TYPE (PolyhymniaArtistPage, polyhymnia_artist_page, ADW_TYPE_NAVIGATION_PAGE)

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

/* Event handler declarations */
static void
polyhymnia_artist_page_add_artist_to_queue_button_clicked (PolyhymniaArtistPage *self,
                                                           GtkButton            *user_data);

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

/* Private function declarations */
static void
polyhymnia_artist_page_fill (PolyhymniaArtistPage *self);

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

  g_clear_pointer (&(self->artist_name), g_free);
  adw_navigation_page_set_child (ADW_NAVIGATION_PAGE (self), NULL);
  gtk_widget_dispose_template (GTK_WIDGET (self), POLYHYMNIA_TYPE_ARTIST_PAGE);

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

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/com/github/pamugk/polyhymnia/ui/polyhymnia-artist-page.ui");

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaArtistPage, root_toolbar_view);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaArtistPage, content);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaArtistPage, status_page);

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaArtistPage, mpd_client);

  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_artist_page_add_artist_to_queue_button_clicked);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_artist_page_mpd_database_updated);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_artist_page_mpd_client_initialized);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_artist_page_play_artist_button_clicked);
}

static void
polyhymnia_artist_page_init (PolyhymniaArtistPage *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}

/* Event handler implementations */
static void
polyhymnia_artist_page_mpd_client_initialized (PolyhymniaArtistPage *self,
                                               GParamSpec           *pspec,
                                               PolyhymniaMpdClient  *user_data)
{
  g_assert (POLYHYMNIA_IS_ARTIST_PAGE (self));

  if (polyhymnia_mpd_client_is_initialized (user_data))
  {
    polyhymnia_artist_page_fill (self);
  }
  else
  {
  }
}

static void
polyhymnia_artist_page_mpd_database_updated (PolyhymniaArtistPage *self,
                                             PolyhymniaMpdClient  *user_data)
{
  g_assert (POLYHYMNIA_IS_ARTIST_PAGE (self));

  polyhymnia_artist_page_fill (self);
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

/* Private function declarations */
static void
polyhymnia_artist_page_fill (PolyhymniaArtistPage *self)
{
  GError    *error = NULL;
}
