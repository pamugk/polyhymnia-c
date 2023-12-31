
#include "config.h"

#include "polyhymnia-application.h"
#include "polyhymnia-mpd-client-core.h"
#include "polyhymnia-player.h"
#include "polyhymnia-preferences-window.h"
#include "polyhymnia-statistics-window.h"
#include "polyhymnia-track-details-window.h"
#include "polyhymnia-window.h"

#define _(x) g_dgettext (GETTEXT_PACKAGE, x)

struct _PolyhymniaApplication
{
  AdwApplication parent_instance;

  PolyhymniaMpdClient *mpd_client;
  PolyhymniaPlayer    *player;
};

G_DEFINE_TYPE (PolyhymniaApplication, polyhymnia_application, ADW_TYPE_APPLICATION)

PolyhymniaApplication *
polyhymnia_application_new (const char        *application_id,
                            GApplicationFlags  flags)
{
  g_return_val_if_fail (application_id != NULL, NULL);

  return g_object_new (POLYHYMNIA_TYPE_APPLICATION,
	                "application-id", application_id,
	                "flags", flags,
	                NULL);
}

/* Event handlers declaration */
static void
polyhymnia_application_mpd_initialized (PolyhymniaApplication *self,
                                        GParamSpec            *pspec,
                                        PolyhymniaMpdClient   *user_data);

/* Class stuff - startup & shutdown callbacks, etc */
static void
polyhymnia_application_activate (GApplication *app)
{
  GtkWindow *window;

  g_assert (POLYHYMNIA_IS_APPLICATION (app));

  window = gtk_application_get_active_window (GTK_APPLICATION (app));

  if (window == NULL)
  {
    window = g_object_new (POLYHYMNIA_TYPE_WINDOW, "application", app, NULL);
  }

  gtk_window_present (window);
}

static void
polyhymnia_application_shutdown (GApplication *app)
{
  PolyhymniaApplication *self;

  g_assert (POLYHYMNIA_IS_APPLICATION (app));

  self = POLYHYMNIA_APPLICATION (app);
  g_clear_object (&(self->mpd_client));
  g_clear_object (&(self->player));

  G_APPLICATION_CLASS (polyhymnia_application_parent_class)->shutdown (app);
}

static void
polyhymnia_application_startup (GApplication *app)
{
  PolyhymniaApplication *self;

  G_APPLICATION_CLASS (polyhymnia_application_parent_class)->startup (app);
  g_assert (POLYHYMNIA_IS_APPLICATION (app));

  self = POLYHYMNIA_APPLICATION (app);

  self->mpd_client = g_object_new (POLYHYMNIA_TYPE_MPD_CLIENT, NULL);
  polyhymnia_application_mpd_initialized (self, NULL, self->mpd_client);
  g_signal_connect_swapped (self->mpd_client, "notify::initialized",
                            G_CALLBACK (polyhymnia_application_mpd_initialized),
                            self);

  self->player = g_object_new (POLYHYMNIA_TYPE_PLAYER, NULL);
}

static void
polyhymnia_application_class_init (PolyhymniaApplicationClass *klass)
{
  GApplicationClass *app_class = G_APPLICATION_CLASS (klass);

  app_class->activate = polyhymnia_application_activate;
  app_class->shutdown = polyhymnia_application_shutdown;
  app_class->startup = polyhymnia_application_startup;
}

/* Actions */
static void
polyhymnia_application_about_action (GSimpleAction *action,
                                     GVariant      *parameter,
                                     gpointer       user_data)
{
  static const char *developers[] = {"pamugk", NULL};
  PolyhymniaApplication *self = user_data;
  GtkWindow *window = NULL;

  g_assert (POLYHYMNIA_IS_APPLICATION (self));

  window = gtk_application_get_active_window (GTK_APPLICATION (self));

  adw_show_about_window (window,
	                 "application-name", "Polyhymnia",
	                 "application-icon", "com.github.pamugk.polyhymnia",
                         "comments", _("Simple MPD-based music player."),
	                 "developer-name", "pamugk",
	                 "version", "0.1.0",
                         "website", "https://github.com/pamugk/polyhymnia-c",
	                 "developers", developers,
                         "issue-url", "https://github.com/pamugk/polyhymnia-c/issues/new/choose",
	                 "copyright", "© 2023 pamugk",
                         "license-type", GTK_LICENSE_GPL_3_0,
	                 NULL);
}

static void
polyhymnia_application_quit_action (GSimpleAction *action,
                                    GVariant      *parameter,
                                    gpointer       user_data)
{
  PolyhymniaApplication *self = user_data;

  g_assert (POLYHYMNIA_IS_APPLICATION (self));

  g_application_quit (G_APPLICATION (self));
}

static void
polyhymnia_application_preferences_action (GSimpleAction *action,
                                           GVariant      *parameter,
                                           gpointer       user_data)
{
  PolyhymniaApplication *self = user_data;
  GtkWindow *window = NULL;
  GtkWindow *preferences_window = NULL;

  g_assert (POLYHYMNIA_IS_APPLICATION (self));

  window = gtk_application_get_active_window (GTK_APPLICATION (self));

  preferences_window = g_object_new (POLYHYMNIA_TYPE_PREFERENCES_WINDOW,
	                              "application", self,
                                      "transient-for", window,
		                      NULL);
  gtk_window_set_modal (preferences_window, TRUE);

  gtk_window_present (preferences_window);
}

static void
polyhymnia_application_reconnect_action (GSimpleAction *action,
                                         GVariant      *parameter,
                                         gpointer       user_data)
{
  GError *error = NULL;
  PolyhymniaApplication *self = user_data;

  g_assert (POLYHYMNIA_IS_APPLICATION (self));

  polyhymnia_mpd_client_connect (self->mpd_client, &error);
  if (error != NULL)
  {
    g_warning("MPD client failed to reconnect: %s\n",
              error->message);
    g_error_free (error);
  }
}

static void
polyhymnia_application_rescan_action (GSimpleAction *action,
                                      GVariant      *parameter,
                                      gpointer       user_data)
{
  GError *error = NULL;
  PolyhymniaApplication *self = user_data;

  g_assert (POLYHYMNIA_IS_APPLICATION (self));

  polyhymnia_mpd_client_rescan (self->mpd_client, &error);
  if (error != NULL)
  {
    g_warning("An error occurred on library full rescan request: %s\n", error->message);
    g_error_free (error);
  }
}

static void
polyhymnia_application_scan_action (GSimpleAction *action,
                                    GVariant      *parameter,
                                    gpointer       user_data)
{
  GError *error = NULL;
  PolyhymniaApplication *self = user_data;

  g_assert (POLYHYMNIA_IS_APPLICATION (self));

  polyhymnia_mpd_client_scan (self->mpd_client, &error);
  if (error != NULL)
  {
    g_warning("An error occurred on library scan request: %s\n", error->message);
    g_error_free (error);
  }
}

static void
polyhymnia_application_statistics_action (GSimpleAction *action,
                                          GVariant      *parameter,
                                          gpointer       user_data)
{
  PolyhymniaApplication *self = user_data;
  GtkWindow *window = NULL;
  GtkWindow *statistics_window = NULL;

  g_assert (POLYHYMNIA_IS_APPLICATION (self));

  window = gtk_application_get_active_window (GTK_APPLICATION (self));

  statistics_window = g_object_new (POLYHYMNIA_TYPE_STATISTICS_WINDOW,
                                    "application", self,
                                    "transient-for", window,
                                    NULL);
  gtk_window_set_modal (statistics_window, TRUE);

  gtk_window_present (statistics_window);
}

static void
polyhymnia_application_track_details_action (GSimpleAction *action,
                                             GVariant      *parameter,
                                             gpointer       user_data)
{
  PolyhymniaApplication *self = user_data;
  GtkWindow *window;
  GtkWindow *track_details_window;

  g_assert (POLYHYMNIA_IS_APPLICATION (self));

  window = gtk_application_get_active_window (GTK_APPLICATION (self));

  track_details_window = g_object_new (POLYHYMNIA_TYPE_TRACK_DETAILS_WINDOW,
                                       "application", self,
                                       "track-uri", g_variant_get_string (parameter, NULL),
                                       "transient-for", window,
                                       NULL);
  gtk_window_set_modal (track_details_window, TRUE);

  gtk_window_present (track_details_window);
}

static const GActionEntry app_actions[] = {
  { "about", polyhymnia_application_about_action },
  { "preferences", polyhymnia_application_preferences_action },
  { "quit", polyhymnia_application_quit_action },
  { "reconnect", polyhymnia_application_reconnect_action },
  { "rescan", polyhymnia_application_rescan_action },
  { "scan", polyhymnia_application_scan_action },
  { "statistics", polyhymnia_application_statistics_action },
  { "track-details", polyhymnia_application_track_details_action, "s" },
};

static void
polyhymnia_application_init (PolyhymniaApplication *self)
{
  g_action_map_add_action_entries (G_ACTION_MAP (self),
	                            app_actions,
	                            G_N_ELEMENTS (app_actions),
	                            self);
  gtk_application_set_accels_for_action (GTK_APPLICATION (self),
	                                  "app.quit",
	                                  (const char *[]) { "<primary>q", NULL });
  gtk_application_set_accels_for_action (GTK_APPLICATION (self),
	                                  "app.preferences",
	                                  (const char *[]) { "<primary>comma", NULL });
}

/* Event handlers declaration */
static void
polyhymnia_application_mpd_initialized (PolyhymniaApplication *self,
                                        GParamSpec            *pspec,
                                        PolyhymniaMpdClient   *user_data)
{
  gboolean mpd_initialized = polyhymnia_mpd_client_is_initialized (user_data);
  GSimpleAction *reconnect_action;
  GSimpleAction *rescan_action;
  GSimpleAction *scan_action;
  GSimpleAction *statistics_action;
  GSimpleAction *track_details_action;

  g_assert (POLYHYMNIA_IS_APPLICATION (self));

  reconnect_action = G_SIMPLE_ACTION (g_action_map_lookup_action (G_ACTION_MAP (self),
                                                                  "reconnect"));
  g_simple_action_set_enabled (reconnect_action, !mpd_initialized);

  rescan_action = G_SIMPLE_ACTION (g_action_map_lookup_action (G_ACTION_MAP (self),
                                                               "rescan"));
  g_simple_action_set_enabled (rescan_action, mpd_initialized);

  scan_action = G_SIMPLE_ACTION (g_action_map_lookup_action (G_ACTION_MAP (self),
                                                             "scan"));
  g_simple_action_set_enabled (scan_action, mpd_initialized);

  statistics_action = G_SIMPLE_ACTION (g_action_map_lookup_action (G_ACTION_MAP (self),
                                                                   "statistics"));
  g_simple_action_set_enabled (statistics_action, mpd_initialized);

  track_details_action = G_SIMPLE_ACTION (g_action_map_lookup_action (G_ACTION_MAP (self),
                                                                      "track-details"));
  g_simple_action_set_enabled (track_details_action, mpd_initialized);
}
