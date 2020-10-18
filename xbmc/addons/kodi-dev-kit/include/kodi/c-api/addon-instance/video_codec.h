/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#ifndef C_API_ADDONINSTANCE_VIDEOCODEC_H
#define C_API_ADDONINSTANCE_VIDEOCODEC_H

#include "../addon_base.h"
#include "inputstream/demux_packet.h"
#include "inputstream/stream_codec.h"
#include "inputstream/stream_crypto.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

  enum VIDEOCODEC_RETVAL
  {
    VC_NONE = 0, //< noop
    VC_ERROR, //< an error occurred, no other messages will be returned
    VC_BUFFER, //< the decoder needs more data
    VC_PICTURE, //< the decoder got a picture
    VC_EOF, //< the decoder signals EOF
  };

  enum VIDEOCODEC_FORMAT
  {
    VIDEOCODEC_FORMAT_UNKNOWN = 0,
    VIDEOCODEC_FORMAT_YV12,
    VIDEOCODEC_FORMAT_I420,
    VIDEOCODEC_FORMAT_MAXFORMATS
  };

  enum VIDEOCODEC_TYPE
  {
    VIDEOCODEC_UNKNOWN = 0,
    VIDEOCODEC_VP8,
    VIDEOCODEC_H264,
    VIDEOCODEC_VP9
  };

  enum VIDEOCODEC_PLANE
  {
    VIDEOCODEC_PICTURE_Y_PLANE = 0,
    VIDEOCODEC_PICTURE_U_PLANE,
    VIDEOCODEC_PICTURE_V_PLANE,
    VIDEOCODEC_PICTURE_MAXPLANES = 3,
  };

  enum VIDEOCODEC_PICTURE_FLAG
  {
    VIDEOCODEC_PICTURE_FLAG_NONE = 0,
    VIDEOCODEC_PICTURE_FLAG_DROP = (1 << 0),
    VIDEOCODEC_PICTURE_FLAG_DRAIN = (1 << 1),
  };

  struct VIDEOCODEC_INITDATA
  {
    enum VIDEOCODEC_TYPE codec;
    enum STREAMCODEC_PROFILE codecProfile;
    enum VIDEOCODEC_FORMAT* videoFormats;
    uint32_t width;
    uint32_t height;
    const uint8_t* extraData;
    unsigned int extraDataSize;
    struct STREAM_CRYPTO_SESSION cryptoSession;
  };

  struct VIDEOCODEC_PICTURE
  {
    enum VIDEOCODEC_FORMAT videoFormat;
    uint32_t flags;
    uint32_t width;
    uint32_t height;
    uint8_t* decodedData;
    size_t decodedDataSize;
    uint32_t planeOffsets[VIDEOCODEC_PICTURE_MAXPLANES];
    uint32_t stride[VIDEOCODEC_PICTURE_MAXPLANES];
    int64_t pts;
    KODI_HANDLE videoBufferHandle; //< will be passed in release_frame_buffer
  };

  // this are properties given to the addon on create
  // at this time we have no parameters for the addon
  typedef struct AddonProps_VideoCodec
  {
    int dummy;
  } AddonProps_VideoCodec;

  struct AddonInstance_VideoCodec;
  typedef struct KodiToAddonFuncTable_VideoCodec
  {
    KODI_HANDLE addonInstance;

    //! \brief Opens a codec
    bool(__cdecl* open)(const struct AddonInstance_VideoCodec* instance,
                        struct VIDEOCODEC_INITDATA* initData);

    //! \brief Reconfigures a codec
    bool(__cdecl* reconfigure)(const struct AddonInstance_VideoCodec* instance,
                               struct VIDEOCODEC_INITDATA* initData);

    //! \brief Feed codec if requested from GetPicture() (return VC_BUFFER)
    bool(__cdecl* add_data)(const struct AddonInstance_VideoCodec* instance,
                            const struct DEMUX_PACKET* packet);

    //! \brief Get a decoded picture / request new data
    enum VIDEOCODEC_RETVAL(__cdecl* get_picture)(const struct AddonInstance_VideoCodec* instance,
                                                 struct VIDEOCODEC_PICTURE* picture);

    //! \brief Get the name of this video decoder
    const char*(__cdecl* get_name)(const struct AddonInstance_VideoCodec* instance);

    //! \brief Reset the codec
    void(__cdecl* reset)(const struct AddonInstance_VideoCodec* instance);
  } KodiToAddonFuncTable_VideoCodec;

  typedef struct AddonToKodiFuncTable_VideoCodec
  {
    KODI_HANDLE kodiInstance;
    bool (*get_frame_buffer)(void* kodiInstance, struct VIDEOCODEC_PICTURE* picture);
    void (*release_frame_buffer)(void* kodiInstance, void* buffer);
  } AddonToKodiFuncTable_VideoCodec;

  typedef struct AddonInstance_VideoCodec
  {
    struct AddonProps_VideoCodec* props;
    struct AddonToKodiFuncTable_VideoCodec* toKodi;
    struct KodiToAddonFuncTable_VideoCodec* toAddon;
  } AddonInstance_VideoCodec;

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* !C_API_ADDONINSTANCE_VIDEOCODEC_H */
