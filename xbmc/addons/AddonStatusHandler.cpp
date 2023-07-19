/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */
#include "AddonStatusHandler.h"

#include "ServiceBroker.h"
#include "addons/AddonManager.h"
#include "addons/IAddonManagerCallback.h"
#include "addons/addoninfo/AddonInfo.h"
#include "addons/gui/GUIDialogAddonSettings.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "dialogs/GUIDialogYesNo.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "messaging/ApplicationMessenger.h"
#include "messaging/helpers/DialogOKHelper.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"

#include <mutex>
#include <utility>

using namespace KODI::MESSAGING;

namespace ADDON
{

/**********************************************************
 * CAddonStatusHandler - AddOn Status Report Class
 *
 * Used to inform the user about occurred errors and
 * changes inside Add-on's, and ask him what to do.
 *
 */

CCriticalSection CAddonStatusHandler::m_critSection;

CAddonStatusHandler::CAddonStatusHandler(const std::string& addonID,
                                         AddonInstanceId instanceId,
                                         ADDON_STATUS status,
                                         bool sameThread)
  : CThread(("AddonStatus " + std::to_string(instanceId) + "@" + addonID).c_str()),
    m_instanceId(instanceId)
{
  //! @todo The status handled CAddonStatusHandler by is related to the class, not the instance
  //! having CAddonMgr construct an instance makes no sense
  if (!CServiceBroker::GetAddonMgr().GetAddon(addonID, m_addon, OnlyEnabled::CHOICE_YES))
    return;

  CLog::Log(LOGINFO,
            "Called Add-on status handler for '{}' of clientName:{}, clientID:{}, instanceID:{} "
            "(same Thread={})",
            status, m_addon->Name(), m_addon->ID(), m_instanceId, sameThread ? "yes" : "no");

  m_status = status;

  if (sameThread)
  {
    Process();
  }
  else
  {
    Create(true);
  }
}

CAddonStatusHandler::~CAddonStatusHandler()
{
  StopThread();
}

void CAddonStatusHandler::OnStartup()
{
  SetPriority(ThreadPriority::LOWEST);
}

void CAddonStatusHandler::OnExit()
{
}

void CAddonStatusHandler::Process()
{
  std::unique_lock<CCriticalSection> lock(m_critSection);

  std::string heading = StringUtils::Format(
      "{}: {}", CAddonInfo::TranslateType(m_addon->Type(), true), m_addon->Name());

  /* Request to restart the AddOn and data structures need updated */
  if (m_status == ADDON_STATUS_NEED_RESTART)
  {
    HELPERS::ShowOKDialogLines(CVariant{heading}, CVariant{24074});
    CServiceBroker::GetAddonMgr()
        .GetCallbackForType(m_addon->Type())
        ->RequestRestart(m_addon->ID(), m_instanceId, true);
  }
  /* Some required settings are missing/invalid */
  else if (m_status == ADDON_STATUS_NEED_SETTINGS)
  {
    CGUIDialogYesNo* pDialogYesNo = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogYesNo>(WINDOW_DIALOG_YES_NO);
    if (!pDialogYesNo) return;

    pDialogYesNo->SetHeading(CVariant{heading});
    pDialogYesNo->SetLine(1, CVariant{24070});
    pDialogYesNo->SetLine(2, CVariant{24072});
    pDialogYesNo->Open();

    if (!pDialogYesNo->IsConfirmed()) return;

    if (!m_addon->HasSettings(m_instanceId))
      return;

    if (CGUIDialogAddonSettings::ShowForAddon(m_addon))
    {
      //! @todo Doesn't dialogaddonsettings save these automatically? It should do this.
      m_addon->SaveSettings(m_instanceId);
      CServiceBroker::GetAddonMgr()
          .GetCallbackForType(m_addon->Type())
          ->RequestRestart(m_addon->ID(), m_instanceId, true);
    }
  }
}


} /*namespace ADDON*/

