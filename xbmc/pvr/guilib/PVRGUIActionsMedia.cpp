/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRGUIActionsMedia.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "ServiceBroker.h"
#include "Util.h"
#include "dialogs/GUIDialogBusy.h"
#include "dialogs/GUIDialogYesNo.h"
#include "filesystem/IDirectory.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/WindowIDs.h"
#include "messaging/helpers/DialogOKHelper.h"
#include "pvr/PVRItem.h"
#include "pvr/PVRManager.h"
#include "pvr/addons/PVRClient.h"
#include "pvr/addons/PVRClients.h"
#include "pvr/dialogs/GUIDialogPVRMediaTagInfo.h"
#include "pvr/dialogs/GUIDialogPVRMediaTagSettings.h"
#include "pvr/media/PVRMediaTag.h"
#include "settings/Settings.h"
#include "threads/IRunnable.h"
#include "utils/Variant.h"
#include "utils/log.h"

#include <memory>
#include <numeric>
#include <string>

using namespace PVR;
using namespace KODI::MESSAGING;

namespace
{
class AsyncMediaTagAction : private IRunnable
{
public:
  bool Execute(const CFileItem& item);

protected:
  AsyncMediaTagAction() = default;

private:
  // IRunnable implementation
  void Run() override;

  // the worker function
  virtual bool DoRun(const std::shared_ptr<CFileItem>& item) = 0;

  std::shared_ptr<CFileItem> m_item;
  bool m_bSuccess = false;
};

bool AsyncMediaTagAction::Execute(const CFileItem& item)
{
  m_item = std::make_shared<CFileItem>(item);
  CGUIDialogBusy::Wait(this, 100, false);
  return m_bSuccess;
}

void AsyncMediaTagAction::Run()
{
  m_bSuccess = DoRun(m_item);

  if (m_bSuccess)
    CServiceBroker::GetPVRManager().TriggerMediaUpdate();
}

class AsyncSetMediaTagPlayCount : public AsyncMediaTagAction
{
private:
  bool DoRun(const std::shared_ptr<CFileItem>& item) override
  {
    const std::shared_ptr<CPVRClient> client = CServiceBroker::GetPVRManager().GetClient(*item);
    if (client)
    {
      const std::shared_ptr<const CPVRMediaTag> mediaTag = item->GetPVRMediaInfoTag();
      return client->SetMediaTagPlayCount(*mediaTag, mediaTag->GetLocalPlayCount()) ==
             PVR_ERROR_NO_ERROR;
    }
    return false;
  }
};

} // unnamed namespace

bool CPVRGUIActionsMedia::ShowMediaTagInfo(const CFileItem& item) const
{
  if (!item.IsPVRMediaTag())
  {
    CLog::LogF(LOGERROR, "No media tag!");
    return false;
  }

  CGUIDialogPVRMediaTagInfo* pDlgInfo =
      CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogPVRMediaTagInfo>(
          WINDOW_DIALOG_PVR_RECORDING_INFO);
  if (!pDlgInfo)
  {
    CLog::LogF(LOGERROR, "Unable to get WINDOW_DIALOG_PVR_RECORDING_INFO!");
    return false;
  }

  pDlgInfo->SetMediaTag(item);
  pDlgInfo->Open();
  return true;
}

bool CPVRGUIActionsMedia::ShowMediaTagSettings(const std::shared_ptr<CPVRMediaTag>& mediaTag) const
{
  CGUIDialogPVRMediaTagSettings* pDlgInfo =
      CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogPVRMediaTagSettings>(
          WINDOW_DIALOG_PVR_RECORDING_SETTING);
  if (!pDlgInfo)
  {
    CLog::LogF(LOGERROR, "Unable to get WINDOW_DIALOG_PVR_RECORDING_SETTING!");
    return false;
  }

  pDlgInfo->SetMediaTag(mediaTag);
  pDlgInfo->Open();

  return pDlgInfo->IsConfirmed();
}
