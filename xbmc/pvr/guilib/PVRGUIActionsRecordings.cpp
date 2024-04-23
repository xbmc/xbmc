/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRGUIActionsRecordings.h"

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
#include "pvr/dialogs/GUIDialogPVRRecordingInfo.h"
#include "pvr/dialogs/GUIDialogPVRRecordingSettings.h"
#include "pvr/recordings/PVRRecording.h"
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
class AsyncRecordingAction : private IRunnable
{
public:
  bool Execute(const CFileItem& item);

protected:
  AsyncRecordingAction() = default;

private:
  // IRunnable implementation
  void Run() override;

  // the worker function
  virtual bool DoRun(const std::shared_ptr<CFileItem>& item) = 0;

  std::shared_ptr<CFileItem> m_item;
  bool m_bSuccess = false;
};

bool AsyncRecordingAction::Execute(const CFileItem& item)
{
  m_item = std::make_shared<CFileItem>(item);
  CGUIDialogBusy::Wait(this, 100, false);
  return m_bSuccess;
}

void AsyncRecordingAction::Run()
{
  m_bSuccess = DoRun(m_item);

  if (m_bSuccess)
    CServiceBroker::GetPVRManager().TriggerRecordingsUpdate();
}

class AsyncRenameRecording : public AsyncRecordingAction
{
public:
  explicit AsyncRenameRecording(const std::string& strNewName) : m_strNewName(strNewName) {}

private:
  bool DoRun(const std::shared_ptr<CFileItem>& item) override
  {
    if (item->IsUsablePVRRecording())
    {
      return item->GetPVRRecordingInfoTag()->Rename(m_strNewName);
    }
    else
    {
      CLog::LogF(LOGERROR, "Cannot rename item '{}': no valid recording tag", item->GetPath());
      return false;
    }
  }
  std::string m_strNewName;
};

class AsyncDeleteRecording : public AsyncRecordingAction
{
public:
  explicit AsyncDeleteRecording(bool bWatchedOnly = false) : m_bWatchedOnly(bWatchedOnly) {}

private:
  bool DoRun(const std::shared_ptr<CFileItem>& item) override
  {
    CFileItemList items;
    if (item->m_bIsFolder)
    {
      CUtil::GetRecursiveListing(item->GetPath(), items, "", XFILE::DIR_FLAG_NO_FILE_INFO);
    }
    else
    {
      items.Add(item);
    }

    return std::accumulate(
        items.cbegin(), items.cend(), true, [this](bool success, const auto& itemToDelete) {
          return (itemToDelete->IsPVRRecording() &&
                  (!m_bWatchedOnly || itemToDelete->GetPVRRecordingInfoTag()->GetPlayCount() > 0) &&
                  !itemToDelete->GetPVRRecordingInfoTag()->Delete())
                     ? false
                     : success;
        });
  }
  bool m_bWatchedOnly = false;
};

class AsyncEmptyRecordingsTrash : public AsyncRecordingAction
{
private:
  bool DoRun(const std::shared_ptr<CFileItem>& item) override
  {
    return CServiceBroker::GetPVRManager().Clients()->DeleteAllRecordingsFromTrash() ==
           PVR_ERROR_NO_ERROR;
  }
};

class AsyncUndeleteRecording : public AsyncRecordingAction
{
private:
  bool DoRun(const std::shared_ptr<CFileItem>& item) override
  {
    if (item->IsDeletedPVRRecording())
    {
      return item->GetPVRRecordingInfoTag()->Undelete();
    }
    else
    {
      CLog::LogF(LOGERROR, "Cannot undelete item '{}': no valid recording tag", item->GetPath());
      return false;
    }
  }
};

class AsyncSetRecordingPlayCount : public AsyncRecordingAction
{
private:
  bool DoRun(const std::shared_ptr<CFileItem>& item) override
  {
    const std::shared_ptr<CPVRClient> client = CServiceBroker::GetPVRManager().GetClient(*item);
    if (client)
    {
      const std::shared_ptr<const CPVRRecording> recording = item->GetPVRRecordingInfoTag();
      return client->SetRecordingPlayCount(*recording, recording->GetLocalPlayCount()) ==
             PVR_ERROR_NO_ERROR;
    }
    return false;
  }
};

class AsyncSetRecordingLifetime : public AsyncRecordingAction
{
private:
  bool DoRun(const std::shared_ptr<CFileItem>& item) override
  {
    const std::shared_ptr<CPVRClient> client = CServiceBroker::GetPVRManager().GetClient(*item);
    if (client)
      return client->SetRecordingLifetime(*item->GetPVRRecordingInfoTag()) == PVR_ERROR_NO_ERROR;
    return false;
  }
};

} // unnamed namespace

bool CPVRGUIActionsRecordings::ShowRecordingInfo(const CFileItem& item) const
{
  if (!item.IsPVRRecording())
  {
    CLog::LogF(LOGERROR, "No recording!");
    return false;
  }

  CGUIDialogPVRRecordingInfo* pDlgInfo =
      CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogPVRRecordingInfo>(
          WINDOW_DIALOG_PVR_RECORDING_INFO);
  if (!pDlgInfo)
  {
    CLog::LogF(LOGERROR, "Unable to get WINDOW_DIALOG_PVR_RECORDING_INFO!");
    return false;
  }

  pDlgInfo->SetRecording(item);
  pDlgInfo->Open();
  return true;
}

bool CPVRGUIActionsRecordings::EditRecording(const CFileItem& item) const
{
  const std::shared_ptr<CPVRRecording> recording = CPVRItem(item).GetRecording();
  if (!recording)
  {
    CLog::LogF(LOGERROR, "No recording!");
    return false;
  }

  std::shared_ptr<CPVRRecording> origRecording(new CPVRRecording);
  origRecording->Update(*recording,
                        *CServiceBroker::GetPVRManager().GetClient(recording->ClientID()));

  if (!ShowRecordingSettings(recording))
    return false;

  if (origRecording->m_strTitle != recording->m_strTitle)
  {
    if (!AsyncRenameRecording(recording->m_strTitle).Execute(item))
      CLog::LogF(LOGERROR, "Renaming recording failed!");
  }

  if (origRecording->GetLocalPlayCount() != recording->GetLocalPlayCount())
  {
    if (!AsyncSetRecordingPlayCount().Execute(item))
      CLog::LogF(LOGERROR, "Setting recording playcount failed!");
  }

  if (origRecording->LifeTime() != recording->LifeTime())
  {
    if (!AsyncSetRecordingLifetime().Execute(item))
      CLog::LogF(LOGERROR, "Setting recording lifetime failed!");
  }

  return true;
}

bool CPVRGUIActionsRecordings::CanEditRecording(const CFileItem& item) const
{
  return CGUIDialogPVRRecordingSettings::CanEditRecording(item);
}

bool CPVRGUIActionsRecordings::DeleteRecording(const CFileItem& item) const
{
  if ((!item.IsPVRRecording() && !item.m_bIsFolder) || item.IsParentFolder())
    return false;

  if (!ConfirmDeleteRecording(item))
    return false;

  if (!AsyncDeleteRecording().Execute(item))
  {
    HELPERS::ShowOKDialogText(
        CVariant{257},
        CVariant{
            19111}); // "Error", "PVR backend error. Check the log for more information about this message."
    return false;
  }

  return true;
}

bool CPVRGUIActionsRecordings::ConfirmDeleteRecording(const CFileItem& item) const
{
  return CGUIDialogYesNo::ShowAndGetInput(
      CVariant{122}, // "Confirm delete"
      item.m_bIsFolder
          ? CVariant{19113} // "Delete all recordings in this folder?"
          : item.GetPVRRecordingInfoTag()->IsDeleted()
                ? CVariant{19294}
                // "Remove this deleted recording from trash? This operation cannot be reverted."
                : CVariant{19112}, // "Delete this recording?"
      CVariant{""}, CVariant{item.GetLabel()});
}

bool CPVRGUIActionsRecordings::DeleteWatchedRecordings(const CFileItem& item) const
{
  if (!item.m_bIsFolder || item.IsParentFolder())
    return false;

  if (!ConfirmDeleteWatchedRecordings(item))
    return false;

  if (!AsyncDeleteRecording(true).Execute(item))
  {
    HELPERS::ShowOKDialogText(
        CVariant{257},
        CVariant{
            19111}); // "Error", "PVR backend error. Check the log for more information about this message."
    return false;
  }

  return true;
}

bool CPVRGUIActionsRecordings::ConfirmDeleteWatchedRecordings(const CFileItem& item) const
{
  return CGUIDialogYesNo::ShowAndGetInput(
      CVariant{122}, // "Confirm delete"
      CVariant{19328}, // "Delete all watched recordings in this folder?"
      CVariant{""}, CVariant{item.GetLabel()});
}

bool CPVRGUIActionsRecordings::DeleteAllRecordingsFromTrash() const
{
  if (!ConfirmDeleteAllRecordingsFromTrash())
    return false;

  if (!AsyncEmptyRecordingsTrash().Execute({}))
    return false;

  return true;
}

bool CPVRGUIActionsRecordings::ConfirmDeleteAllRecordingsFromTrash() const
{
  return CGUIDialogYesNo::ShowAndGetInput(
      CVariant{19292}, // "Delete all permanently"
      CVariant{
          19293}); // "Remove all deleted recordings from trash? This operation cannot be reverted."
}

bool CPVRGUIActionsRecordings::UndeleteRecording(const CFileItem& item) const
{
  if (!item.IsDeletedPVRRecording())
    return false;

  if (!AsyncUndeleteRecording().Execute(item))
  {
    HELPERS::ShowOKDialogText(
        CVariant{257},
        CVariant{
            19111}); // "Error", "PVR backend error. Check the log for more information about this message."
    return false;
  }

  return true;
}

bool CPVRGUIActionsRecordings::ShowRecordingSettings(
    const std::shared_ptr<CPVRRecording>& recording) const
{
  CGUIDialogPVRRecordingSettings* pDlgInfo =
      CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogPVRRecordingSettings>(
          WINDOW_DIALOG_PVR_RECORDING_SETTING);
  if (!pDlgInfo)
  {
    CLog::LogF(LOGERROR, "Unable to get WINDOW_DIALOG_PVR_RECORDING_SETTING!");
    return false;
  }

  pDlgInfo->SetRecording(recording);
  pDlgInfo->Open();

  return pDlgInfo->IsConfirmed();
}
