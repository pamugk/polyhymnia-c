
#include <gdk/gdk.h>
#include "polyhymnia-player.h"
#include "polyhymnia-player-bar.h"

/* Type metadata */
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

  /* Template objects */
  PolyhymniaPlayer    *player;
};

G_DEFINE_FINAL_TYPE (PolyhymniaPlayerBar, polyhymnia_player_bar, GTK_TYPE_WIDGET)

/* Utility functions declaration */
static const gchar *
polyhymnia_player_bar_state_to_icon(PolyhymniaPlayerPlaybackStatus state);

/* Event handler declaration  */
static void
polyhymnia_player_bar_current_track(PolyhymniaPlayerBar *self,
                                    GParamSpec          *pspec,
                                    PolyhymniaPlayer    *user_data);

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

static void
polyhymnia_player_bar_previous_button_clicked(PolyhymniaPlayerBar *self,
                                              gpointer             user_data);

static void
polyhymnia_player_bar_state(PolyhymniaPlayerBar *self,
                            GParamSpec          *pspec,
                            PolyhymniaPlayer    *user_data);

/* Class stuff */
static void
polyhymnia_player_bar_dispose(GObject *gobject)
{
  PolyhymniaPlayerBar *self = POLYHYMNIA_PLAYER_BAR (gobject);

  gtk_widget_unparent (GTK_WIDGET (self->root_action_bar));
  gtk_widget_dispose_template (GTK_WIDGET (self), POLYHYMNIA_TYPE_PLAYER_BAR);

  G_OBJECT_CLASS (polyhymnia_player_bar_parent_class)->dispose (gobject);
}

static void
polyhymnia_player_bar_class_init (PolyhymniaPlayerBarClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->dispose = polyhymnia_player_bar_dispose;

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

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaPlayerBar, player);

  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_player_bar_next_button_clicked);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_player_bar_play_button_clicked);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_player_bar_previous_button_clicked);

  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_player_bar_current_track);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_player_bar_elapsed_seconds);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_player_bar_state);
}

static void
polyhymnia_player_bar_init (PolyhymniaPlayerBar *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

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
    gtk_label_set_text (self->current_track_artist_label, NULL);
    gtk_label_set_text (self->current_track_title_label, NULL);

    gtk_adjustment_set_upper (self->playback_adjustment, 0);
    gtk_image_set_from_icon_name (self->current_track_cover_image,
                                  "image-missing-symbolic");
    g_clear_object (&self->current_track_album_cover);
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
        g_bytes_unref (cover);
        gtk_image_set_from_icon_name (self->current_track_cover_image,
                                      "image-missing-symbolic");
      }
      else
      {
        gtk_image_set_from_paintable (self->current_track_cover_image,
                                      GDK_PAINTABLE (self->current_track_album_cover));
      }
    }

    gtk_label_set_text (self->current_track_artist_label,
                        polyhymnia_track_get_artist (current_track));
    gtk_label_set_text (self->current_track_title_label,
                        polyhymnia_track_get_title (current_track));

    gtk_adjustment_set_upper (self->playback_adjustment,
                              (gdouble) polyhymnia_track_get_duration (current_track));
  }
}

static void
polyhymnia_player_bar_elapsed_seconds(PolyhymniaPlayerBar *self,
                                      GParamSpec          *pspec,
                                      PolyhymniaPlayer    *user_data)
{
  g_return_if_fail (POLYHYMNIA_IS_PLAYER_BAR (self));

  gtk_adjustment_set_value (self->playback_adjustment,
                            (gdouble) polyhymnia_player_get_elapsed (user_data));
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
