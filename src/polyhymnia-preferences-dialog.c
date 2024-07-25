
#include "app-features.h"
#include "config.h"

#include "polyhymnia-preferences-dialog.h"

#include "polyhymnia-mpd-client-outputs.h"

#define _(x) g_dgettext (GETTEXT_PACKAGE, x)

struct _PolyhymniaPreferencesDialog
{
  AdwPreferencesDialog  parent_instance;

  /* Template widgets */
  AdwPreferencesGroup *audio_outputs_group;

  AdwSwitchRow        *resume_playback_switch;

  AdwSwitchRow        *scan_startup_switch;

#ifdef POLYHYMNIA_FEATURE_EXTERNAL_DATA

  AdwSwitchRow        *additional_info_lastfm_switch;

#ifdef POLYHYMNIA_FEATURE_LYRICS
  AdwSwitchRow        *lyrics_genius_switch;
#endif

#endif

  /* Template objects */
  PolyhymniaMpdClient *mpd_client;
  GSettings           *settings;

  /* UI state */
  GPtrArray           *outputs;
  GPtrArray           *output_rows;
};

G_DEFINE_FINAL_TYPE (PolyhymniaPreferencesDialog, polyhymnia_preferences_dialog, ADW_TYPE_PREFERENCES_DIALOG)

/* Event handler declarations */
static void
polyhymnia_preferences_dialog_mpd_client_initialized (PolyhymniaPreferencesDialog *self,
                                                      GParamSpec                  *pspec,
                                                      PolyhymniaMpdClient         *user_data);

static void
polyhymnia_preferences_dialog_output_state_changed (PolyhymniaPreferencesDialog *self,
                                                    GParamSpec                  *pspec,
                                                    PolyhymniaOutput            *output);

/* Private methods declarations */
static void
polyhymnia_preferences_dialog_clear_outputs (PolyhymniaPreferencesDialog *self);

/* Class stuff */
static void
polyhymnia_preferences_dialog_dispose(GObject *gobject)
{
  PolyhymniaPreferencesDialog *self = POLYHYMNIA_PREFERENCES_DIALOG (gobject);

  polyhymnia_preferences_dialog_clear_outputs (self);
  gtk_widget_dispose_template (GTK_WIDGET (self), POLYHYMNIA_TYPE_PREFERENCES_DIALOG);

  G_OBJECT_CLASS (polyhymnia_preferences_dialog_parent_class)->dispose (gobject);
}

static void
polyhymnia_preferences_dialog_class_init (PolyhymniaPreferencesDialogClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->dispose = polyhymnia_preferences_dialog_dispose;

  gtk_widget_class_set_template_from_resource (widget_class, "/com/github/pamugk/polyhymnia/ui/polyhymnia-preferences-dialog.ui");

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaPreferencesDialog, audio_outputs_group);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaPreferencesDialog, resume_playback_switch);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaPreferencesDialog, scan_startup_switch);

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaPreferencesDialog, mpd_client);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaPreferencesDialog, settings);

  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_preferences_dialog_mpd_client_initialized);
}

static void
polyhymnia_preferences_dialog_init (PolyhymniaPreferencesDialog *self)
{
  gboolean allow_lyrics_configuration = FALSE;

  gtk_widget_init_template (GTK_WIDGET (self));

  g_settings_bind (self->settings, "app-system-resume-playback",
                  self->resume_playback_switch, "active",
                  G_SETTINGS_BIND_DEFAULT);
  g_settings_bind (self->settings, "app-library-scan-startup",
                  self->scan_startup_switch, "active",
                  G_SETTINGS_BIND_DEFAULT);

#ifdef POLYHYMNIA_FEATURE_EXTERNAL_DATA

  self->additional_info_lastfm_switch = ADW_SWITCH_ROW (adw_switch_row_new ());
  adw_action_row_set_subtitle (ADW_ACTION_ROW (self->additional_info_lastfm_switch),
                                  _("Look for additional info on <a href=\"https://www.last.fm/\">Last.fm</a>"));
  adw_preferences_row_set_title (ADW_PREFERENCES_ROW (self->additional_info_lastfm_switch), "Last.fm");

  g_settings_bind (self->settings, "app-external-data-additional-info-lastfm",
                  self->additional_info_lastfm_switch, "active",
                  G_SETTINGS_BIND_DEFAULT);

#ifdef POLYHYMNIA_FEATURE_LYRICS
  allow_lyrics_configuration = TRUE;

  self->lyrics_genius_switch = ADW_SWITCH_ROW (adw_switch_row_new ());
  adw_action_row_set_subtitle (ADW_ACTION_ROW (self->lyrics_genius_switch),
                                  _("Look for lyrics on <a href=\"https://genius.com/\">Genius</a>"));
  adw_preferences_row_set_title (ADW_PREFERENCES_ROW (self->lyrics_genius_switch), "Genius");

  g_settings_bind (self->settings, "app-external-data-lyrics-genius",
                  self->lyrics_genius_switch, "active",
                  G_SETTINGS_BIND_DEFAULT);
#endif

  {
    AdwPreferencesPage *external_data_page = ADW_PREFERENCES_PAGE (adw_preferences_page_new ());
    AdwPreferencesGroup *additional_info_group = ADW_PREFERENCES_GROUP (adw_preferences_group_new ());

    adw_preferences_page_set_description (external_data_page, _("Configure additional info search about albums / artists / songs (like description, biography, lyrics and so on) on the Internet"));
    adw_preferences_page_set_icon_name (external_data_page, "globe-symbolic");
    adw_preferences_page_set_name (external_data_page, "external_data_page");
    adw_preferences_page_set_title (external_data_page, _("External Data"));

    adw_preferences_group_add (additional_info_group, GTK_WIDGET (self->additional_info_lastfm_switch));
    adw_preferences_group_set_description (additional_info_group, _("Additional info search configuration"));
    adw_preferences_group_set_title (additional_info_group, _("Additional info"));

    adw_preferences_page_add (external_data_page, additional_info_group);

    if (allow_lyrics_configuration)
    {
      AdwPreferencesGroup *lyrics_group = ADW_PREFERENCES_GROUP (adw_preferences_group_new ());
      adw_preferences_group_add (lyrics_group, GTK_WIDGET (self->lyrics_genius_switch));
      adw_preferences_group_set_description (lyrics_group, _("Lyrics search configuration"));
      adw_preferences_group_set_title (lyrics_group, _("Lyrics"));

      adw_preferences_page_add (external_data_page, lyrics_group);
    }

    adw_preferences_dialog_add (ADW_PREFERENCES_DIALOG (self),
                                external_data_page);
  }
#endif

  polyhymnia_preferences_dialog_mpd_client_initialized (self, NULL,
                                                        self->mpd_client);
}

/* Event handler implementations */
static void
polyhymnia_preferences_dialog_mpd_client_initialized (PolyhymniaPreferencesDialog *self,
                                                      GParamSpec                  *pspec,
                                                      PolyhymniaMpdClient         *user_data)
{
  g_assert (POLYHYMNIA_IS_PREFERENCES_DIALOG (self));

  polyhymnia_preferences_dialog_clear_outputs (self);
  if (polyhymnia_mpd_client_is_initialized (user_data))
  {
    GError *error = NULL;
    GPtrArray *new_outputs;

    new_outputs = polyhymnia_mpd_client_get_outputs (self->mpd_client, &error);

    if (error == NULL)
    {
      self->output_rows = g_ptr_array_sized_new (new_outputs->len);
      for (guint i = 0; i < new_outputs->len; i++)
      {
        PolyhymniaOutput *output = g_ptr_array_index (new_outputs, i);
        GtkWidget *output_row = adw_switch_row_new ();
        adw_preferences_row_set_title (ADW_PREFERENCES_ROW (output_row),
                                       polyhymnia_output_get_name (output));
        adw_action_row_set_subtitle (ADW_ACTION_ROW (output_row),
                                     polyhymnia_output_get_plugin (output));
        g_object_bind_property (output, "enabled",
                                output_row, "active",
                                G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);
        g_signal_connect_swapped (output, "notify::enabled",
                                  (GCallback) polyhymnia_preferences_dialog_output_state_changed,
                                  self);
        adw_preferences_group_add (self->audio_outputs_group, output_row);
        g_ptr_array_add (self->output_rows, output_row);
      }
      self->outputs = new_outputs;
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

static void
polyhymnia_preferences_dialog_output_state_changed (PolyhymniaPreferencesDialog *self,
                                                    GParamSpec                  *pspec,
                                                    PolyhymniaOutput            *output)
{
  GError *error = NULL;

  g_assert (POLYHYMNIA_IS_PREFERENCES_DIALOG (self));
  g_return_if_fail (POLYHYMNIA_IS_OUTPUT (output));

  polyhymnia_mpd_client_toggle_output (self->mpd_client,
                                       polyhymnia_output_get_id (output),
                                       &error);

  if (error != NULL)
  {
      g_warning("Failed to toggle an audio output state: %s", error->message);
      g_error_free (error);
      error = NULL;
  }
}

/* Private methods implementations */
static void
polyhymnia_preferences_dialog_clear_outputs (PolyhymniaPreferencesDialog *self)
{
  if (self->output_rows != NULL)
  {
    for (int i = 0; i < self->output_rows->len; i++)
    {
      adw_preferences_group_remove (self->audio_outputs_group,
                                    g_ptr_array_index (self->output_rows, i));
    }
    g_ptr_array_unref (self->output_rows);
    self->output_rows = NULL;
  }

  if (self->outputs != NULL)
  {
    g_ptr_array_unref (self->outputs);
    self->outputs = NULL;
  }
}
