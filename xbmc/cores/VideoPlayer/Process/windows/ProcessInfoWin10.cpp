/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ProcessInfoWin10.h"

#include "ServiceBroker.h"
#include "cores/VideoPlayer/Process/ProcessInfo.h"
#include "cores/VideoPlayer/VideoRenderers/windows/DolbyVisionEffect.h"
#include "cores/VideoPlayer/VideoRenderers/windows/RendererShaders.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"

#include <set>

using namespace VIDEOPLAYER;

CProcessInfo* CProcessInfoWin10::Create()
{
  return new CProcessInfoWin10();
}

void CProcessInfoWin10::Register()
{
  CProcessInfo::RegisterProcessControl("win10", CProcessInfoWin10::Create);
}

EINTERLACEMETHOD CProcessInfoWin10::GetFallbackDeintMethod()
{
  return EINTERLACEMETHOD::VS_INTERLACEMETHOD_AUTO;
}

std::vector<AVPixelFormat> CProcessInfoWin10::GetRenderFormats()
{
  return CRendererShaders::GetRenderFormats();
}

bool CProcessInfoWin10::WantsDolbyVisionRpu()
{
  if (!CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
          "videoplayer.usemfdolbyvision"))
    return false;

  static const bool effectAvailable = CDolbyVisionEffect::IsAvailable();
  return effectAvailable;
}
