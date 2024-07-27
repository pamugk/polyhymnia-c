
#include "app-features.h"

#include "polyhymnia-album-header.h"

/* Type metadata */
typedef enum
{
  PROP_ALBUM_COVER = 1,
  PROP_ALBUM_TITLE,
  PROP_ALBUM_RELEASE,
  PROP_ALBUM_MUSICBRAINZ_ID,
  N_PROPERTIES,
} PolyhymniaAlbumHeaderProperty;

typedef enum
{
  SIGNAL_ENQUEUE = 1,
  SIGNAL_PLAY,
#ifdef POLYHYMNIA_FEATURE_EXTERNAL_DATA
  SIGNAL_REQUEST_INFO,
#endif
  N_SIGNALS,
} PolyhymniaAlbumHeaderSignal;

struct _PolyhymniaAlbumHeader
{
  GtkWidget  parent_instance;

  /* Template widgets */
  GtkImage            *album_cover_image;
  GtkLabel            *album_release_label;
  GtkLabel            *album_title_label;
  GtkButton           *enqueue_album_button;
  GtkButton           *play_album_button;
  GtkConstraintLayout *root_layout_manager;

  /* Instance properties */
  gchar               *album_title;
  gchar               *album_release;
  gchar               *album_musicbrainz_id;

// Feature-specific properties
#ifdef POLYHYMNIA_FEATURE_EXTERNAL_DATA
  GtkButton           *show_album_additional_info_button;
#endif
};

G_DEFINE_FINAL_TYPE (PolyhymniaAlbumHeader, polyhymnia_album_header, GTK_TYPE_WIDGET)

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

static guint obj_signals[N_SIGNALS] = { 0, };

/* Event handler declarations */
static void
polyhymnia_album_header_add_album_to_queue_button_clicked (PolyhymniaAlbumHeader *self,
                                                           GtkButton        *user_data);

static void
polyhymnia_album_header_play_album_button_clicked (PolyhymniaAlbumHeader *self,
                                                   GtkButton        *user_data);

#ifdef POLYHYMNIA_FEATURE_EXTERNAL_DATA
static void
polyhymnia_album_header_show_album_additional_info_button_clicked (PolyhymniaAlbumHeader *self,
                                                                   GtkButton        *user_data);
#endif

/* Class stuff */
static void
polyhymnia_album_header_dispose(GObject *gobject)
{
  PolyhymniaAlbumHeader *self = POLYHYMNIA_ALBUM_HEADER (gobject);

  gtk_widget_unparent (GTK_WIDGET (self->album_cover_image));
  gtk_widget_unparent (GTK_WIDGET (self->album_release_label));
  gtk_widget_unparent (GTK_WIDGET (self->album_title_label));
  gtk_widget_unparent (GTK_WIDGET (self->enqueue_album_button));
  gtk_widget_unparent (GTK_WIDGET (self->play_album_button));
#ifdef POLYHYMNIA_FEATURE_EXTERNAL_DATA
  gtk_widget_unparent (GTK_WIDGET (self->show_album_additional_info_button));
#endif
  gtk_widget_dispose_template (GTK_WIDGET (self), POLYHYMNIA_TYPE_ALBUM_HEADER);
  g_clear_pointer (&(self->album_release), g_free);
  g_clear_pointer (&(self->album_title), g_free);
  g_clear_pointer (&(self->album_musicbrainz_id), g_free);

  G_OBJECT_CLASS (polyhymnia_album_header_parent_class)->dispose (gobject);
}

static void
polyhymnia_album_header_get_property (GObject    *object,
                                    guint       property_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
  PolyhymniaAlbumHeader *self = POLYHYMNIA_ALBUM_HEADER (object);

  switch ((PolyhymniaAlbumHeaderProperty) property_id)
    {
    case PROP_ALBUM_TITLE:
      g_value_set_string (value, self->album_title);
      break;
    case PROP_ALBUM_RELEASE:
      g_value_set_string (value, self->album_release);
      break;
    case PROP_ALBUM_MUSICBRAINZ_ID:
      g_value_set_string (value, self->album_musicbrainz_id);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
polyhymnia_album_header_set_property (GObject      *object,
                                    guint         property_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
  PolyhymniaAlbumHeader *self = POLYHYMNIA_ALBUM_HEADER (object);

  switch ((PolyhymniaAlbumHeaderProperty) property_id)
    {
    case PROP_ALBUM_COVER:
    {
      GdkPaintable *cover = g_value_get_object (value);
      if (cover == NULL)
      {
        gtk_image_set_from_icon_name (self->album_cover_image, "cd-symbolic");
      }
      else
      {
        gtk_image_set_from_paintable (self->album_cover_image, cover);
      }
      break;
    }
    case PROP_ALBUM_TITLE:
      g_set_str (&(self->album_title), g_value_get_string (value));
      break;
    case PROP_ALBUM_RELEASE:
      g_set_str (&(self->album_release), g_value_get_string (value));
      break;
    case PROP_ALBUM_MUSICBRAINZ_ID:
      g_set_str (&(self->album_musicbrainz_id), g_value_get_string (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
polyhymnia_album_header_class_init (PolyhymniaAlbumHeaderClass *klass)
{
  GObjectClass   *gobject_class = G_OBJECT_CLASS (klass);
  GType          type = G_TYPE_FROM_CLASS (gobject_class);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  gobject_class->dispose = polyhymnia_album_header_dispose;
  gobject_class->get_property = polyhymnia_album_header_get_property;
  gobject_class->set_property = polyhymnia_album_header_set_property;

  obj_properties[PROP_ALBUM_COVER] =
    g_param_spec_object ("album-cover",
                         "Album cover",
                         "Cover of a displayed album",
                         GDK_TYPE_PAINTABLE,
                         G_PARAM_WRITABLE | G_PARAM_STATIC_NAME);
  obj_properties[PROP_ALBUM_TITLE] =
    g_param_spec_string ("album-title",
                         "Album title",
                         "Title of a displayed album.",
                         NULL,
                         G_PARAM_READWRITE | G_PARAM_STATIC_NAME);
  obj_properties[PROP_ALBUM_RELEASE] =
    g_param_spec_string ("album-release",
                         "Album release",
                         "Release date of a displayed album.",
                         NULL,
                         G_PARAM_READWRITE | G_PARAM_STATIC_NAME);
  obj_properties[PROP_ALBUM_MUSICBRAINZ_ID] =
    g_param_spec_string ("album-musicbrainz-id",
                         "Album MusicBrainz id",
                         "Album identifier in MusicBrainz database.",
                         NULL,
                         G_PARAM_READWRITE | G_PARAM_STATIC_NAME);

  g_object_class_install_properties (gobject_class,
                                     N_PROPERTIES,
                                     obj_properties);

  obj_signals[SIGNAL_ENQUEUE] =
     g_signal_newv ("enqueued", type,
                    G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                    NULL, NULL, NULL, NULL,
                    G_TYPE_NONE,
                    0, NULL);
  obj_signals[SIGNAL_PLAY] =
     g_signal_newv ("played", type,
                    G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                    NULL, NULL, NULL, NULL,
                    G_TYPE_NONE,
                    0, NULL);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/com/github/pamugk/polyhymnia/ui/polyhymnia-album-header.ui");

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaAlbumHeader, album_cover_image);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaAlbumHeader, album_release_label);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaAlbumHeader, album_title_label);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaAlbumHeader, enqueue_album_button);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaAlbumHeader, play_album_button);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaAlbumHeader, root_layout_manager);

  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_album_header_add_album_to_queue_button_clicked);
  gtk_widget_class_bind_template_callback (widget_class,
                                           polyhymnia_album_header_play_album_button_clicked);

#ifdef POLYHYMNIA_FEATURE_EXTERNAL_DATA
  obj_signals[SIGNAL_REQUEST_INFO] =
     g_signal_newv ("info-requested", type,
                    G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                    NULL, NULL, NULL, NULL,
                    G_TYPE_NONE,
                    0, NULL);
#endif
}

static void
polyhymnia_album_header_init (PolyhymniaAlbumHeader *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

#ifdef POLYHYMNIA_FEATURE_EXTERNAL_DATA
  self->show_album_additional_info_button = GTK_BUTTON (gtk_button_new_from_icon_name ("info-symbolic"));
  g_signal_connect_swapped (self->show_album_additional_info_button, "clicked",
                            (GCallback) polyhymnia_album_header_show_album_additional_info_button_clicked,
                            self);

  gtk_widget_insert_after (GTK_WIDGET (self->show_album_additional_info_button),
                           GTK_WIDGET (self), NULL);
  gtk_constraint_layout_add_constraint (self->root_layout_manager,
                                        gtk_constraint_new (self->show_album_additional_info_button,
                                                            GTK_CONSTRAINT_ATTRIBUTE_BOTTOM,
                                                            GTK_CONSTRAINT_RELATION_EQ,
                                                            NULL,
                                                            GTK_CONSTRAINT_ATTRIBUTE_BOTTOM,
                                                            1,
                                                            -6,
                                                            GTK_CONSTRAINT_STRENGTH_REQUIRED));
  gtk_constraint_layout_add_constraint (self->root_layout_manager,
                                        gtk_constraint_new (self->show_album_additional_info_button,
                                                            GTK_CONSTRAINT_ATTRIBUTE_START,
                                                            GTK_CONSTRAINT_RELATION_EQ,
                                                            self->enqueue_album_button,
                                                            GTK_CONSTRAINT_ATTRIBUTE_END,
                                                            1,
                                                            12,
                                                            GTK_CONSTRAINT_STRENGTH_REQUIRED));
#endif
}

/* Instance method implementations */
const gchar *
polyhymnia_album_header_get_album_musicbrainz_id (PolyhymniaAlbumHeader *self)
{
  g_return_val_if_fail (POLYHYMNIA_IS_ALBUM_HEADER (self), NULL);
  return self->album_musicbrainz_id;
}

const gchar *
polyhymnia_album_header_get_album_title (PolyhymniaAlbumHeader *self)
{
  g_return_val_if_fail (POLYHYMNIA_IS_ALBUM_HEADER (self), NULL);
  return self->album_title;
}

/* Event handler implementations */
static void
polyhymnia_album_header_add_album_to_queue_button_clicked (PolyhymniaAlbumHeader *self,
                                                           GtkButton        *user_data)
{
  g_assert (POLYHYMNIA_IS_ALBUM_HEADER (self));

  if (self->album_title != NULL)
  {
    g_signal_emit (self, obj_signals[SIGNAL_ENQUEUE], 0);
  }
}

static void
polyhymnia_album_header_play_album_button_clicked (PolyhymniaAlbumHeader *self,
                                                   GtkButton        *user_data)
{
  g_assert (POLYHYMNIA_IS_ALBUM_HEADER (self));

  if (self->album_title != NULL)
  {
    g_signal_emit (self, obj_signals[SIGNAL_PLAY], 0);
  }
}

#ifdef POLYHYMNIA_FEATURE_EXTERNAL_DATA
static void
polyhymnia_album_header_show_album_additional_info_button_clicked (PolyhymniaAlbumHeader *self,
                                                                   GtkButton        *user_data)
{
  g_assert (POLYHYMNIA_IS_ALBUM_HEADER (self));

  g_signal_emit (self, obj_signals[SIGNAL_REQUEST_INFO], 0);
}
#endif
