/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RPProcessInfoAmlogic.h"
#include "utils/AMLUtils.h"
#include "utils/log.h"

using namespace KODI;
using namespace RETRO;

CRPProcessInfoAmlogic::CRPProcessInfoAmlogic() :
  CRPProcessInfo("Amlogic")
{
}

CRPProcessInfo* CRPProcessInfoAmlogic::Create()
{
  return new CRPProcessInfoAmlogic();
}

void CRPProcessInfoAmlogic::Register()
{
  CRPProcessInfo::RegisterProcessControl(CRPProcessInfoAmlogic::Create);
}

void CRPProcessInfoAmlogic::ConfigureRenderSystem(AVPixelFormat format)
{
  if (format == AV_PIX_FMT_0RGB32 || format == AV_PIX_FMT_0BGR32)
  {
    /*  Set the Amlogic chip to ignore the alpha channel.
     *  The proprietary OpenGL lib does not (currently)
     *  handle this, potentially resulting in a black screen.
     *  This capability is only present in S905 chips and higher.
     */
    if (aml_set_reg_ignore_alpha())
      CLog::Log(LOGDEBUG, "RetroPlayer[RENDER]: Amlogic set to ignore alpha");
  }
  else
  {
    if (aml_unset_reg_ignore_alpha())
      CLog::Log(LOGDEBUG, "RetroPlayer[RENDER]: Amlogic unset to ignore alpha");
  }
}
