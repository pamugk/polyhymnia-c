
#include "config.h"

#include "polyhymnia-window.h"

struct _PolyhymniaWindow
{
	AdwApplicationWindow  parent_instance;

	/* Template widgets */
	//GtkHeaderBar        *header_bar;
	//GtkLabel            *label;
};

G_DEFINE_FINAL_TYPE (PolyhymniaWindow, polyhymnia_window, ADW_TYPE_APPLICATION_WINDOW)

static void
polyhymnia_window_class_init (PolyhymniaWindowClass *klass)
{
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

	gtk_widget_class_set_template_from_resource (widget_class, "/com/github/pamugk/polyhymnia/ui/polyhymnia-window.ui");
	//gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, header_bar);
	//gtk_widget_class_bind_template_child (widget_class, PolyhymniaWindow, label);
}

static void
polyhymnia_window_init (PolyhymniaWindow *self)
{
	gtk_widget_init_template (GTK_WIDGET (self));
}
