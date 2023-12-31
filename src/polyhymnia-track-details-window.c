
#include "config.h"

#include "polyhymnia-track-details-window.h"

#include "polyhymnia-mpd-client-details.h"
#include "polyhymnia-mpd-client-images.h"

#define _(x) g_dgettext (GETTEXT_PACKAGE, x)

/* Type metadata */
typedef enum
{
  PROP_TRACK_URI = 1,
  N_PROPERTIES,
} PolyhymniaTrackDetailsWindowProperty;

struct _PolyhymniaTrackDetailsWindow
{
  AdwWindow parent_instance;

  /* Stored UI state */

  /* Template widgets */

  /* Template objects */
  PolyhymniaMpdClient *mpd_client;

  /* Instance properties */
  gchar *track_uri;
};

G_DEFINE_FINAL_TYPE (PolyhymniaTrackDetailsWindow, polyhymnia_track_details_window, ADW_TYPE_WINDOW)

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

/* Event handlers declaration */
static void
polyhymnia_track_details_window_mpd_client_initialized (PolyhymniaTrackDetailsWindow *self,
                                                        GParamSpec                   *pspec,
                                                        PolyhymniaMpdClient          *user_data);

static void
polyhymnia_track_details_window_mpd_database_updated (PolyhymniaTrackDetailsWindow *self,
                                                      PolyhymniaMpdClient          *user_data);

/* Class stuff */
static void
polyhymnia_track_details_window_constructed (GObject *gobject)
{
  PolyhymniaTrackDetailsWindow *self = POLYHYMNIA_TRACK_DETAILS_WINDOW (gobject);

  polyhymnia_track_details_window_mpd_client_initialized (self, NULL, self->mpd_client);

  G_OBJECT_CLASS (polyhymnia_track_details_window_parent_class)->constructed (gobject);
}

static void
polyhymnia_track_details_window_dispose(GObject *gobject)
{
  PolyhymniaTrackDetailsWindow *self = POLYHYMNIA_TRACK_DETAILS_WINDOW (gobject);

  g_clear_pointer (&(self->track_uri), g_free);
  gtk_widget_dispose_template (GTK_WIDGET (self), POLYHYMNIA_TYPE_TRACK_DETAILS_WINDOW);

  G_OBJECT_CLASS (polyhymnia_track_details_window_parent_class)->dispose (gobject);
}

static void
polyhymnia_track_details_window_get_property (GObject    *object,
                                              guint       property_id,
                                              GValue     *value,
                                              GParamSpec *pspec)
{
  PolyhymniaTrackDetailsWindow *self = POLYHYMNIA_TRACK_DETAILS_WINDOW (object);

  switch ((PolyhymniaTrackDetailsWindowProperty) property_id)
    {
    case PROP_TRACK_URI:
      g_value_set_string (value, self->track_uri);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
polyhymnia_track_details_window_set_property (GObject      *object,
                                              guint         property_id,
                                              const GValue *value,
                                              GParamSpec   *pspec)
{
  PolyhymniaTrackDetailsWindow *self = POLYHYMNIA_TRACK_DETAILS_WINDOW (object);

  switch ((PolyhymniaTrackDetailsWindowProperty) property_id)
    {
    case PROP_TRACK_URI:
      g_set_str (&(self->track_uri), g_value_get_string (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
polyhymnia_track_details_window_class_init (PolyhymniaTrackDetailsWindowClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->constructed = polyhymnia_track_details_window_constructed;
  gobject_class->dispose = polyhymnia_track_details_window_dispose;
  gobject_class->get_property = polyhymnia_track_details_window_get_property;
  gobject_class->set_property = polyhymnia_track_details_window_set_property;

  obj_properties[PROP_TRACK_URI] =
    g_param_spec_string ("track-uri",
                         "Track URI",
                         "URI of a track being displayed.",
                         NULL,
                         G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_NAME);

  g_object_class_install_properties (gobject_class,
                                     N_PROPERTIES,
                                     obj_properties);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/com/github/pamugk/polyhymnia/ui/polyhymnia-track-details-window.ui");

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaTrackDetailsWindow, mpd_client);

  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_track_details_window_mpd_client_initialized);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_track_details_window_mpd_database_updated);
}

static void
polyhymnia_track_details_window_init (PolyhymniaTrackDetailsWindow *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}

/* Event handlers implementation */
static void
polyhymnia_track_details_window_mpd_client_initialized (PolyhymniaTrackDetailsWindow *self,
                                                        GParamSpec                   *pspec,
                                                        PolyhymniaMpdClient          *user_data)
{
  g_assert (POLYHYMNIA_IS_TRACK_DETAILS_WINDOW (self));

  if (polyhymnia_mpd_client_is_initialized (user_data))
  {
    polyhymnia_track_details_window_mpd_database_updated (self, user_data);
  }
  else
  {
    gtk_window_close (GTK_WINDOW (self));
  }
}

static void
polyhymnia_track_details_window_mpd_database_updated (PolyhymniaTrackDetailsWindow *self,
                                                      PolyhymniaMpdClient          *user_data)
{
  g_assert (POLYHYMNIA_IS_TRACK_DETAILS_WINDOW (self));
}
