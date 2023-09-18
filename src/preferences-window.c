
#include "config.h"

#include "preferences-window.h"

struct _PolyhymniaPreferencesWindow
{
	AdwPreferencesWindow  parent_instance;

	/* Template widgets */
	//GtkHeaderBar        *header_bar;
	//GtkLabel            *label;
};

G_DEFINE_FINAL_TYPE (PolyhymniaPreferencesWindow, polyhymnia_preferences_window, ADW_TYPE_PREFERENCES_WINDOW)

static void
polyhymnia_preferences_window_class_init (PolyhymniaPreferencesWindowClass *klass)
{
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

	gtk_widget_class_set_template_from_resource (widget_class, "/com/github/pamugk/polyhymnia/ui/preferences-window.ui");
}

static void
polyhymnia_preferences_window_init (PolyhymniaPreferencesWindow *self)
{
	gtk_widget_init_template (GTK_WIDGET (self));
}
