
#include "config.h"

#include "polyhymnia-application.h"
#include "polyhymnia-mpd-client-core.h"
#include "polyhymnia-player.h"
#include "polyhymnia-preferences-window.h"
#include "polyhymnia-window.h"

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

/* Class stuff - startup & shutdown callbacks, etc */
static void
polyhymnia_application_activate (GApplication *app)
{
  GtkWindow *window;

  g_assert (POLYHYMNIA_IS_APPLICATION (app));

  window = gtk_application_get_active_window (GTK_APPLICATION (app));

  if (window == NULL)
  {
    window = g_object_new (POLYHYMNIA_TYPE_WINDOW,
		            "application", app,
		            NULL);
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
	                  "developer-name", "pamugk",
	                  "version", "0.1.0",
	                  "developers", developers,
	                  "copyright", "© 2023 pamugk",
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
		                      NULL);
  gtk_window_set_modal (preferences_window, TRUE);
  gtk_window_set_transient_for(preferences_window, window);

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

static const GActionEntry app_actions[] = {
  { "about", polyhymnia_application_about_action },
  { "preferences", polyhymnia_application_preferences_action },
  { "quit", polyhymnia_application_quit_action },
  { "reconnect", polyhymnia_application_reconnect_action },
  { "rescan", polyhymnia_application_rescan_action },
  { "scan", polyhymnia_application_scan_action },
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

