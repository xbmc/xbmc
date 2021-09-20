/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RenderCapture.h"

#include "ServiceBroker.h"
#include "cores/IPlayer.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"

bool CRenderCapture::UseOcclusionQuery()
{
  if (m_flags & CAPTUREFLAG_IMMEDIATELY)
    return false;
  else if (CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoCaptureUseOcclusionQuery == 0)
    return false;
  else
    return true;
}
