
#include "config.h"

#include "polyhymnia-statistics-dialog.h"

#include "polyhymnia-format-utils.h"
#include "polyhymnia-mpd-client-statistics.h"

#define _(x) g_dgettext (GETTEXT_PACKAGE, x)

/* Type metadata */
struct _PolyhymniaStatisticsDialog
{
  AdwDialog parent_instance;

  /* Stored UI state */
  PolyhymniaStatistics *statistics;
  GCancellable         *statistics_cancellable;

  /* Template widgets */
  GtkBox               *content;
  AdwStatusPage        *error_status_page;
  GtkScrolledWindow    *root_container;
  GtkSpinner           *spinner;

  GtkLabel             *artists_count_label;
  GtkLabel             *albums_count_label;
  GtkLabel             *tracks_count_label;
  GtkLabel             *mpd_uptime_label;
  GtkLabel             *total_playtime_label;
  GtkLabel             *total_played_label;
  GtkLabel             *last_update_label;

  /* Template objects */
  PolyhymniaMpdClient *mpd_client;
};

G_DEFINE_FINAL_TYPE (PolyhymniaStatisticsDialog, polyhymnia_statistics_dialog, ADW_TYPE_DIALOG)

/* Event handlers declaration */
static void
polyhymnia_statistics_dialog_get_statistics_callback (GObject *source_object,
                                                      GAsyncResult *result,
                                                      gpointer user_data);

static void
polyhymnia_statistics_dialog_mpd_client_initialized (PolyhymniaStatisticsDialog *self,
                                                     GParamSpec                 *pspec,
                                                     PolyhymniaMpdClient        *user_data);

static void
polyhymnia_statistics_dialog_mpd_database_updated (PolyhymniaStatisticsDialog *self,
                                                   PolyhymniaMpdClient        *user_data);

/* Class stuff */
static void
polyhymnia_statistics_dialog_dispose(GObject *gobject)
{
  PolyhymniaStatisticsDialog *self = POLYHYMNIA_STATISTICS_DIALOG (gobject);

  g_cancellable_cancel (self->statistics_cancellable);
  gtk_widget_dispose_template (GTK_WIDGET (self), POLYHYMNIA_TYPE_STATISTICS_DIALOG);
  g_clear_object (&(self->statistics));

  G_OBJECT_CLASS (polyhymnia_statistics_dialog_parent_class)->dispose (gobject);
}

static void
polyhymnia_statistics_dialog_class_init (PolyhymniaStatisticsDialogClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->dispose = polyhymnia_statistics_dialog_dispose;

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/com/github/pamugk/polyhymnia/ui/polyhymnia-statistics-dialog.ui");

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaStatisticsDialog,
                                        content);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaStatisticsDialog,
                                        error_status_page);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaStatisticsDialog,
                                        root_container);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaStatisticsDialog,
                                        spinner);

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaStatisticsDialog,
                                        artists_count_label);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaStatisticsDialog,
                                        albums_count_label);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaStatisticsDialog,
                                        tracks_count_label);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaStatisticsDialog,
                                        mpd_uptime_label);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaStatisticsDialog,
                                        total_playtime_label);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaStatisticsDialog,
                                        total_played_label);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaStatisticsDialog,
                                        last_update_label);

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaStatisticsDialog, mpd_client);

  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_statistics_dialog_mpd_client_initialized);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_statistics_dialog_mpd_database_updated);
}

static void
polyhymnia_statistics_dialog_init (PolyhymniaStatisticsDialog *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  polyhymnia_statistics_dialog_mpd_client_initialized (self, NULL, self->mpd_client);
}

/* Event handlers implementation */
static void
polyhymnia_statistics_dialog_get_statistics_callback (GObject *source_object,
                                                      GAsyncResult *result,
                                                      gpointer user_data)
{
  GError *error = NULL;
  PolyhymniaMpdClient *mpd_client = POLYHYMNIA_MPD_CLIENT (source_object);
  PolyhymniaStatistics *new_statistics;
  PolyhymniaStatisticsDialog *self = user_data;

  new_statistics = polyhymnia_mpd_client_get_statistics_finish (mpd_client, result, &error);

  if (error == NULL)
  {
    guint artists_count = polyhymnia_statistics_get_artists_count (new_statistics);
    gchar *artists_count_str = g_strdup_printf ("%d", artists_count);
    guint albums_count = polyhymnia_statistics_get_albums_count (new_statistics);
    gchar *albums_count_str = g_strdup_printf ("%d", albums_count);
    guint tracks_count = polyhymnia_statistics_get_tracks_count (new_statistics);
    gchar *tracks_count_str = g_strdup_printf ("%d", tracks_count);
    gulong mpd_uptime = polyhymnia_statistics_get_mpd_uptime (new_statistics);
    gchar *mpd_uptime_str = timespan_to_readable (mpd_uptime);
    gulong total_playtime = polyhymnia_statistics_get_db_play_time (new_statistics);
    gchar *total_playtime_str = timespan_to_readable (total_playtime);
    gulong total_played = polyhymnia_statistics_get_mpd_play_time (new_statistics);
    gchar *total_played_str = timespan_to_readable (total_played);
    gulong last_update = polyhymnia_statistics_get_db_last_update (new_statistics);

    gtk_label_set_label (self->artists_count_label, artists_count_str);
    g_free (artists_count_str);
    gtk_label_set_label (self->albums_count_label, albums_count_str);
    g_free (albums_count_str);
    gtk_label_set_label (self->tracks_count_label, tracks_count_str);
    g_free (tracks_count_str);
    gtk_label_set_label (self->mpd_uptime_label, mpd_uptime_str);
    g_free (mpd_uptime_str);
    gtk_label_set_label (self->total_playtime_label, total_playtime_str);
    g_free (total_playtime_str);
    gtk_label_set_label (self->total_played_label, total_played_str);
    g_free (total_played_str);

    if (last_update == 0)
    {
      gtk_label_set_label (self->last_update_label, _("Library not scanned yet"));
    }
    else
    {
      GDateTime *last_update_datetime = g_date_time_new_from_unix_local (last_update);
      gchar *last_update_str = g_date_time_format (last_update_datetime, "\%c");
      gtk_label_set_label (self->last_update_label, last_update_str);
      g_free (last_update_str);
    }

    gtk_scrolled_window_set_child (self->root_container,
                                   GTK_WIDGET (self->content));
    g_clear_object (&(self->statistics));
    self->statistics = g_object_ref (new_statistics);
  }
  else if (!g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
  {
    g_clear_object (&(self->statistics));
    gtk_scrolled_window_set_child (self->root_container,
                                   GTK_WIDGET (self->error_status_page));
    g_warning("Failed to fetch statistics: %s\n", error->message);
    g_error_free (error);
    error = NULL;
  }
  else
  {
    g_error_free (error);
    g_clear_object (&(self->statistics_cancellable));
    return;
  }

  gtk_spinner_stop (self->spinner);
  g_clear_object (&(self->statistics_cancellable));
}

static void
polyhymnia_statistics_dialog_mpd_client_initialized (PolyhymniaStatisticsDialog *self,
                                                     GParamSpec                 *pspec,
                                                     PolyhymniaMpdClient        *user_data)
{
  g_assert (POLYHYMNIA_IS_STATISTICS_DIALOG (self));

  if (polyhymnia_mpd_client_is_initialized (user_data))
  {
    polyhymnia_statistics_dialog_mpd_database_updated (self, user_data);
  }
  else
  {
    gtk_window_close (GTK_WINDOW (self));
  }
}

static void
polyhymnia_statistics_dialog_mpd_database_updated (PolyhymniaStatisticsDialog *self,
                                                   PolyhymniaMpdClient        *user_data)
{
  g_assert (POLYHYMNIA_IS_STATISTICS_DIALOG (self));

  if (self->statistics_cancellable != NULL)
  {
    g_debug ("Statistics fetching already in progress");
    return;
  }

  self->statistics_cancellable = g_cancellable_new ();
  polyhymnia_mpd_client_get_statistics_async (self->mpd_client,
                                              self->statistics_cancellable,
                                              polyhymnia_statistics_dialog_get_statistics_callback,
                                              self);
  gtk_spinner_start (self->spinner);
  gtk_scrolled_window_set_child (self->root_container, GTK_WIDGET (self->spinner));
}
