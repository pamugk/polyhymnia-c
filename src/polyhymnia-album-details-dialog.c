
#include "config.h"

#include "polyhymnia-album-details-dialog.h"

#include "polyhymnia-additional-info-provider.h"

#define _(x) g_dgettext (GETTEXT_PACKAGE, x)

/* Type metadata */
typedef enum
{
  PROP_ALBUM_NAME = 1,
  PROP_ALBUM_ARTIST_NAME,
  PROP_ALBUM_MUSICBRAINZ_ID,
  N_PROPERTIES,
} PolyhymniaAlbumDetailsDialogProperty;

struct _PolyhymniaAlbumDetailsDialog
{
  AdwDialog parent_instance;

  /* Template widgets */
  AdwToolbarView                   *root_toolbar_view;

  AdwStatusPage                    *status_page;
  AdwClamp                         *main_content;
  GtkScrolledWindow                *main_scrolled_window;
  GtkSpinner                       *spinner;

  GtkLabel                         *additional_info_label;

  /* Template objects */
  PolyhymniaAdditionalInfoProvider *additional_info_provider;
  GCancellable                     *additional_info_cancellable;

  /* Instance properties */
  gchar                            *album_artist_name;
  gchar                            *album_name;
  gchar                            *album_musicbrainz_id;
};

G_DEFINE_FINAL_TYPE (PolyhymniaAlbumDetailsDialog, polyhymnia_album_details_dialog, ADW_TYPE_DIALOG)

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

/* Event handlers declaration */
static void
polyhymnia_album_details_dialog_search_additional_info_callback (GObject      *source,
                                                                 GAsyncResult *result,
                                                                 gpointer      user_data);

/* Class stuff */
static void
polyhymnia_album_details_dialog_constructed (GObject *gobject)
{
  PolyhymniaSearchAlbumInfoRequest request;
  PolyhymniaAlbumDetailsDialog    *self = POLYHYMNIA_ALBUM_DETAILS_DIALOG (gobject);

  request.artist_name = self->album_artist_name;
  request.album_name = self->album_name;
  request.album_musicbrainz_id = self->album_musicbrainz_id;
  polyhymnia_additional_info_provider_search_album_info_async (self->additional_info_provider,
                                                               &request,
                                                               self->additional_info_cancellable,
                                                               polyhymnia_album_details_dialog_search_additional_info_callback,
                                                               self);

  G_OBJECT_CLASS (polyhymnia_album_details_dialog_parent_class)->constructed (gobject);
}

static void
polyhymnia_album_details_dialog_dispose(GObject *gobject)
{
  PolyhymniaAlbumDetailsDialog *self = POLYHYMNIA_ALBUM_DETAILS_DIALOG (gobject);

  g_cancellable_cancel (self->additional_info_cancellable);

  gtk_widget_dispose_template (GTK_WIDGET (self), POLYHYMNIA_TYPE_ALBUM_DETAILS_DIALOG);

  g_clear_pointer (&(self->album_name), g_free);
  g_clear_pointer (&(self->album_artist_name), g_free);
  g_clear_pointer (&(self->album_musicbrainz_id), g_free);

  G_OBJECT_CLASS (polyhymnia_album_details_dialog_parent_class)->dispose (gobject);
}

static void
polyhymnia_album_details_dialog_get_property (GObject    *object,
                                              guint       property_id,
                                              GValue     *value,
                                              GParamSpec *pspec)
{
  PolyhymniaAlbumDetailsDialog *self = POLYHYMNIA_ALBUM_DETAILS_DIALOG (object);

  switch ((PolyhymniaAlbumDetailsDialogProperty) property_id)
    {
    case PROP_ALBUM_NAME:
      g_value_set_string (value, self->album_name);
      break;
    case PROP_ALBUM_ARTIST_NAME:
      g_value_set_string (value, self->album_artist_name);
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
polyhymnia_album_details_dialog_set_property (GObject      *object,
                                              guint         property_id,
                                              const GValue *value,
                                              GParamSpec   *pspec)
{
  PolyhymniaAlbumDetailsDialog *self = POLYHYMNIA_ALBUM_DETAILS_DIALOG (object);

  switch ((PolyhymniaAlbumDetailsDialogProperty) property_id)
    {
    case PROP_ALBUM_NAME:
      g_set_str (&(self->album_name), g_value_get_string (value));
      break;
    case PROP_ALBUM_ARTIST_NAME:
      g_set_str (&(self->album_artist_name), g_value_get_string (value));
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
polyhymnia_album_details_dialog_class_init (PolyhymniaAlbumDetailsDialogClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->constructed = polyhymnia_album_details_dialog_constructed;
  gobject_class->dispose = polyhymnia_album_details_dialog_dispose;
  gobject_class->get_property = polyhymnia_album_details_dialog_get_property;
  gobject_class->set_property = polyhymnia_album_details_dialog_set_property;

  obj_properties[PROP_ALBUM_NAME] =
    g_param_spec_string ("album-name",
                         "Album name",
                         "Name of an album.",
                         NULL,
                         G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_NAME);
  obj_properties[PROP_ALBUM_ARTIST_NAME] =
    g_param_spec_string ("album-artist-name",
                         "Album artist name",
                         "Name of an artist performing album.",
                         NULL,
                         G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_NAME);
  obj_properties[PROP_ALBUM_MUSICBRAINZ_ID] =
    g_param_spec_string ("album-musicbrainz-id",
                         "Album MusicBrainz id",
                         "Album identifier in MusicBrainz database.",
                         NULL,
                         G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_NAME);

  g_object_class_install_properties (gobject_class,
                                     N_PROPERTIES,
                                     obj_properties);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/com/github/pamugk/polyhymnia/ui/polyhymnia-album-details-dialog.ui");

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaAlbumDetailsDialog, root_toolbar_view);

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaAlbumDetailsDialog, status_page);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaAlbumDetailsDialog, main_content);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaAlbumDetailsDialog, main_scrolled_window);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaAlbumDetailsDialog, spinner);

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaAlbumDetailsDialog, additional_info_label);

  gtk_widget_class_bind_template_child (widget_class, PolyhymniaAlbumDetailsDialog, additional_info_provider);
  gtk_widget_class_bind_template_child (widget_class, PolyhymniaAlbumDetailsDialog, additional_info_cancellable);
}

static void
polyhymnia_album_details_dialog_init (PolyhymniaAlbumDetailsDialog *self)
{
  g_type_ensure (POLYHYMNIA_TYPE_ADDITIONAL_INFO_PROVIDER);

  gtk_widget_init_template (GTK_WIDGET (self));

  gtk_scrolled_window_set_child (self->main_scrolled_window,
                                 GTK_WIDGET (self->spinner));
  gtk_spinner_start (self->spinner);
}

/* Event handlers implementation */
static void
polyhymnia_album_details_dialog_search_additional_info_callback (GObject      *source,
                                                                 GAsyncResult *result,
                                                                 gpointer      user_data)
{
  GError                            *error = NULL;
  PolyhymniaSearchAlbumInfoResponse *response;
  PolyhymniaAlbumDetailsDialog      *self;

  response = polyhymnia_additional_info_provider_search_album_info_finish (POLYHYMNIA_ADDITIONAL_INFO_PROVIDER (source),
                                                                           result,
                                                                           &error);
  if (error != NULL)
  {
    if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
    {
      return;
    }
    else
    {
      self = POLYHYMNIA_ALBUM_DETAILS_DIALOG (user_data);
      g_object_set (G_OBJECT (self->status_page),
                    "description", _("Failed to find additional info"),
                    NULL);
      gtk_scrolled_window_set_child (self->main_scrolled_window,
                                     GTK_WIDGET (self->status_page));
    }
  }
  else if (response == NULL || response->description_full == NULL)
  {
    self = POLYHYMNIA_ALBUM_DETAILS_DIALOG (user_data);
    g_object_set (G_OBJECT (self->status_page),
                  "description", _("No additional info found"),
                  NULL);
    gtk_scrolled_window_set_child (self->main_scrolled_window,
                                   GTK_WIDGET (self->status_page));
  }
  else
  {
    self = POLYHYMNIA_ALBUM_DETAILS_DIALOG (user_data);
    gtk_label_set_label (self->additional_info_label, response->description_full);

    gtk_scrolled_window_set_child (self->main_scrolled_window,
                                   GTK_WIDGET (self->main_content));
    adw_toolbar_view_set_reveal_bottom_bars (self->root_toolbar_view,
                                             TRUE);
  }

  gtk_spinner_stop (self->spinner);

  polyhymnia_search_album_info_response_free (response);
  self->additional_info_cancellable = NULL;
}
