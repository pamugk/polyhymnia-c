
#include "config.h"

#include "polyhymnia-preferences-window.h"

#include "polyhymnia-mpd-client-outputs.h"

struct _PolyhymniaPreferencesWindow
{
  AdwPreferencesWindow  parent_instance;

  /* Template widgets */
  AdwPreferencesGroup *audio_outputs_group;

  AdwSwitchRow        *resume_playback_switch;

  AdwSwitchRow        *play_explicit_switch;
  AdwSwitchRow        *show_explicit_switch;
  AdwSwitchRow        *scan_startup_switch;

  /* Template objects */
  PolyhymniaMpdClient *mpd_client;
  GSettings           *settings;
};

G_DEFINE_FINAL_TYPE (PolyhymniaPreferencesWindow, polyhymnia_preferences_window, ADW_TYPE_PREFERENCES_WINDOW)

/* Event handler declarations */
static void
polyhymnia_preferences_window_mpd_client_initialized (PolyhymniaPreferencesWindow *self,
                                                      GParamSpec                  *pspec,
                                                      PolyhymniaMpdClient         *user_data);

/* Class stuff */
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

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaPreferencesWindow, audio_outputs_group);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaPreferencesWindow, resume_playback_switch);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaPreferencesWindow, play_explicit_switch);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaPreferencesWindow, show_explicit_switch);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaPreferencesWindow, scan_startup_switch);

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaPreferencesWindow, mpd_client);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaPreferencesWindow, settings);

  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_preferences_window_mpd_client_initialized);
}

static void
polyhymnia_preferences_window_init (PolyhymniaPreferencesWindow *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  polyhymnia_preferences_window_mpd_client_initialized (self, NULL,
                                                        self->mpd_client);
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

/* Event handler implementations */
static void
polyhymnia_preferences_window_mpd_client_initialized (PolyhymniaPreferencesWindow *self,
                                                      GParamSpec                  *pspec,
                                                      PolyhymniaMpdClient         *user_data)
{
  g_assert (POLYHYMNIA_IS_PREFERENCES_WINDOW (self));

  if (polyhymnia_mpd_client_is_initialized (user_data))
  {
    GError *error = NULL;
    GPtrArray *outputs;

    outputs = polyhymnia_mpd_client_get_outputs (self->mpd_client, &error);

    if (error == NULL)
    {
      for (guint i = 0; i < outputs->len; i++)
      {
        PolyhymniaOutput *output = g_ptr_array_index (outputs, i);
        GtkWidget *output_row = adw_switch_row_new ();
        adw_preferences_row_set_title (ADW_PREFERENCES_ROW (output_row),
                                       polyhymnia_output_get_name (output));
        adw_action_row_set_subtitle (ADW_ACTION_ROW (output_row),
                                     polyhymnia_output_get_plugin (output));
        adw_switch_row_set_active (ADW_SWITCH_ROW (output_row),
                                   polyhymnia_output_get_enabled (output));
        adw_preferences_group_add (self->audio_outputs_group, output_row);
      }
      g_ptr_array_free (outputs, TRUE);
    }
    else
    {
      g_warning("Failed to get audio outputs: %s", error->message);
      g_error_free (error);
      error = NULL;
    }
  }
  else
  {
  }
}
