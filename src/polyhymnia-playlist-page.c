
#include "config.h"

#include "polyhymnia-playlist-page.h"

#include "polyhymnia-mpd-client-playlists.h"
#include "polyhymnia-track.h"

#define _(x) g_dgettext (GETTEXT_PACKAGE, x)

/* Type metadata */
typedef enum
{
  PROP_PLAYLIST_TITLE = 1,
  N_PROPERTIES,
} PolyhymniaPlaylistPageProperty;

struct _PolyhymniaPlaylistPage
{
  AdwNavigationPage  parent_instance;

  /* Template widgets */
  AdwToolbarView            *root_toolbar_view;
  AdwBreakpointBin          *root_content;

  GtkLabel                  *statistics_label;
  GtkLabel                  *duration_label;

  AdwStatusPage             *tracks_status_page;

  /* Template objects */
  PolyhymniaMpdClient       *mpd_client;

  GListStore                *tracks_model;
  GtkNoSelection            *tracks_selection_model;

  /* Instance properties */
  gchar *playlist_title;
};

G_DEFINE_FINAL_TYPE (PolyhymniaPlaylistPage, polyhymnia_playlist_page, ADW_TYPE_NAVIGATION_PAGE)

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

/* Event handler declarations */
static void
polyhymnia_playlist_page_mpd_client_initialized (PolyhymniaPlaylistPage *self,
                                                 GParamSpec          *pspec,
                                                 PolyhymniaMpdClient *user_data);

static void
polyhymnia_playlist_page_mpd_playlists_changed (PolyhymniaPlaylistPage *self,
                                                PolyhymniaMpdClient *user_data);

/* Private function declarations */
static void
polyhymnia_playlist_page_fill (PolyhymniaPlaylistPage *self);

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

  g_clear_pointer (&(self->playlist_title), g_free);
  adw_navigation_page_set_child (ADW_NAVIGATION_PAGE (self), NULL);
  gtk_widget_dispose_template (GTK_WIDGET (self), POLYHYMNIA_TYPE_PLAYLIST_PAGE);

  G_OBJECT_CLASS (polyhymnia_playlist_page_parent_class)->dispose (gobject);
}

static void
polyhymnia_playlist_page_get_property (GObject    *object,
                                       guint       property_id,
                                       GValue     *value,
                                       GParamSpec *pspec)
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
                                       guint         property_id,
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

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/com/github/pamugk/polyhymnia/ui/polyhymnia-playlist-page.ui");

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaPlaylistPage, root_toolbar_view);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaPlaylistPage, root_content);

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaPlaylistPage, statistics_label);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaPlaylistPage, duration_label);

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaPlaylistPage, tracks_status_page);

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaPlaylistPage, mpd_client);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaPlaylistPage, tracks_selection_model);

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
polyhymnia_playlist_page_mpd_client_initialized (PolyhymniaPlaylistPage   *self,
                                                 GParamSpec            *pspec,
                                                 PolyhymniaMpdClient   *user_data)
{
  g_assert (POLYHYMNIA_IS_PLAYLIST_PAGE (self));

  if (polyhymnia_mpd_client_is_initialized (user_data))
  {
    polyhymnia_playlist_page_fill (self);
  }
  else
  {
    g_list_store_remove_all (self->tracks_model);
  }
}

static void
polyhymnia_playlist_page_mpd_playlists_changed (PolyhymniaPlaylistPage *self,
                                                PolyhymniaMpdClient *user_data)
{
  g_assert (POLYHYMNIA_IS_PLAYLIST_PAGE (self));
  polyhymnia_playlist_page_fill (self);
}

/* Private function declarations */
static void
polyhymnia_playlist_page_fill (PolyhymniaPlaylistPage *self)
{
  GError    *error = NULL;
  GPtrArray *tracks;

  tracks = polyhymnia_mpd_client_get_playlist_tracks (self->mpd_client,
                                                      self->playlist_title,
                                                      &error);
  if (error != NULL)
  {
    g_list_store_remove_all (self->tracks_model);
    g_object_set (G_OBJECT (self->tracks_status_page),
                  "description", _("Failed to get a playlist"),
                  "icon-name", NULL,
                  NULL);
    adw_toolbar_view_set_content (self->root_toolbar_view,
                                  GTK_WIDGET (self->tracks_status_page));
    g_warning("Failed to find a playlist: %s", error->message);
    g_error_free (error);
    error = NULL;
  }
  else if (tracks->len == 0)
  {
    g_ptr_array_free (tracks, TRUE);
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
    guint total_duration = 0;
    gchar *total_duration_translated;
    guint hours;
    guint minutes;
    gchar *statistics = g_strdup_printf (g_dngettext(GETTEXT_PACKAGE,
                                                     "%d song", "%d songs",
                                                     tracks->len),
                                         tracks->len);
    for (guint i = 0; i < tracks->len; i++)
    {
      const PolyhymniaTrack *track = g_ptr_array_index (tracks, i);
      total_duration += polyhymnia_track_get_duration (track);
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

    g_list_store_splice (self->tracks_model, 0,
                          g_list_model_get_n_items (G_LIST_MODEL (self->tracks_model)),
                          tracks->pdata, tracks->len);
    g_ptr_array_free (tracks, TRUE);
    adw_toolbar_view_set_content (self->root_toolbar_view,
                                  GTK_WIDGET (self->root_content));
  }
}
