
#include "config.h"

#include "polyhymnia-application.h"
#include "preferences-window.h"
#include "polyhymnia-window.h"

struct _PolyhymniaApplication
{
	AdwApplication parent_instance;
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

static void
polyhymnia_application_activate (GApplication *app)
{
	GtkWindow *window;

	g_assert (POLYHYMNIA_IS_APPLICATION (app));

	window = gtk_application_get_active_window (GTK_APPLICATION (app));

	if (window == NULL)
		window = g_object_new (POLYHYMNIA_TYPE_WINDOW,
		                       "application", app,
		                       NULL);

	gtk_window_present (window);
}

static void
polyhymnia_application_class_init (PolyhymniaApplicationClass *klass)
{
	GApplicationClass *app_class = G_APPLICATION_CLASS (klass);

	app_class->activate = polyhymnia_application_activate;
}

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
	                       "application-name", "polyhymnia",
	                       "application-icon", "com.github.pamugk.polyhymnia",
	                       "developer-name", "pamugk",
	                       "version", "0.1.0",
	                       "developers", developers,
	                       "copyright", "© 2023 pamugk",
	                       NULL);
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
polyhymnia_application_quit_action (GSimpleAction *action,
                                    GVariant      *parameter,
                                    gpointer       user_data)
{
	PolyhymniaApplication *self = user_data;

	g_assert (POLYHYMNIA_IS_APPLICATION (self));

	g_application_quit (G_APPLICATION (self));
}

static const GActionEntry app_actions[] = {
	{ "about", polyhymnia_application_about_action },
	{ "quit", polyhymnia_application_quit_action },
	{ "preferences", polyhymnia_application_preferences_action },
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
