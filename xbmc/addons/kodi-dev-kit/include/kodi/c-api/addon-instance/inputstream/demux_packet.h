/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#ifndef C_API_ADDONINSTANCE_INPUTSTREAM_DEMUXPACKET_H
#define C_API_ADDONINSTANCE_INPUTSTREAM_DEMUXPACKET_H

#include "timing_constants.h"

#include <stdbool.h>
#include <stdint.h>

#define DEMUX_SPECIALID_STREAMINFO -10
#define DEMUX_SPECIALID_STREAMCHANGE -11

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

  struct DEMUX_CRYPTO_INFO;

  //============================================================================
  /// @defgroup cpp_kodi_addon_inputstream_Defs_Interface_DEMUX_PACKET struct DEMUX_PACKET
  /// @ingroup cpp_kodi_addon_inputstream_Defs_Interface
  /// @brief **Demux packet**\n
  /// To processed codec and demux inputstream stream.
  ///
  /// This part is in the "C" style in order to have better performance and
  /// possibly to be used in "C" libraries.
  ///
  /// The structure should be created with @ref kodi::addon::CInstanceInputStream::AllocateDemuxPacket()
  /// or @ref kodi::addon::CInstanceInputStream::AllocateEncryptedDemuxPacket()
  /// and if not added to Kodi with @ref kodi::addon::CInstanceInputStream::FreeDemuxPacket()
  /// be deleted again.
  ///
  /// Packages that have been given to Kodi and processed will then be deleted
  /// by him.
  ///
  ///@{
  struct DEMUX_PACKET
  {
    /// @brief Stream package which is given for decoding.
    ///
    /// @note Associated storage from here is created using
    /// @ref kodi::addon::CInstanceInputStream::AllocateDemuxPacket()
    /// or @ref kodi::addon::CInstanceInputStream::AllocateEncryptedDemuxPacket().
    uint8_t* pData;

    /// @brief Size of the package given at @ref pData.
    int iSize;

    /// @brief Identification of the stream.
    int iStreamId;

    /// @brief Identification of the associated demuxer, this can be identical
    /// on several streams.
    int64_t demuxerId;

    /// @brief The group this data belongs to, used to group data from different
    /// streams together.
    int iGroupId;

    //------------------------------------------

    /// @brief Additional packet data that can be provided by the container.
    ///
    /// Packet can contain several types of side information.
    ///
    /// This is usually based on that of ffmpeg, see
    /// [AVPacketSideData](https://ffmpeg.org/doxygen/trunk/structAVPacketSideData.html).
    void* pSideData;

    /// @brief Data elements stored at @ref pSideData.
    int iSideDataElems;

    //------------------------------------------

    /// @brief Presentation time stamp (PTS).
    double pts;

    /// @brief Decoding time stamp (DTS).
    double dts;

    /// @brief Duration in @ref STREAM_TIME_BASE if available
    double duration;

    /// @brief Display time from input stream
    int dispTime;

    /// @brief To show that this package allows recreating the presentation by
    /// mistake.
    bool recoveryPoint;

    //------------------------------------------

    /// @brief Optional data to allow decryption at processing site if
    /// necessary.
    ///
    /// This can be created using @ref kodi::addon::CInstanceInputStream::AllocateEncryptedDemuxPacket(),
    /// otherwise this is declared as <b>`nullptr`</b>.
    ///
    /// See @ref DEMUX_CRYPTO_INFO for their style.
    struct DEMUX_CRYPTO_INFO* cryptoInfo;
  };
  ///@}
  //----------------------------------------------------------------------------

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* !C_API_ADDONINSTANCE_INPUTSTREAM_DEMUXPACKET_H */
