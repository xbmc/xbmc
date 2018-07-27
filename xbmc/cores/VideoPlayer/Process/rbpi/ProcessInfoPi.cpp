/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include <interface/mmal/util/mmal_default_components.h>
#include "ProcessInfoPi.h"
#include "platform/linux/RBP.h"
#include "cores/VideoPlayer/DVDCodecs/Video/MMALFFmpeg.h"

// Override for platform ports
#if defined(TARGET_RASPBERRY_PI)

using namespace MMAL;

CProcessInfo* CProcessInfoPi::Create()
{
  return new CProcessInfoPi();
}

CProcessInfoPi::CProcessInfoPi()
{
  /* Create dummy component with attached pool */
  std::shared_ptr<IVideoBufferPool> pool = std::make_shared<CMMALPool>(MMAL_COMPONENT_DEFAULT_VIDEO_DECODER, false, MMAL_NUM_OUTPUT_BUFFERS, 0, MMAL_ENCODING_UNKNOWN, MMALStateFFDec);
  m_videoBufferManager.RegisterPool(pool);
}

void CProcessInfoPi::Register()
{
  CProcessInfo::RegisterProcessControl("rbpi", CProcessInfoPi::Create);
}

EINTERLACEMETHOD CProcessInfoPi::GetFallbackDeintMethod()
{
  return EINTERLACEMETHOD::VS_INTERLACEMETHOD_DEINTERLACE_HALF;
}

bool CProcessInfoPi::AllowDTSHDDecode()
{
  if (g_RBP.RaspberryPiVersion() == 1)
    return false;
  return true;
}

#endif

