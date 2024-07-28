
#include "config.h"

#include "polyhymnia-albums-page.h"

#include "polyhymnia-mpd-client-api.h"

#define _(x) g_dgettext (GETTEXT_PACKAGE, x)

/* Type metadata */
typedef enum
{
  SIGNAL_NAVIGATE = 1,
  N_SIGNALS,
} PolyhymniaAlbumsPageSignal;

struct _PolyhymniaAlbumsPage
{
  AdwNavigationPage  parent_instance;

  /* Stored UI state */
  GCancellable         *albums_cancellable;

  /* Template widgets */
  GtkGridView         *albums_content;
  GtkSpinner          *albums_spinner;
  AdwStatusPage       *albums_status_page;

  /* Template objects */
  PolyhymniaMpdClient *mpd_client;

  GListStore          *albums_model;
  GtkNoSelection      *albums_selection_model;
};

G_DEFINE_FINAL_TYPE (PolyhymniaAlbumsPage, polyhymnia_albums_page, ADW_TYPE_NAVIGATION_PAGE)

static guint obj_signals[N_SIGNALS] = { 0, };

/* Event handler declarations */
static void
polyhymnia_albums_page_album_clicked (PolyhymniaAlbumsPage *self,
                                      guint                 position,
                                      GtkGridView          *user_data);

static void
polyhymnia_albums_page_mpd_client_initialized (PolyhymniaAlbumsPage *self,
                                               GParamSpec           *pspec,
                                               PolyhymniaMpdClient  *mpd_client);

static void
polyhymnia_albums_page_mpd_database_updated (PolyhymniaAlbumsPage *self,
                                             PolyhymniaMpdClient  *user_data);

static void
polyhymnia_albums_page_search_albums_callback (GObject      *source_object,
                                               GAsyncResult *result,
                                               gpointer      user_data);

/* Class stuff */
static void
polyhymnia_albums_page_dispose(GObject *gobject)
{
  PolyhymniaAlbumsPage *self = POLYHYMNIA_ALBUMS_PAGE (gobject);

  adw_navigation_page_set_child (ADW_NAVIGATION_PAGE (self), NULL);
  gtk_widget_dispose_template (GTK_WIDGET (self), POLYHYMNIA_TYPE_ALBUMS_PAGE);

  G_OBJECT_CLASS (polyhymnia_albums_page_parent_class)->dispose (gobject);
}

static void
polyhymnia_albums_page_class_init (PolyhymniaAlbumsPageClass *klass)
{
  GObjectClass   *gobject_class = G_OBJECT_CLASS (klass);
  GType          type = G_TYPE_FROM_CLASS (gobject_class);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  GType navigate_param_types[1] = { G_TYPE_STRING };

  gobject_class->dispose = polyhymnia_albums_page_dispose;

  obj_signals[SIGNAL_NAVIGATE] =
     g_signal_newv ("navigate", type,
                    G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                    NULL, NULL, NULL, NULL,
                    G_TYPE_NONE,
                    1, navigate_param_types);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/com/github/pamugk/polyhymnia/ui/polyhymnia-albums-page.ui");

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaAlbumsPage, albums_content);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaAlbumsPage, albums_spinner);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaAlbumsPage, albums_status_page);

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaAlbumsPage, mpd_client);

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaAlbumsPage, albums_selection_model);

  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_albums_page_album_clicked);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_albums_page_mpd_client_initialized);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_albums_page_mpd_database_updated);
}

static void
polyhymnia_albums_page_init (PolyhymniaAlbumsPage *self)
{
  self->albums_model = g_list_store_new (POLYHYMNIA_TYPE_ALBUM);

  gtk_widget_init_template (GTK_WIDGET (self));

  gtk_no_selection_set_model (self->albums_selection_model,
                              G_LIST_MODEL (self->albums_model));
  polyhymnia_albums_page_mpd_client_initialized (self, NULL, self->mpd_client);
}

/* Event handler functions implementation */
static void
polyhymnia_albums_page_album_clicked (PolyhymniaAlbumsPage *self,
                                      guint                 position,
                                      GtkGridView          *user_data)
{
  PolyhymniaAlbum *album;

  g_assert (POLYHYMNIA_IS_ALBUMS_PAGE (self));

  album = g_list_model_get_item (G_LIST_MODEL (self->albums_model), position);
  g_signal_emit (self, obj_signals[SIGNAL_NAVIGATE], 0,
                 polyhymnia_album_get_title (album));
}

static void
polyhymnia_albums_page_mpd_client_initialized (PolyhymniaAlbumsPage *self,
                                               GParamSpec           *pspec,
                                               PolyhymniaMpdClient  *mpd_client)
{
  g_assert (POLYHYMNIA_IS_ALBUMS_PAGE (self));

  if (polyhymnia_mpd_client_is_initialized (mpd_client))
  {
    polyhymnia_albums_page_mpd_database_updated (self, mpd_client);
  }
  else
  {
    g_list_store_remove_all (self->albums_model);
  }
}

static void
polyhymnia_albums_page_mpd_database_updated (PolyhymniaAlbumsPage *self,
                                             PolyhymniaMpdClient  *user_data)
{
  g_assert (POLYHYMNIA_IS_ALBUMS_PAGE (self));

  if (self->albums_cancellable != NULL)
  {
    return;
  }

  self->albums_cancellable = g_cancellable_new ();
  polyhymnia_mpd_client_search_albums_async (user_data,
                                             self->albums_cancellable,
                                             polyhymnia_albums_page_search_albums_callback,
                                             self);

  gtk_spinner_start (self->albums_spinner);

  if (adw_navigation_page_get_child (ADW_NAVIGATION_PAGE (self)) != GTK_WIDGET (self->albums_spinner))
  {
    adw_navigation_page_set_child (ADW_NAVIGATION_PAGE (self), GTK_WIDGET (self->albums_spinner));
  }
}

static void
polyhymnia_albums_page_search_albums_callback (GObject      *source_object,
                                               GAsyncResult *result,
                                               gpointer      user_data)
{
  GPtrArray            *albums;
  GError               *error = NULL;
  PolyhymniaMpdClient  *mpd_client = POLYHYMNIA_MPD_CLIENT (source_object);
  GtkWidget            *new_child;
  PolyhymniaAlbumsPage *self = user_data;

  albums = polyhymnia_mpd_client_search_albums_finish (mpd_client, result,
                                                       &error);
  if (error == NULL)
  {
    if (albums->len == 0)
    {
      g_object_set (G_OBJECT (self->albums_status_page),
                    "description", _("If something is missing, try launching library scanning"),
                    "icon-name", "question-round-symbolic",
                    "title", _("No albums found"),
                    NULL);
      new_child = GTK_WIDGET (self->albums_status_page);
      g_list_store_remove_all (self->albums_model);
    }
    else
    {
      g_list_store_splice (self->albums_model,
                           0, g_list_model_get_n_items (G_LIST_MODEL (self->albums_model)),
                           albums->pdata, albums->len);
      new_child = GTK_WIDGET (self->albums_content);
    }
    g_ptr_array_unref (albums);
  }
  else if (!g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
  {
    g_object_set (G_OBJECT (self->albums_status_page),
                  "description", NULL,
                  "icon-name", "error-symbolic",
                  "title", _("Search for albums failed"),
                  NULL);
    new_child = GTK_WIDGET (self->albums_status_page);
    g_warning("Search for albums failed: %s\n", error->message);
    g_list_store_remove_all (self->albums_model);
    g_error_free (error);
  }
  else
  {
    g_error_free (error);
    g_clear_object (&(self->albums_cancellable));
    return;
  }

  if (adw_navigation_page_get_child (ADW_NAVIGATION_PAGE (self)) != new_child)
  {
    adw_navigation_page_set_child (ADW_NAVIGATION_PAGE (self), new_child);
  }

  gtk_spinner_stop (self->albums_spinner);
  g_clear_object (&(self->albums_cancellable));
}

