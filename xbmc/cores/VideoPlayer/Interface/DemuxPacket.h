/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "TimingConstants.h"
#include "addons/kodi-dev-kit/include/kodi/c-api/addon-instance/inputstream/demux_packet.h"

#define DMX_SPECIALID_STREAMINFO DEMUX_SPECIALID_STREAMINFO
#define DMX_SPECIALID_STREAMCHANGE DEMUX_SPECIALID_STREAMCHANGE

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

  struct DemuxPacket : DEMUX_PACKET
  {
    DemuxPacket()
    {
      pData = nullptr;
      iSize = 0;
      iStreamId = -1;
      demuxerId = -1;
      iGroupId = -1;

      pSideData = nullptr;
      iSideDataElems = 0;

      pts = DVD_NOPTS_VALUE;
      dts = DVD_NOPTS_VALUE;
      duration = 0;
      dispTime = 0;
      recoveryPoint = false;

      cryptoInfo = nullptr;
    }

    //! @brief PTS offset correction applied to the PTS and DTS.
    double m_ptsOffsetCorrection{0};

    //! @brief Whether the packet holds a keyframe, as reported by the demuxer.
    //! Unlike recoveryPoint this says nothing about where output may start: an
    //! open GOP keyframe is followed by leading pictures that reference frames
    //! before it. Packets from inputstream addons leave this false, since the
    //! field is not part of the DEMUX_PACKET ABI.
    bool m_keyFrame{false};
  };

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */
