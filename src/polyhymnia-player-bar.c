
#include "config.h"

#include <gdk/gdk.h>

#include "polyhymnia-format-utils.h"
#include "polyhymnia-player.h"
#include "polyhymnia-player-bar.h"

#define _(x) g_dgettext (GETTEXT_PACKAGE, x)

/* Type metadata */
typedef enum
{
  SIGNAL_VIEW_TRACK_DETAILS = 1,
  N_SIGNALS,
} PolyhymniaTracksPageSignal;

struct _PolyhymniaPlayerBar
{
  GtkWidget  parent_instance;

  /* Stored UI state */
  GdkTexture          *current_track_album_cover;

  /* Template widgets */
  GtkActionBar        *root_action_bar;
  GtkLabel            *current_track_artist_label;
  GtkImage            *current_track_cover_image;
  GtkLabel            *current_track_title_label;
  GtkToggleButton     *queue_button;
  GtkButton           *play_button;
  GtkAdjustment       *playback_adjustment;
  GtkLabel            *playback_elapsed_label;
  GtkLabel            *playback_total_label;
  GtkScaleButton      *volume_scale_button;

  /* Template objects */
  PolyhymniaPlayer    *player;

#ifdef POLYHYMNIA_FEATURE_LYRICS
  GtkToggleButton     *lyrics_button;
#endif
};

G_DEFINE_FINAL_TYPE (PolyhymniaPlayerBar, polyhymnia_player_bar, GTK_TYPE_WIDGET)

static guint obj_signals[N_SIGNALS] = { 0, };

static const gchar *VOLUME_ICONS[] =
{
  "audio-volume-low-symbolic",
  "audio-volume-high-symbolic",
  "audio-volume-medium-symbolic",
  NULL,
};

/* Utility functions declaration */
static const gchar *
polyhymnia_player_bar_state_to_icon(PolyhymniaPlayerPlaybackStatus state);

/* Event handler declaration  */
static void
polyhymnia_player_bar_current_track(PolyhymniaPlayerBar *self,
                                    GParamSpec          *pspec,
                                    PolyhymniaPlayer    *user_data);

static void
polyhymnia_player_bar_details_button_clicked(PolyhymniaPlayerBar *self,
                                             gpointer             user_data);

static void
polyhymnia_player_bar_elapsed_seconds(PolyhymniaPlayerBar *self,
                                      GParamSpec          *pspec,
                                      PolyhymniaPlayer    *user_data);

static void
polyhymnia_player_bar_next_button_clicked(PolyhymniaPlayerBar *self,
                                          gpointer             user_data);

static void
polyhymnia_player_bar_play_button_clicked(PolyhymniaPlayerBar *self,
                                          gpointer             user_data);

static gboolean
polyhymnia_player_bar_playback_seek(PolyhymniaPlayerBar *self,
                                    GtkScrollType       *scroll,
                                    gdouble              value,
                                    GtkScale            *user_data);

static void
polyhymnia_player_bar_previous_button_clicked(PolyhymniaPlayerBar *self,
                                              gpointer             user_data);

static void
polyhymnia_player_bar_state(PolyhymniaPlayerBar *self,
                            GParamSpec          *pspec,
                            PolyhymniaPlayer    *user_data);

static void
polyhymnia_player_bar_volume_minus_button_clicked(PolyhymniaPlayerBar *self,
                                                  gpointer             user_data);

static void
polyhymnia_player_bar_volume_plus_button_clicked(PolyhymniaPlayerBar *self,
                                                 gpointer             user_data);

#ifdef POLYHYMNIA_FEATURE_LYRICS
static void
polyhymnia_player_bar_lyrics_toggled(PolyhymniaPlayerBar *self,
                                     gpointer             user_data);

static void
polyhymnia_player_bar_queue_toggled(PolyhymniaPlayerBar *self,
                                    gpointer             user_data);
#endif

/* Class stuff */
static void
polyhymnia_player_bar_dispose(GObject *gobject)
{
  PolyhymniaPlayerBar *self = POLYHYMNIA_PLAYER_BAR (gobject);

  g_clear_object (&(self->current_track_album_cover));
  gtk_widget_unparent (GTK_WIDGET (self->root_action_bar));
  gtk_widget_dispose_template (GTK_WIDGET (self), POLYHYMNIA_TYPE_PLAYER_BAR);

  G_OBJECT_CLASS (polyhymnia_player_bar_parent_class)->dispose (gobject);
}

static void
polyhymnia_player_bar_class_init (PolyhymniaPlayerBarClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GType type = G_TYPE_FROM_CLASS (gobject_class);
  GType view_detail_types[] = { G_TYPE_STRING };

  gobject_class->dispose = polyhymnia_player_bar_dispose;

  obj_signals[SIGNAL_VIEW_TRACK_DETAILS] =
     g_signal_newv ("view-track-details", type,
                    G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                    NULL, NULL, NULL, NULL,
                    G_TYPE_NONE,
                    1, view_detail_types);

  gtk_widget_class_set_layout_manager_type (widget_class, GTK_TYPE_BIN_LAYOUT);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/com/github/pamugk/polyhymnia/ui/polyhymnia-player-bar.ui");

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaPlayerBar, root_action_bar);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaPlayerBar, current_track_artist_label);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaPlayerBar, current_track_cover_image);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaPlayerBar, current_track_title_label);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaPlayerBar, queue_button);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaPlayerBar, play_button);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaPlayerBar, playback_adjustment);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaPlayerBar, playback_elapsed_label);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaPlayerBar, playback_total_label);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaPlayerBar, volume_scale_button);

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaPlayerBar, player);

  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_player_bar_details_button_clicked);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_player_bar_next_button_clicked);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_player_bar_play_button_clicked);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_player_bar_playback_seek);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_player_bar_previous_button_clicked);

  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_player_bar_current_track);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_player_bar_elapsed_seconds);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_player_bar_state);

#ifdef POLYHYMNIA_FEATURE_LYRICS
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_player_bar_queue_toggled);
#endif
}

static void
polyhymnia_player_bar_init (PolyhymniaPlayerBar *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  gtk_scale_button_set_icons (self->volume_scale_button, VOLUME_ICONS);
  g_signal_connect_swapped (gtk_scale_button_get_minus_button (self->volume_scale_button),
                            "clicked",
                            G_CALLBACK (polyhymnia_player_bar_volume_minus_button_clicked),
                            self);
  g_signal_connect_swapped (gtk_scale_button_get_plus_button (self->volume_scale_button),
                            "clicked",
                            G_CALLBACK (polyhymnia_player_bar_volume_plus_button_clicked),
                            self);

#ifdef POLYHYMNIA_FEATURE_LYRICS
  self->lyrics_button = GTK_TOGGLE_BUTTON (gtk_toggle_button_new ());
  gtk_widget_set_has_tooltip (GTK_WIDGET (self->lyrics_button), TRUE);
  gtk_button_set_icon_name (GTK_BUTTON (self->lyrics_button), "subtitles-symbolic");
  g_object_bind_property (self->player, "active",
                          self->lyrics_button, "sensitive",
                          G_BINDING_SYNC_CREATE);
  gtk_widget_set_tooltip_text (GTK_WIDGET (self->lyrics_button), _("Lyrics"));
  gtk_widget_set_valign (GTK_WIDGET (self->lyrics_button), GTK_ALIGN_CENTER);
  gtk_action_bar_pack_end (self->root_action_bar, GTK_WIDGET (self->lyrics_button));

  g_signal_connect_swapped (self->lyrics_button, "toggled",
                            (GCallback) polyhymnia_player_bar_lyrics_toggled,
                            self);
  g_signal_connect_swapped (self->queue_button, "toggled",
                            (GCallback) polyhymnia_player_bar_queue_toggled,
                            self);
#endif
  gtk_action_bar_pack_end (self->root_action_bar, GTK_WIDGET (self->queue_button));
  gtk_action_bar_pack_end (self->root_action_bar, GTK_WIDGET (self->volume_scale_button));

  polyhymnia_player_bar_current_track (self, NULL, self->player);
  polyhymnia_player_bar_elapsed_seconds (self, NULL, self->player);
  polyhymnia_player_bar_state (self, NULL, self->player);
}

/* Instance methods */
GtkWidget *
polyhymnia_player_bar_get_queue_toggle_button (const PolyhymniaPlayerBar *self)
{
  return GTK_WIDGET (self->queue_button);
}

#ifdef POLYHYMNIA_FEATURE_LYRICS
GtkWidget *
polyhymnia_player_bar_get_lyrics_toggle_button (const PolyhymniaPlayerBar *self)
{
  return GTK_WIDGET (self->lyrics_button);
}
#endif

/* Event handler implementation */
static void
polyhymnia_player_bar_current_track(PolyhymniaPlayerBar *self,
                                    GParamSpec          *pspec,
                                    PolyhymniaPlayer    *user_data)
{
  const PolyhymniaTrack *current_track;

  g_return_if_fail (POLYHYMNIA_IS_PLAYER_BAR (self));

  current_track = polyhymnia_player_get_current_track (user_data);
  if (current_track == NULL)
  {
    gtk_label_set_text (self->playback_total_label, NULL);

    gtk_label_set_text (self->current_track_artist_label, NULL);
    gtk_label_set_text (self->current_track_title_label, NULL);

    gtk_adjustment_set_upper (self->playback_adjustment, 0);
    gtk_image_set_from_icon_name (self->current_track_cover_image,
                                  "image-missing-symbolic");
    g_clear_object (&self->current_track_album_cover);
#ifdef POLYHYMNIA_FEATURE_LYRICS
    gtk_toggle_button_set_active (self->lyrics_button, FALSE);
#endif
  }
  else
  {
    GError *error = NULL;
    GBytes *cover = polyhymnia_player_get_current_track_album_cover (self->player,
                                                                     &error);
    if (error != NULL)
    {
      g_warning ("Failed to get current track cover: %s\n", error->message);
      g_error_free (error);
      error = NULL;
      gtk_image_set_from_icon_name (self->current_track_cover_image,
                                    "image-missing-symbolic");
    }
    else if (cover != NULL)
    {
      self->current_track_album_cover = gdk_texture_new_from_bytes (cover, &error);
      if (error != NULL)
      {
        g_warning ("Failed to convert current track cover: %s\n", error->message);
        g_error_free (error);
        error = NULL;
        gtk_image_set_from_icon_name (self->current_track_cover_image,
                                      "image-missing-symbolic");
      }
      else
      {
        gtk_image_set_from_paintable (self->current_track_cover_image,
                                      GDK_PAINTABLE (self->current_track_album_cover));
      }
      g_bytes_unref (cover);
    }

    gtk_label_set_text (self->playback_total_label,
                        polyhymnia_track_get_duration_readable (current_track));
    gtk_label_set_text (self->current_track_artist_label,
                        polyhymnia_track_get_artist (current_track));
    gtk_label_set_text (self->current_track_title_label,
                        polyhymnia_track_get_title (current_track));

    gtk_adjustment_set_upper (self->playback_adjustment,
                              (gdouble) polyhymnia_track_get_duration (current_track));
  }
}

static void
polyhymnia_player_bar_details_button_clicked(PolyhymniaPlayerBar *self,
                                             gpointer             user_data)
{
  const PolyhymniaTrack *current_track;

  g_return_if_fail (POLYHYMNIA_IS_PLAYER_BAR (self));

  current_track = polyhymnia_player_get_current_track (self->player);
  g_return_if_fail (current_track != NULL);
  g_signal_emit (self, obj_signals[SIGNAL_VIEW_TRACK_DETAILS], 0,
                 polyhymnia_track_get_uri (current_track));
}

static void
polyhymnia_player_bar_elapsed_seconds(PolyhymniaPlayerBar *self,
                                      GParamSpec          *pspec,
                                      PolyhymniaPlayer    *user_data)
{
  guint elapsed = polyhymnia_player_get_elapsed (user_data);
  gchar *elapsed_readable = seconds_to_readable (elapsed);

  g_return_if_fail (POLYHYMNIA_IS_PLAYER_BAR (self));

  gtk_label_set_text (self->playback_elapsed_label, elapsed_readable);
  gtk_adjustment_set_value (self->playback_adjustment, (gdouble) elapsed);

  g_free (elapsed_readable);
}

static void
polyhymnia_player_bar_next_button_clicked(PolyhymniaPlayerBar *self,
                                          gpointer            user_data)
{
  g_return_if_fail (POLYHYMNIA_IS_PLAYER_BAR (self));

  polyhymnia_player_play_next (self->player, NULL);
}

static void
polyhymnia_player_bar_play_button_clicked(PolyhymniaPlayerBar *self,
                                          gpointer            user_data)
{
  g_return_if_fail (POLYHYMNIA_IS_PLAYER_BAR (self));

  polyhymnia_player_toggle_playback_state (self->player, NULL);
}

static gboolean
polyhymnia_player_bar_playback_seek(PolyhymniaPlayerBar *self,
                                    GtkScrollType       *scroll,
                                    gdouble              value,
                                    GtkScale            *user_data)
{
  polyhymnia_player_playback_seek (self->player, value, NULL);

  return FALSE;
}

static void
polyhymnia_player_bar_previous_button_clicked(PolyhymniaPlayerBar *self,
                                              gpointer            user_data)
{
  g_return_if_fail (POLYHYMNIA_IS_PLAYER_BAR (self));

  polyhymnia_player_play_previous (self->player, NULL);
}

static void
polyhymnia_player_bar_state(PolyhymniaPlayerBar *self,
                            GParamSpec          *pspec,
                            PolyhymniaPlayer    *user_data)
{
  const char *icon_name;
  PolyhymniaPlayerPlaybackStatus player_state;

  g_return_if_fail (POLYHYMNIA_IS_PLAYER_BAR (self));

  player_state = polyhymnia_player_get_playback_status (user_data);
  icon_name = polyhymnia_player_bar_state_to_icon (player_state);
  gtk_button_set_icon_name (self->play_button, icon_name);
}

static void
polyhymnia_player_bar_volume_minus_button_clicked(PolyhymniaPlayerBar *self,
                                                  gpointer             user_data)
{
  polyhymnia_player_change_volume (self->player, -5, NULL);
}

static void
polyhymnia_player_bar_volume_plus_button_clicked(PolyhymniaPlayerBar *self,
                                                 gpointer             user_data)
{
  polyhymnia_player_change_volume (self->player, 5, NULL);
}

#ifdef POLYHYMNIA_FEATURE_LYRICS
static void
polyhymnia_player_bar_lyrics_toggled(PolyhymniaPlayerBar *self,
                                     gpointer             user_data)
{
  g_return_if_fail (POLYHYMNIA_IS_PLAYER_BAR (self));
  g_return_if_fail (GTK_IS_TOGGLE_BUTTON (user_data));

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (user_data)))
  {
    gtk_toggle_button_set_active (self->queue_button, FALSE);
  }
}

static void
polyhymnia_player_bar_queue_toggled(PolyhymniaPlayerBar *self,
                                    gpointer             user_data)
{
  g_return_if_fail (POLYHYMNIA_IS_PLAYER_BAR (self));
  g_return_if_fail (GTK_IS_TOGGLE_BUTTON (user_data));

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (user_data)))
  {
    gtk_toggle_button_set_active (self->lyrics_button, FALSE);
  }
}
#endif

/* Utility functions implementation*/
static const gchar *
polyhymnia_player_bar_state_to_icon(PolyhymniaPlayerPlaybackStatus state)
{
  switch (state)
  {
  case POLYHYMNIA_PLAYER_PLAYBACK_STATUS_PLAYING:
    return "pause-large-symbolic";
  case POLYHYMNIA_PLAYER_PLAYBACK_STATUS_UNKNOWN:
    return "play-large-disabled-symbolic";
  default:
    return "play-large-symbolic";
  }
}

