
#include "config.h"

#include "polyhymnia-queue-pane.h"

#include <libadwaita-1/adwaita.h>

#include "polyhymnia-mpd-client-queue.h"
#include "polyhymnia-track.h"

#define _(x) g_dgettext (GETTEXT_PACKAGE, x)

/* Type metadata */
struct _PolyhymniaQueuePane
{
  GtkWidget  parent_instance;

  /* Template widgets */
  AdwToolbarView      *root_toolbar_view;
  GtkListView         *queue_list_view;
  GtkScrolledWindow   *queue_page_content;
  AdwStatusPage       *queue_status_page;

  /* Template objects */
  GListStore          *queue_model;
  GtkNoSelection      *queue_selection_model;

  PolyhymniaMpdClient *mpd_client;
};

G_DEFINE_FINAL_TYPE (PolyhymniaQueuePane, polyhymnia_queue_pane, GTK_TYPE_WIDGET)

/* Event handlers*/
static void
polyhymnia_queue_pane_clear_button_clicked (PolyhymniaQueuePane *self,
                                            GtkButton           *user_data);

static void
polyhymnia_queue_pane_mpd_client_initialized (PolyhymniaQueuePane *self,
                                              GParamSpec          *pspec,
                                              PolyhymniaMpdClient *user_data);

static void
polyhymnia_queue_pane_mpd_queue_modified (PolyhymniaQueuePane *self,
                                          PolyhymniaMpdClient *user_data);

static void
polyhymnia_queue_pane_queue_to_playlist_button_clicked (PolyhymniaQueuePane *self,
                                                        GtkButton           *user_data);

/* Private methods*/
static void
polyhymnia_queue_pane_fill (PolyhymniaQueuePane *self);

/* Class stuff */
static void
polyhymnia_queue_pane_dispose(GObject *gobject)
{
  PolyhymniaQueuePane *self = POLYHYMNIA_QUEUE_PANE (gobject);

  gtk_widget_unparent (GTK_WIDGET (self->root_toolbar_view));
  gtk_widget_dispose_template (GTK_WIDGET (self), POLYHYMNIA_TYPE_QUEUE_PANE);

  G_OBJECT_CLASS (polyhymnia_queue_pane_parent_class)->dispose (gobject);
}

static void
polyhymnia_queue_pane_class_init (PolyhymniaQueuePaneClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->dispose = polyhymnia_queue_pane_dispose;

  gtk_widget_class_set_layout_manager_type (widget_class, GTK_TYPE_BIN_LAYOUT);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/com/github/pamugk/polyhymnia/ui/polyhymnia-queue-pane.ui");

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaQueuePane, root_toolbar_view);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaQueuePane, queue_list_view);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaQueuePane, queue_page_content);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaQueuePane, queue_status_page);

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaQueuePane, queue_selection_model);

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaQueuePane, mpd_client);

  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_queue_pane_clear_button_clicked);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_queue_pane_queue_to_playlist_button_clicked);

  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_queue_pane_mpd_client_initialized);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_queue_pane_mpd_queue_modified);
}

static void
polyhymnia_queue_pane_init (PolyhymniaQueuePane *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  self->queue_model = g_list_store_new (POLYHYMNIA_TYPE_TRACK);
  gtk_no_selection_set_model (self->queue_selection_model,
                              G_LIST_MODEL (self->queue_model));

  polyhymnia_queue_pane_mpd_client_initialized (self, NULL, self->mpd_client);
}

/* Event handler implementation */
static void
polyhymnia_queue_pane_clear_button_clicked (PolyhymniaQueuePane *self,
                                            GtkButton           *user_data)
{
  GError           *error = NULL;

  g_assert (POLYHYMNIA_IS_QUEUE_PANE (self));

  polyhymnia_mpd_client_clear_queue (self->mpd_client, &error);

  if (error != NULL)
  {
    g_warning("Failed to clear queue: %s\n", error->message);
    g_error_free (error);
    error = NULL;
  }
}

static void
polyhymnia_queue_pane_mpd_client_initialized (PolyhymniaQueuePane *self,
                                              GParamSpec          *pspec,
                                              PolyhymniaMpdClient *user_data)
{
  g_assert (POLYHYMNIA_IS_QUEUE_PANE (self));

  if (polyhymnia_mpd_client_is_initialized (user_data))
  {
    polyhymnia_queue_pane_fill (self);
  }
  else
  {
    g_list_store_remove_all (self->queue_model);
  }
}

static void
polyhymnia_queue_pane_mpd_queue_modified (PolyhymniaQueuePane *self,
                                          PolyhymniaMpdClient *user_data)
{
  g_assert (POLYHYMNIA_IS_QUEUE_PANE (self));

  g_list_store_remove_all (self->queue_model);
  polyhymnia_queue_pane_fill (self);
}

static void
polyhymnia_queue_pane_queue_to_playlist_button_clicked (PolyhymniaQueuePane *self,
                                                        GtkButton           *user_data)
{

}

/* Private methods implementation */
static void
polyhymnia_queue_pane_fill (PolyhymniaQueuePane *self)
{
  GError *error = NULL;
  GPtrArray *queue = polyhymnia_mpd_client_get_queue (self->mpd_client,
                                                      &error);
  if (error != NULL)
  {
    g_object_set (G_OBJECT (self->queue_status_page),
                  "description", _("Failed to fetch queue"),
                  NULL);
    adw_toolbar_view_set_reveal_bottom_bars (self->root_toolbar_view, FALSE);
    adw_toolbar_view_set_reveal_top_bars (self->root_toolbar_view, FALSE);
    gtk_scrolled_window_set_child (self->queue_page_content,
                                   GTK_WIDGET (self->queue_status_page));
    g_warning("Queue fetch failed: %s\n", error->message);
    g_error_free (error);
    error = NULL;
  }
  else if (queue->len == 0)
  {
    g_ptr_array_free (queue, FALSE);
    g_object_set (G_OBJECT (self->queue_status_page),
                  "description", _("Queue is empty"),
                  NULL);
    adw_toolbar_view_set_reveal_bottom_bars (self->root_toolbar_view, FALSE);
    adw_toolbar_view_set_reveal_top_bars (self->root_toolbar_view, FALSE);
    gtk_scrolled_window_set_child (self->queue_page_content,
                                   GTK_WIDGET (self->queue_status_page));
  }
  else
  {
    for (int i = 0; i < queue->len; i++)
    {
      PolyhymniaTrack *queue_track = g_ptr_array_index(queue, i);
      g_list_store_append (self->queue_model, queue_track);
    }
    g_ptr_array_free (queue, TRUE);
    gtk_scrolled_window_set_child (self->queue_page_content,
                                   GTK_WIDGET (self->queue_list_view));
    adw_toolbar_view_set_reveal_bottom_bars (self->root_toolbar_view, TRUE);
    adw_toolbar_view_set_reveal_top_bars (self->root_toolbar_view, TRUE);
  }
}
