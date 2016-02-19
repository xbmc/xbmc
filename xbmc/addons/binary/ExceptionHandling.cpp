/*
 *      Copyright (C) 2015-2016 Team KODI
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
 *  along with KODI; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "Util.h"
#include "addons/Addon.h"
#include "addons/AddonManager.h"
#include "addons/ScreenSaver.h"
#include "addons/Visualisation.h"
#include "cores/AudioEngine/DSPAddons/ActiveAEDSP.h"
#include "dialogs/GUIDialogOK.h"
#include "pvr/PVRManager.h"
#include "pvr/addons/PVRClient.h"
#include "pvr/addons/PVRClients.h"
#include "utils/log.h"
#include "utils/StringUtils.h"

#include "ExceptionHandling.h"

namespace ADDON
{

void CAddonExceptionHandler::Handle(const ADDON::WrongValueException& e)
{
  CLog::Log(LOGERROR,"Wrong add-on value EXCEPTION: %s", e.GetMessage());
  DestroyAddon(e.GetRelatedAddon());
}

void CAddonExceptionHandler::Handle(const XbmcCommons::Exception& e)
{
  CLog::Log(LOGERROR,"Kodi's commons EXCEPTION: %s", e.GetMessage());
}

void CAddonExceptionHandler::HandleUnknown(std::string functionName)
{
  CLog::Log(LOGERROR, "EXCEPTION: Unknown exception thrown from the call \"%s\"", functionName.c_str());
}

void CAddonExceptionHandler::Handle(const ADDON::UnimplementedException e)
{
  CLog::Log(LOGERROR,"EXCEPTION: %s",e.GetMessage());
}

void CAddonExceptionHandler::DestroyAddon(const CAddon* addon)
{
  if (addon)
  {
    std::string errorText = StringUtils::Format(g_localizeStrings.Get(13007).c_str(), addon->Name().c_str());
    CGUIDialogOK::ShowAndGetInput(13006, errorText);

    /*
     * If the addon has a running instance, grab that and retrieve used pointer
     * from related add-on system.
     */
    bool ret;
    AddonPtr addonPtr;
    switch (addon->Type())
    {
    case ADDON_PVRDLL:
    ret = PVR::g_PVRClients->GetClient(addon->ID(), addonPtr);
    break;

    case ADDON_ADSPDLL:
    ret = ActiveAE::CActiveAEDSP::GetInstance().GetAudioDSPAddon(addon->ID(), addonPtr);
    break;

    default:
    ret = false;
    break;
    }

    if (ret)
    {
      CAddonMgr::GetInstance().DisableAddon(addonPtr->ID());
    }
  }
}

}; /* namespace ADDON */
