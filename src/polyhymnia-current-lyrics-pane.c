
#include "config.h"

#include "polyhymnia-current-lyrics-pane.h"

#include <libadwaita-1/adwaita.h>
#include <webkit/webkit.h>

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
  AdwToolbarView *root_toolbar_view;
  AdwBin         *lyrics_page_content;
  AdwStatusPage  *lyrics_status_page;
  WebKitWebView  *lyrics_web_view;
  GtkSpinner     *spinner;

  /* Template objects */
  PolyhymniaLyricsProvider *lyrics_provider;
  PolyhymniaMpdClient      *mpd_client;
  PolyhymniaPlayer         *player;
  GtkUriLauncher           *uri_launcher;
  GCancellable             *uri_launcher_cancellable;
};

G_DEFINE_FINAL_TYPE (PolyhymniaCurrentLyricsPane, polyhymnia_current_lyrics_pane, GTK_TYPE_WIDGET)

/* Event handlers*/
static void
polyhymnia_current_lyrics_pane_current_track_changed (PolyhymniaCurrentLyricsPane *self,
                                                      GParamSpec                  *pspec,
                                                      PolyhymniaPlayer            *user_data);

static gboolean
polyhymnia_current_lyrics_pane_lyrics_web_view_decide_policy (PolyhymniaCurrentLyricsPane *self,
                                                              WebKitPolicyDecision        *decision,
                                                              WebKitPolicyDecisionType     decision_type,
                                                              WebKitWebView               *user_data);

static void
polyhymnia_current_lyrics_pane_lyrics_web_view_load_changed (PolyhymniaCurrentLyricsPane *self,
                                                             WebKitLoadEvent              load_event,
                                                             WebKitWebView               *user_data);

static void
polyhymnia_current_lyrics_pane_search_lyrics_callback (GObject      *source,
                                                       GAsyncResult *result,
                                                       void         *user_data);

static void
polyhymnia_current_lyrics_pane_mpd_client_initialized (PolyhymniaCurrentLyricsPane *self,
                                                       GParamSpec                  *pspec,
                                                       PolyhymniaMpdClient         *user_data);

static void
polyhymnia_current_lyrics_pane_show_uri_callback (GObject      *source_object,
                                                  GAsyncResult *result,
                                                  gpointer      user_data);

/* Class stuff */
static void
polyhymnia_current_lyrics_pane_dispose(GObject *gobject)
{
  PolyhymniaCurrentLyricsPane *self = POLYHYMNIA_CURRENT_LYRICS_PANE (gobject);

  g_cancellable_cancel (self->lyrics_cancellable);
  g_cancellable_cancel (self->uri_launcher_cancellable);
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
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaCurrentLyricsPane, lyrics_status_page);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaCurrentLyricsPane, lyrics_web_view);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaCurrentLyricsPane, spinner);

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaCurrentLyricsPane, lyrics_provider);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaCurrentLyricsPane, mpd_client);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaCurrentLyricsPane, player);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaCurrentLyricsPane, uri_launcher);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaCurrentLyricsPane, uri_launcher_cancellable);

  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_current_lyrics_pane_current_track_changed);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_current_lyrics_pane_lyrics_web_view_decide_policy);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_current_lyrics_pane_lyrics_web_view_load_changed);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_current_lyrics_pane_mpd_client_initialized);
}

static void
polyhymnia_current_lyrics_pane_init (PolyhymniaCurrentLyricsPane *self)
{
  g_type_ensure (WEBKIT_TYPE_WEB_VIEW);

  g_type_ensure (POLYHYMNIA_TYPE_LYRICS_PROVIDER);
  g_type_ensure (POLYHYMNIA_TYPE_MPD_CLIENT);

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

  adw_toolbar_view_set_reveal_top_bars (self->root_toolbar_view, FALSE);
  webkit_web_view_stop_loading (self->lyrics_web_view);

  current_track = polyhymnia_player_get_current_track (user_data);
  if (current_track == NULL)
  {
      g_object_set (G_OBJECT (self->lyrics_status_page),
                    "description", _("No current song"),
                    NULL);
      adw_bin_set_child (self->lyrics_page_content,
                         GTK_WIDGET (self->lyrics_status_page));
  }
  else
  {
    PolyhymniaSearchLyricsRequest *search_lyrics_request;

    search_lyrics_request = g_malloc (sizeof (PolyhymniaSearchLyricsRequest));
    search_lyrics_request->artist = g_strdup (polyhymnia_track_get_artist (current_track));
    search_lyrics_request->title = g_strdup (polyhymnia_track_get_title (current_track));

    adw_bin_set_child (self->lyrics_page_content, GTK_WIDGET (self->spinner));
    gtk_spinner_start (self->spinner);
    self->lyrics_cancellable = g_cancellable_new ();
    polyhymnia_lyrics_provider_search_lyrics_async (self->lyrics_provider,
                                                    search_lyrics_request,
                                                    self->lyrics_cancellable,
                                                    polyhymnia_current_lyrics_pane_search_lyrics_callback,
                                                    self);
  }
}

static gboolean
polyhymnia_current_lyrics_pane_lyrics_web_view_decide_policy (PolyhymniaCurrentLyricsPane *self,
                                                              WebKitPolicyDecision        *decision,
                                                              WebKitPolicyDecisionType     decision_type,
                                                              WebKitWebView               *user_data)
{
  g_assert (POLYHYMNIA_IS_CURRENT_LYRICS_PANE (self));

  switch (decision_type)
  {
    case WEBKIT_POLICY_DECISION_TYPE_NAVIGATION_ACTION:
    case WEBKIT_POLICY_DECISION_TYPE_NEW_WINDOW_ACTION:
    {
      WebKitNavigationAction         *navigation_action;
      WebKitNavigationPolicyDecision *navigation_decision;
      const char                     *uri;
      WebKitURIRequest               *uri_request;

      navigation_decision = WEBKIT_NAVIGATION_POLICY_DECISION (decision);
      navigation_action = webkit_navigation_policy_decision_get_navigation_action (navigation_decision);
      uri_request = webkit_navigation_action_get_request (navigation_action);
      uri = webkit_uri_request_get_uri (uri_request);

      // about:blank used for embedded content, it should be opened in WebView
      if (g_strcmp0 ("about:blank", uri) == 0)
      {
        webkit_policy_decision_use (decision);
      }
      else
      {
        webkit_policy_decision_ignore (decision);
        gtk_uri_launcher_set_uri (self->uri_launcher,
                                  webkit_uri_request_get_uri (uri_request));
        gtk_uri_launcher_launch (self->uri_launcher,
                                 GTK_WINDOW (gtk_widget_get_root (GTK_WIDGET (self))),
                                 self->uri_launcher_cancellable,
                                 polyhymnia_current_lyrics_pane_show_uri_callback,
                                 self);
      }

      break;
    }
    default:
    {
      return FALSE;
    }
  }
  return TRUE;
}

static void
polyhymnia_current_lyrics_pane_lyrics_web_view_load_changed (PolyhymniaCurrentLyricsPane *self,
                                                             WebKitLoadEvent              load_event,
                                                             WebKitWebView               *user_data)
{
  g_assert (POLYHYMNIA_IS_CURRENT_LYRICS_PANE (self));

  if (load_event == WEBKIT_LOAD_FINISHED)
  {
    adw_bin_set_child (self->lyrics_page_content,
                       GTK_WIDGET (self->lyrics_web_view));
    adw_toolbar_view_set_reveal_top_bars (self->root_toolbar_view, TRUE);
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

  lyrics = polyhymnia_lyrics_provider_search_lyrics_finish (POLYHYMNIA_LYRICS_PROVIDER (source),
                                                            result,
                                                            &error);
  if (error != NULL)
  {
      g_object_set (G_OBJECT (self->lyrics_status_page),
                    "description", _("Failed to find song lyrics"),
                    NULL);
      adw_bin_set_child (self->lyrics_page_content,
                         GTK_WIDGET (self->lyrics_status_page));
  }
  else if (lyrics == NULL)
  {
      g_object_set (G_OBJECT (self->lyrics_status_page),
                    "description", _("No song lyrics found"),
                    NULL);
      adw_bin_set_child (self->lyrics_page_content,
                         GTK_WIDGET (self->lyrics_status_page));
  }
  else
  {
    GString *lyrics_string = g_string_new_take (lyrics);
    // This is needed to bypass problems with script download
    g_string_replace (lyrics_string, "src='//", "src='https://", 0);
    // This is needed to follow dark theme (if enabled)
    g_string_prepend (lyrics_string, "<style>:root{color-scheme:light dark;} .rg_embed.music{background-color:transparent;}{background-color: transparent;}</style>");
    webkit_web_view_load_html (self->lyrics_web_view, lyrics_string->str, NULL);

    g_string_free (lyrics_string, TRUE);
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
    webkit_web_view_stop_loading (self->lyrics_web_view);
    g_cancellable_cancel (self->lyrics_cancellable);
  }
}

static void
polyhymnia_current_lyrics_pane_show_uri_callback (GObject      *source_object,
                                                  GAsyncResult *result,
                                                  gpointer      user_data)
{
  GError *error = NULL;

  g_assert (POLYHYMNIA_IS_CURRENT_LYRICS_PANE (user_data));

  gtk_uri_launcher_launch_finish (GTK_URI_LAUNCHER (source_object), result,
                                  &error);
  if (error != NULL)
  {
    g_warning ("Failed to open requested URI. Error: %s", error->message);
    g_error_free (error);
  }
}
