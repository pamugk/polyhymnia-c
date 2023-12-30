
#include "config.h"

#include "polyhymnia-statistics-window.h"

#include "polyhymnia-mpd-client-statistics.h"

#define _(x) g_dgettext (GETTEXT_PACKAGE, x)

/* Type metadata */
struct _PolyhymniaStatisticsWindow
{
  AdwWindow parent_instance;

  /* Template widgets */
  GtkBox              *content;
  AdwToastOverlay     *root_toast_overlay;

  /* Template objects */
  PolyhymniaMpdClient *mpd_client;
};

G_DEFINE_FINAL_TYPE (PolyhymniaStatisticsWindow, polyhymnia_statistics_window, ADW_TYPE_WINDOW)

/* Event handlers declaration */
static void
polyhymnia_statistics_window_mpd_client_initialized (PolyhymniaStatisticsWindow *self,
                                                     GParamSpec                 *pspec,
                                                     PolyhymniaMpdClient        *user_data);

static void
polyhymnia_statistics_window_mpd_database_updated (PolyhymniaStatisticsWindow *self,
                                                   PolyhymniaMpdClient        *user_data);

/* Class stuff */
static void
polyhymnia_statistics_window_dispose(GObject *gobject)
{
  PolyhymniaStatisticsWindow *self = POLYHYMNIA_STATISTICS_WINDOW (gobject);

  gtk_widget_dispose_template (GTK_WIDGET (self), POLYHYMNIA_TYPE_STATISTICS_WINDOW);

  G_OBJECT_CLASS (polyhymnia_statistics_window_parent_class)->dispose (gobject);
}

static void
polyhymnia_statistics_window_class_init (PolyhymniaStatisticsWindowClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->dispose = polyhymnia_statistics_window_dispose;

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/com/github/pamugk/polyhymnia/ui/polyhymnia-statistics-window.ui");

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaStatisticsWindow, content);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaStatisticsWindow, root_toast_overlay);

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaStatisticsWindow, mpd_client);

  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_statistics_window_mpd_client_initialized);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_statistics_window_mpd_database_updated);
}

static void
polyhymnia_statistics_window_init (PolyhymniaStatisticsWindow *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  polyhymnia_statistics_window_mpd_client_initialized (self, NULL, self->mpd_client);
}

/* Event handlers implementation */
static void
polyhymnia_statistics_window_mpd_client_initialized (PolyhymniaStatisticsWindow *self,
                                                     GParamSpec                 *pspec,
                                                     PolyhymniaMpdClient        *user_data)
{
  g_assert (POLYHYMNIA_IS_STATISTICS_WINDOW (self));

  if (polyhymnia_mpd_client_is_initialized (user_data))
  {
    polyhymnia_statistics_window_mpd_database_updated (self, user_data);
  }
  else
  {
    gtk_window_close (GTK_WINDOW (self));
  }
}

static void
polyhymnia_statistics_window_mpd_database_updated (PolyhymniaStatisticsWindow *self,
                                                   PolyhymniaMpdClient        *user_data)
{
  g_assert (POLYHYMNIA_IS_STATISTICS_WINDOW (self));

  adw_toast_overlay_set_child (self->root_toast_overlay, GTK_WIDGET (self->content));
}
