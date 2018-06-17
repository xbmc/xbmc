/*
 *      Copyright (C) 2017 Team Kodi
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
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
