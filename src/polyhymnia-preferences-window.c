
#include "config.h"

#include "polyhymnia-preferences-window.h"

struct _PolyhymniaPreferencesWindow
{
  AdwPreferencesWindow  parent_instance;

  GSettings *settings;

  /* Template widgets */
  GtkSwitch        *resume_playback_switch;

  GtkSwitch        *play_explicit_switch;
  GtkSwitch        *show_explicit_switch;
  GtkSwitch        *scan_startup_switch;
};

G_DEFINE_FINAL_TYPE (PolyhymniaPreferencesWindow, polyhymnia_preferences_window, ADW_TYPE_PREFERENCES_WINDOW)

static void
polyhymnia_preferenes_window_finalize(GObject *gobject)
{
  PolyhymniaPreferencesWindow *self = POLYHYMNIA_PREFERENCES_WINDOW (gobject);

  g_clear_object (&self->settings);

  G_OBJECT_CLASS (polyhymnia_preferences_window_parent_class)->finalize (gobject);
}

static void
polyhymnia_preferences_window_class_init (PolyhymniaPreferencesWindowClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->finalize = polyhymnia_preferenes_window_finalize;

  gtk_widget_class_set_template_from_resource (widget_class, "/com/github/pamugk/polyhymnia/ui/polyhymnia-preferences-window.ui");

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaPreferencesWindow, resume_playback_switch);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaPreferencesWindow, play_explicit_switch);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaPreferencesWindow, show_explicit_switch);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaPreferencesWindow, scan_startup_switch);
}

static void
polyhymnia_preferences_window_init (PolyhymniaPreferencesWindow *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  self->settings = g_settings_new ("com.github.pamugk.polyhymnia");

  g_settings_bind (self->settings, "app-system-resume-playback",
                  self->resume_playback_switch, "active",
                  G_SETTINGS_BIND_DEFAULT);
  g_settings_bind (self->settings, "app-library-explicit-songs",
                  self->play_explicit_switch, "active",
                  G_SETTINGS_BIND_DEFAULT);
  g_settings_bind (self->settings, "app-library-explicit-covers",
                  self->show_explicit_switch, "active",
                  G_SETTINGS_BIND_DEFAULT);
  g_settings_bind (self->settings, "app-library-scan-startup",
                  self->scan_startup_switch, "active",
                  G_SETTINGS_BIND_DEFAULT);
}
