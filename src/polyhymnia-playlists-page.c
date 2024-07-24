
#include "config.h"

#include "polyhymnia-playlists-page.h"

#include "polyhymnia-mpd-client-playlists.h"

#define _(x) g_dgettext (GETTEXT_PACKAGE, x)

/* Type metadata */
typedef enum
{
  SIGNAL_NAVIGATE = 1,
  N_SIGNALS,
} PolyhymniaPlaylistsPageSignal;

struct _PolyhymniaPlaylistsPage
{
  AdwNavigationPage  parent_instance;

  /* Stored UI state */
  GCancellable        *playlists_cancellable;

  /* Template widgets */
  GtkScrolledWindow   *playlists_content;
  AdwStatusPage       *playlists_status_page;
  GtkSpinner          *spinner;

  /* Template objects */
  PolyhymniaMpdClient *mpd_client;
  GtkStringList       *playlists_model;
};

G_DEFINE_FINAL_TYPE (PolyhymniaPlaylistsPage, polyhymnia_playlists_page, ADW_TYPE_NAVIGATION_PAGE)

static guint obj_signals[N_SIGNALS] = { 0, };

/* Event handler declarations */
static void
polyhymnia_playlists_page_search_playlists_callback (GObject      *source_object,
                                                     GAsyncResult *result,
                                                     gpointer      user_data);

static void
polyhymnia_playlists_page_mpd_client_initialized (PolyhymniaPlaylistsPage *self,
                                                  GParamSpec              *pspec,
                                                  PolyhymniaMpdClient     *mpd_client);

static void
polyhymnia_playlists_page_mpd_playlists_changed (PolyhymniaPlaylistsPage *self,
                                                 PolyhymniaMpdClient     *mpd_client);

static void
polyhymnia_playlists_page_playlist_clicked (PolyhymniaPlaylistsPage *self,
                                            guint                   position,
                                            GtkGridView             *user_data);

/* Class stuff */
static void
polyhymnia_playlists_page_dispose(GObject *gobject)
{
  PolyhymniaPlaylistsPage *self = POLYHYMNIA_PLAYLISTS_PAGE (gobject);

  g_cancellable_cancel (self->playlists_cancellable);
  adw_navigation_page_set_child (ADW_NAVIGATION_PAGE (self), NULL);
  gtk_widget_dispose_template (GTK_WIDGET (self), POLYHYMNIA_TYPE_PLAYLISTS_PAGE);

  G_OBJECT_CLASS (polyhymnia_playlists_page_parent_class)->dispose (gobject);
}

static void
polyhymnia_playlists_page_class_init (PolyhymniaPlaylistsPageClass *klass)
{
  GObjectClass   *gobject_class = G_OBJECT_CLASS (klass);
  GType          type = G_TYPE_FROM_CLASS (gobject_class);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  GType navigate_param_types[1] = { G_TYPE_STRING };

  gobject_class->dispose = polyhymnia_playlists_page_dispose;

  obj_signals[SIGNAL_NAVIGATE] =
     g_signal_newv ("navigate", type,
                    G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                    NULL, NULL, NULL, NULL,
                    G_TYPE_NONE,
                    1, navigate_param_types);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/com/github/pamugk/polyhymnia/ui/polyhymnia-playlists-page.ui");

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaPlaylistsPage, playlists_content);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaPlaylistsPage, playlists_status_page);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaPlaylistsPage, spinner);

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaPlaylistsPage, mpd_client);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaPlaylistsPage, playlists_model);

  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_playlists_page_mpd_client_initialized);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_playlists_page_mpd_playlists_changed);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_playlists_page_playlist_clicked);
}

static void
polyhymnia_playlists_page_init (PolyhymniaPlaylistsPage *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  polyhymnia_playlists_page_mpd_client_initialized (self, NULL, self->mpd_client);
}

/* Event handler functions implementation */
static void
polyhymnia_playlists_page_search_playlists_callback (GObject      *source_object,
                                                     GAsyncResult *result,
                                                     gpointer      user_data)
{
  GError *error = NULL;
  PolyhymniaMpdClient *mpd_client = POLYHYMNIA_MPD_CLIENT (source_object);
  GtkWidget *new_child;
  GPtrArray *playlists;
  PolyhymniaPlaylistsPage *self = user_data;

  playlists = polyhymnia_mpd_client_search_playlists_finish (mpd_client, result,
                                                             &error);
  if (error == NULL)
  {
    if (playlists->len == 0)
    {
      g_object_set (G_OBJECT (self->playlists_status_page),
                    "description", _("If something is missing, try launching library scanning"),
                    "icon-name", "question-round-symbolic",
                    "title", _("No playlists found"),
                    NULL);
      new_child = GTK_WIDGET (self->playlists_status_page);
      gtk_string_list_splice (self->playlists_model,
                              0, g_list_model_get_n_items (G_LIST_MODEL (self->playlists_model)),
                              NULL);
    }
    else
    {
      gtk_string_list_splice (self->playlists_model,
                              0, g_list_model_get_n_items (G_LIST_MODEL (self->playlists_model)),
                              (const gchar *const *) playlists->pdata);
      new_child = GTK_WIDGET (self->playlists_content);
    }
    g_ptr_array_unref (playlists);
  }
  else if (!g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
  {
    g_object_set (G_OBJECT (self->playlists_status_page),
                  "description", NULL,
                  "icon-name", "error-symbolic",
                  "title", _("Search for playlists failed"),
                  NULL);
    new_child = GTK_WIDGET (self->playlists_status_page);
    g_warning("Search for playlists failed: %s\n", error->message);
    g_error_free (error);
    error = NULL;
    gtk_string_list_splice (self->playlists_model,
                            0, g_list_model_get_n_items (G_LIST_MODEL (self->playlists_model)),
                            NULL);
  }
  else
  {
    g_clear_object (&(self->playlists_cancellable));
    return;
  }

  if (adw_navigation_page_get_child (ADW_NAVIGATION_PAGE (self)) != new_child)
  {
    adw_navigation_page_set_child (ADW_NAVIGATION_PAGE (self), new_child);
  }

  gtk_spinner_stop (self->spinner);
  g_clear_object (&(self->playlists_cancellable));
}

static void
polyhymnia_playlists_page_mpd_client_initialized (PolyhymniaPlaylistsPage *self,
                                                  GParamSpec              *pspec,
                                                  PolyhymniaMpdClient     *mpd_client)
{
  g_assert (POLYHYMNIA_IS_PLAYLISTS_PAGE (self));

  if (polyhymnia_mpd_client_is_initialized (mpd_client))
  {
    polyhymnia_playlists_page_mpd_playlists_changed (self, mpd_client);
  }
  else
  {
    gtk_string_list_splice (self->playlists_model,
                            0, g_list_model_get_n_items (G_LIST_MODEL (self->playlists_model)),
                            NULL);
  }
}

static void
polyhymnia_playlists_page_mpd_playlists_changed (PolyhymniaPlaylistsPage *self,
                                                 PolyhymniaMpdClient     *mpd_client)
{
  g_assert (POLYHYMNIA_IS_PLAYLISTS_PAGE (self));

  if (self->playlists_cancellable == NULL)
  {
    self->playlists_cancellable = g_cancellable_new ();
    polyhymnia_mpd_client_search_playlists_async (mpd_client,
                                                  self->playlists_cancellable,
                                                  polyhymnia_playlists_page_search_playlists_callback,
                                                  self);

    gtk_spinner_start (self->spinner);

    if (adw_navigation_page_get_child (ADW_NAVIGATION_PAGE (self)) != GTK_WIDGET (self->spinner))
    {
      adw_navigation_page_set_child (ADW_NAVIGATION_PAGE (self), GTK_WIDGET (self->spinner));
    }
  }
}

static void
polyhymnia_playlists_page_playlist_clicked (PolyhymniaPlaylistsPage *self,
                                            guint                   position,
                                            GtkGridView             *user_data)
{
  const gchar *playlist_title;

  g_assert (POLYHYMNIA_IS_PLAYLISTS_PAGE (self));

  playlist_title = gtk_string_list_get_string (self->playlists_model, position);
  g_signal_emit (self, obj_signals[SIGNAL_NAVIGATE], 0, playlist_title);
}
