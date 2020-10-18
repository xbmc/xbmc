/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

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

  struct DEMUX_CRYPTO_INFO
  {
    uint16_t numSubSamples; // number of subsamples
    uint16_t flags; // flags for later use

    uint16_t*
        clearBytes; // numSubSamples uint16_t's wich define the size of clear size of a subsample
    uint32_t*
        cipherBytes; // numSubSamples uint32_t's wich define the size of cipher size of a subsample

    uint8_t iv[16]; // initialization vector
    uint8_t kid[16]; // key id
  };

  struct DEMUX_PACKET
  {
    uint8_t* pData;
    int iSize;
    int iStreamId;
    int64_t demuxerId; // id of the demuxer that created the packet
    int iGroupId; // the group this data belongs to, used to group data from different streams
                  // together

    void* pSideData;
    int iSideDataElems;

    double pts;
    double dts;
    double duration; // duration in DVD_TIME_BASE if available
    int dispTime;
    bool recoveryPoint;

    struct DEMUX_CRYPTO_INFO* cryptoInfo;
  };

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* !C_API_ADDONINSTANCE_INPUTSTREAM_DEMUXPACKET_H */
