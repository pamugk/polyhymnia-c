
#include "config.h"

#include "polyhymnia-preferences-window.h"

struct _PolyhymniaPreferencesWindow
{
  AdwPreferencesWindow  parent_instance;

  /* Template widgets */
  AdwSwitchRow        *resume_playback_switch;

  AdwSwitchRow        *play_explicit_switch;
  AdwSwitchRow        *show_explicit_switch;
  AdwSwitchRow        *scan_startup_switch;

  /* Template objects */
  GSettings *settings;
};

G_DEFINE_FINAL_TYPE (PolyhymniaPreferencesWindow, polyhymnia_preferences_window, ADW_TYPE_PREFERENCES_WINDOW)

static void
polyhymnia_preferences_window_dispose(GObject *gobject)
{
  PolyhymniaPreferencesWindow *self = POLYHYMNIA_PREFERENCES_WINDOW (gobject);

  gtk_widget_dispose_template (GTK_WIDGET (self), POLYHYMNIA_TYPE_PREFERENCES_WINDOW);

  G_OBJECT_CLASS (polyhymnia_preferences_window_parent_class)->dispose (gobject);
}

static void
polyhymnia_preferences_window_finalize(GObject *gobject)
{
  G_OBJECT_CLASS (polyhymnia_preferences_window_parent_class)->finalize (gobject);
}

static void
polyhymnia_preferences_window_class_init (PolyhymniaPreferencesWindowClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->dispose = polyhymnia_preferences_window_dispose;
  gobject_class->finalize = polyhymnia_preferences_window_finalize;

  gtk_widget_class_set_template_from_resource (widget_class, "/com/github/pamugk/polyhymnia/ui/polyhymnia-preferences-window.ui");

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaPreferencesWindow, resume_playback_switch);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaPreferencesWindow, play_explicit_switch);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaPreferencesWindow, show_explicit_switch);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaPreferencesWindow, scan_startup_switch);

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaPreferencesWindow, settings);
}

static void
polyhymnia_preferences_window_init (PolyhymniaPreferencesWindow *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

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
