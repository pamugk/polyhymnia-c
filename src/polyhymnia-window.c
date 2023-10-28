
#include "config.h"

#include "polyhymnia-mpd-client.h"
#include "polyhymnia-window.h"

struct _PolyhymniaWindow
{
  AdwApplicationWindow  parent_instance;

  /* Auxiliary objects  */
  PolyhymniaMpdClient *mpd_client;
  GSettings           *settings;

  /* Template widgets */
  GtkButton           *scan_button;
};

G_DEFINE_FINAL_TYPE (PolyhymniaWindow, polyhymnia_window, ADW_TYPE_APPLICATION_WINDOW)

static void
polyhymnia_window_finalize(GObject *gobject)
{
  PolyhymniaWindow *self = POLYHYMNIA_WINDOW (gobject);

  g_clear_object (&self->mpd_client);
  g_clear_object (&self->settings);

  G_OBJECT_CLASS (polyhymnia_window_parent_class)->finalize (gobject);
}

static void
polyhymnia_window_class_init (PolyhymniaWindowClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->finalize = polyhymnia_window_finalize;

  gtk_widget_class_set_template_from_resource (widget_class, "/com/github/pamugk/polyhymnia/ui/polyhymnia-window.ui");

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, scan_button);
}

static void
polyhymnia_window_init (PolyhymniaWindow *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  self->mpd_client = g_object_new (POLYHYMNIA_TYPE_MPD_CLIENT, NULL);
  g_object_bind_property(self->mpd_client, "initialized",
                         self->scan_button, "visible",
                         G_BINDING_DEFAULT | G_BINDING_SYNC_CREATE);

  self->settings = g_settings_new ("com.github.pamugk.polyhymnia");

  g_settings_bind (self->settings, "window-width",
                    self, "default-width",
                    G_SETTINGS_BIND_DEFAULT);
  g_settings_bind (self->settings, "window-height",
                    self, "default-height",
                    G_SETTINGS_BIND_DEFAULT);
  g_settings_bind (self->settings, "window-maximized",
                    self, "maximized",
                    G_SETTINGS_BIND_DEFAULT);
  g_settings_bind (self->settings, "window-fullscreened",
                    self, "fullscreened",
                    G_SETTINGS_BIND_DEFAULT);
}
