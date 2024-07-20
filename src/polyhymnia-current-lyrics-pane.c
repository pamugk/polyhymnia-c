
#include "config.h"

#include "polyhymnia-current-lyrics-pane.h"

#include <libadwaita-1/adwaita.h>

#include "polyhymnia-mpd-client-common.h"

#define _(x) g_dgettext (GETTEXT_PACKAGE, x)

/* Type metadata */
struct _PolyhymniaCurrentLyricsPane
{
  GtkWidget  parent_instance;

  /* Template widgets */
  AdwToolbarView      *root_toolbar_view;
  GtkScrolledWindow   *lyrics_page_content;
  AdwStatusPage       *lyrics_status_page;
  GtkTextView         *lyrics_text_view;
  GtkSpinner          *spinner;

  /* Template objects */
  PolyhymniaMpdClient *mpd_client;
};

G_DEFINE_FINAL_TYPE (PolyhymniaCurrentLyricsPane, polyhymnia_current_lyrics_pane, GTK_TYPE_WIDGET)

/* Event handlers*/
static void
polyhymnia_current_lyrics_pane_current_track_changed (PolyhymniaCurrentLyricsPane *self,
                                                      GParamSpec                  *pspec,
                                                      PolyhymniaMpdClient         *user_data);

static void
polyhymnia_current_lyrics_pane_mpd_client_initialized (PolyhymniaCurrentLyricsPane *self,
                                                       GParamSpec                  *pspec,
                                                       PolyhymniaMpdClient         *user_data);

/* Class stuff */
static void
polyhymnia_current_lyrics_pane_dispose(GObject *gobject)
{
  PolyhymniaCurrentLyricsPane *self = POLYHYMNIA_CURRENT_LYRICS_PANE (gobject);

  gtk_widget_unparent (GTK_WIDGET (self->root_toolbar_view));
  gtk_widget_dispose_template (GTK_WIDGET (self), POLYHYMNIA_TYPE_CURRENT_LYRICS_PANE);

  G_OBJECT_CLASS (polyhymnia_current_lyrics_pane_parent_class)->dispose (gobject);
}

static void
polyhymnia_current_lyrics_pane_class_init (PolyhymniaCurrentLyricsPaneClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GType type = G_TYPE_FROM_CLASS (gobject_class);
  GType view_detail_types[] = { G_TYPE_STRING };

  gobject_class->dispose = polyhymnia_current_lyrics_pane_dispose;

  gtk_widget_class_set_layout_manager_type (widget_class, GTK_TYPE_BIN_LAYOUT);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/com/github/pamugk/polyhymnia/ui/polyhymnia-current-lyrics-pane.ui");

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaCurrentLyricsPane, root_toolbar_view);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaCurrentLyricsPane, lyrics_page_content);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaCurrentLyricsPane, lyrics_status_page);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaCurrentLyricsPane, lyrics_text_view);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaCurrentLyricsPane, spinner);

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaCurrentLyricsPane, mpd_client);

  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_current_lyrics_pane_current_track_changed);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_current_lyrics_pane_mpd_client_initialized);
}

static void
polyhymnia_current_lyrics_pane_init (PolyhymniaCurrentLyricsPane *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  polyhymnia_current_lyrics_pane_mpd_client_initialized (self, NULL, self->mpd_client);
}

/* Event handler implementation */
static void
polyhymnia_current_lyrics_pane_current_track_changed (PolyhymniaCurrentLyricsPane *self,
                                                      GParamSpec                  *pspec,
                                                      PolyhymniaMpdClient         *user_data)
{
  g_assert (POLYHYMNIA_IS_CURRENT_LYRICS_PANE (self));

  adw_toolbar_view_set_reveal_bottom_bars (self->root_toolbar_view, FALSE);
  adw_toolbar_view_set_reveal_top_bars (self->root_toolbar_view, FALSE);
  gtk_scrolled_window_set_child (self->lyrics_page_content,
                                 GTK_WIDGET (self->spinner));
  gtk_spinner_start (self->spinner);
}

static void
polyhymnia_current_lyrics_pane_mpd_client_initialized (PolyhymniaCurrentLyricsPane *self,
                                                       GParamSpec                  *pspec,
                                                       PolyhymniaMpdClient         *user_data)
{
  g_assert (POLYHYMNIA_IS_CURRENT_LYRICS_PANE (self));

  if (polyhymnia_mpd_client_is_initialized (user_data))
  {
    polyhymnia_current_lyrics_pane_current_track_changed (self, NULL, self->mpd_client);
  }
  else
  {
  }
}
