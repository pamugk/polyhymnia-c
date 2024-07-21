
#include "config.h"

#include "polyhymnia-current-lyrics-pane.h"

#include <libadwaita-1/adwaita.h>

#include "polyhymnia-lyrics-provider.h"
#include "polyhymnia-mpd-client-common.h"
#include "polyhymnia-player.h"

#define _(x) g_dgettext (GETTEXT_PACKAGE, x)

/* Type metadata */
struct _PolyhymniaCurrentLyricsPane
{
  GtkWidget  parent_instance;

  /* Stored UI state */
  GCancellable        *lyrics_cancellable;

  /* Template widgets */
  AdwToolbarView      *root_toolbar_view;
  GtkScrolledWindow   *lyrics_page_content;
  GtkLabel            *lyrics_label;
  AdwStatusPage       *lyrics_status_page;
  GtkTextView         *lyrics_text_view;
  GtkSpinner          *spinner;

  /* Template objects */
  PolyhymniaLyricsProvider *lyrics_provider;
  PolyhymniaMpdClient      *mpd_client;
  PolyhymniaPlayer         *player;
};

G_DEFINE_FINAL_TYPE (PolyhymniaCurrentLyricsPane, polyhymnia_current_lyrics_pane, GTK_TYPE_WIDGET)

/* Event handlers*/
static void
polyhymnia_current_lyrics_pane_current_track_changed (PolyhymniaCurrentLyricsPane *self,
                                                      GParamSpec                  *pspec,
                                                      PolyhymniaPlayer            *user_data);

static void
polyhymnia_current_lyrics_pane_search_lyrics_callback (GObject      *source,
                                                       GAsyncResult *result,
                                                       void         *user_data);

static void
polyhymnia_current_lyrics_pane_mpd_client_initialized (PolyhymniaCurrentLyricsPane *self,
                                                       GParamSpec                  *pspec,
                                                       PolyhymniaMpdClient         *user_data);

/* Class stuff */
static void
polyhymnia_current_lyrics_pane_dispose(GObject *gobject)
{
  PolyhymniaCurrentLyricsPane *self = POLYHYMNIA_CURRENT_LYRICS_PANE (gobject);

  g_cancellable_cancel (self->lyrics_cancellable);
  gtk_widget_unparent (GTK_WIDGET (self->root_toolbar_view));
  gtk_widget_dispose_template (GTK_WIDGET (self), POLYHYMNIA_TYPE_CURRENT_LYRICS_PANE);

  G_OBJECT_CLASS (polyhymnia_current_lyrics_pane_parent_class)->dispose (gobject);
}

static void
polyhymnia_current_lyrics_pane_class_init (PolyhymniaCurrentLyricsPaneClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->dispose = polyhymnia_current_lyrics_pane_dispose;

  gtk_widget_class_set_layout_manager_type (widget_class, GTK_TYPE_BIN_LAYOUT);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/com/github/pamugk/polyhymnia/ui/polyhymnia-current-lyrics-pane.ui");

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaCurrentLyricsPane, root_toolbar_view);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaCurrentLyricsPane, lyrics_page_content);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaCurrentLyricsPane, lyrics_label);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaCurrentLyricsPane, lyrics_status_page);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaCurrentLyricsPane, lyrics_text_view);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaCurrentLyricsPane, spinner);

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaCurrentLyricsPane, lyrics_provider);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaCurrentLyricsPane, mpd_client);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaCurrentLyricsPane, player);

  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_current_lyrics_pane_current_track_changed);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_current_lyrics_pane_mpd_client_initialized);
}

static void
polyhymnia_current_lyrics_pane_init (PolyhymniaCurrentLyricsPane *self)
{
  g_type_ensure (POLYHYMNIA_TYPE_LYRICS_PROVIDER);

  gtk_widget_init_template (GTK_WIDGET (self));

  polyhymnia_current_lyrics_pane_mpd_client_initialized (self, NULL, self->mpd_client);
}

/* Event handler implementation */
static void
polyhymnia_current_lyrics_pane_current_track_changed (PolyhymniaCurrentLyricsPane *self,
                                                      GParamSpec                  *pspec,
                                                      PolyhymniaPlayer            *user_data)
{
  PolyhymniaTrack *current_track;

  g_assert (POLYHYMNIA_IS_CURRENT_LYRICS_PANE (self));

  adw_toolbar_view_set_reveal_bottom_bars (self->root_toolbar_view, FALSE);
  adw_toolbar_view_set_reveal_top_bars (self->root_toolbar_view, FALSE);

  current_track = polyhymnia_player_get_current_track (user_data);
  if (current_track == NULL)
  {
      g_object_set (G_OBJECT (self->lyrics_status_page),
                    "description", _("No current song"),
                    NULL);
      gtk_scrolled_window_set_child (self->lyrics_page_content,
                                     GTK_WIDGET (self->lyrics_status_page));
  }
  else
  {
    gtk_scrolled_window_set_child (self->lyrics_page_content,
                                   GTK_WIDGET (self->spinner));
    gtk_spinner_start (self->spinner);
    self->lyrics_cancellable = g_cancellable_new ();
    polyhymnia_lyrics_provider_search_track_lyrics_async (self->lyrics_provider,
                                                          current_track,
                                                          self->lyrics_cancellable,
                                                          polyhymnia_current_lyrics_pane_search_lyrics_callback,
                                                          self);
  }
}

static void
polyhymnia_current_lyrics_pane_search_lyrics_callback (GObject      *source,
                                                       GAsyncResult *result,
                                                       void         *user_data)
{
  GError                      *error = NULL;
  char                        *lyrics;
  PolyhymniaCurrentLyricsPane *self = POLYHYMNIA_CURRENT_LYRICS_PANE (user_data);

  lyrics = polyhymnia_lyrics_provider_search_track_lyrics_finish (POLYHYMNIA_LYRICS_PROVIDER (source),
                                                                  result,
                                                                  &error);
  if (error != NULL)
  {
      g_object_set (G_OBJECT (self->lyrics_status_page),
                    "description", _("Failed to find song lyrics"),
                    NULL);
      gtk_scrolled_window_set_child (self->lyrics_page_content,
                                     GTK_WIDGET (self->lyrics_status_page));
  }
  else if (lyrics == NULL)
  {
      g_object_set (G_OBJECT (self->lyrics_status_page),
                    "description", _("No song lyrics found"),
                    NULL);
      gtk_scrolled_window_set_child (self->lyrics_page_content,
                                     GTK_WIDGET (self->lyrics_status_page));
  }
  else
  {
    char *lyrics_ui_text = g_strdup_printf ("Lyrics can be viewed <a href=\"%s\">here</a>", lyrics);
    gtk_label_set_label (self->lyrics_label, lyrics_ui_text);
    gtk_scrolled_window_set_child (self->lyrics_page_content,
                                   GTK_WIDGET (self->lyrics_label));
    adw_toolbar_view_set_reveal_bottom_bars (self->root_toolbar_view, TRUE);
    adw_toolbar_view_set_reveal_top_bars (self->root_toolbar_view, TRUE);

    g_free (lyrics);
    g_free (lyrics_ui_text);
  }

  gtk_spinner_stop (self->spinner);
  g_clear_object (&(self->lyrics_cancellable));
}

static void
polyhymnia_current_lyrics_pane_mpd_client_initialized (PolyhymniaCurrentLyricsPane *self,
                                                       GParamSpec                  *pspec,
                                                       PolyhymniaMpdClient         *user_data)
{
  g_assert (POLYHYMNIA_IS_CURRENT_LYRICS_PANE (self));

  if (polyhymnia_mpd_client_is_initialized (user_data))
  {
    polyhymnia_current_lyrics_pane_current_track_changed (self, NULL, self->player);
  }
  else
  {
    g_cancellable_cancel (self->lyrics_cancellable);
  }
}
