
#pragma once

#include <glib-object.h>
#include <gio/gio.h>

G_BEGIN_DECLS

typedef struct
{
  const char *album_musicbrainz_id;
  const char *album_name;
  const char *artist_name;
} PolyhymniaSearchAlbumInfoRequest;

typedef struct
{
  char *description_full;
  char *description_summary;
} PolyhymniaSearchAlbumInfoResponse;

void
polyhymnia_search_album_info_response_free (PolyhymniaSearchAlbumInfoResponse *self);

typedef struct
{
  const char *artist_musicbrainz_id;
  const char *artist_name;
} PolyhymniaSearchArtistInfoRequest;

typedef struct
{
  char *bio_full;
  char *bio_summary;
} PolyhymniaSearchArtistInfoResponse;

typedef struct
{
  char *name;
  char *music_brainz_id;
  char *url;
  char *image;
} PolyhymniaSimilarArtist;

void
polyhymnia_search_artist_info_response_free (PolyhymniaSearchArtistInfoResponse *self);

void
polyhymnia_similar_artist_clear (PolyhymniaSimilarArtist *self);

typedef struct
{
  const char *artist_name;
  const char *track_musicbrainz_id;
  const char *track_name;
} PolyhymniaSearchTrackInfoRequest;

typedef struct
{
  char *description_full;
  char *description_summary;
} PolyhymniaSearchTrackInfoResponse;

void
polyhymnia_search_track_info_response_free (PolyhymniaSearchTrackInfoResponse *self);

#define POLYHYMNIA_TYPE_ADDITIONAL_INFO_PROVIDER (polyhymnia_additional_info_provider_get_type())

G_DECLARE_FINAL_TYPE (PolyhymniaAdditionalInfoProvider, polyhymnia_additional_info_provider, POLYHYMNIA, ADDITIONAL_INFO_PROVIDER, GObject)

void
polyhymnia_additional_info_provider_search_album_info_async (PolyhymniaAdditionalInfoProvider       *self,
                                                             const PolyhymniaSearchAlbumInfoRequest *request,
                                                             GCancellable                           *cancellable,
                                                             GAsyncReadyCallback                     callback,
                                                             void                                   *user_data);

PolyhymniaSearchAlbumInfoResponse *
polyhymnia_additional_info_provider_search_album_info_finish (PolyhymniaAdditionalInfoProvider *self,
                                                              GAsyncResult                     *result,
                                                              GError                          **error);

void
polyhymnia_additional_info_provider_search_artist_info_async (PolyhymniaAdditionalInfoProvider        *self,
                                                              const PolyhymniaSearchArtistInfoRequest *request,
                                                              GCancellable                            *cancellable,
                                                              GAsyncReadyCallback                      callback,
                                                              void                                    *user_data);

PolyhymniaSearchArtistInfoResponse *
polyhymnia_additional_info_provider_search_artist_info_finish (PolyhymniaAdditionalInfoProvider *self,
                                                               GAsyncResult                     *result,
                                                               GError                          **error);

void
polyhymnia_additional_info_provider_search_artist_similar_async (PolyhymniaAdditionalInfoProvider        *self,
                                                                 const PolyhymniaSearchArtistInfoRequest *request,
                                                                 GCancellable                            *cancellable,
                                                                 GAsyncReadyCallback                      callback,
                                                                 void                                    *user_data);

GArray *
polyhymnia_additional_info_provider_search_artist_similar_finish (PolyhymniaAdditionalInfoProvider *self,
                                                                  GAsyncResult                     *result,
                                                                  GError                          **error);

void
polyhymnia_additional_info_provider_search_track_info_async (PolyhymniaAdditionalInfoProvider       *self,
                                                             const PolyhymniaSearchTrackInfoRequest *request,
                                                             GCancellable                           *cancellable,
                                                             GAsyncReadyCallback                     callback,
                                                             void                                   *user_data);

PolyhymniaSearchTrackInfoResponse *
polyhymnia_additional_info_provider_search_track_info_finish (PolyhymniaAdditionalInfoProvider *self,
                                                              GAsyncResult                     *result,
                                                              GError                          **error);

G_END_DECLS
