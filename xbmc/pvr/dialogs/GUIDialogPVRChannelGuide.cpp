/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogPVRChannelGuide.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "ServiceBroker.h"
#include "pvr/PVRManager.h"
#include "pvr/PVRPlaybackState.h"
#include "pvr/channels/PVRChannel.h"
#include "pvr/epg/EpgInfoTag.h"

#include <memory>
#include <vector>

using namespace PVR;

CGUIDialogPVRChannelGuide::CGUIDialogPVRChannelGuide()
  : CGUIDialogPVRItemsViewBase(WINDOW_DIALOG_PVR_CHANNEL_GUIDE, "DialogPVRChannelGuide.xml")
{
}

void CGUIDialogPVRChannelGuide::Open(const std::shared_ptr<const CPVRChannel>& channel)
{
  m_channel = channel;
  CGUIDialogPVRItemsViewBase::Open();
}

void CGUIDialogPVRChannelGuide::OnInitWindow()
{
  // no user-specific channel is set; use current playing channel
  if (!m_channel)
    m_channel = CServiceBroker::GetPVRManager().PlaybackState()->GetPlayingChannel();

  if (!m_channel)
  {
    Close();
    return;
  }

  Init();

  const std::vector<std::shared_ptr<CPVREpgInfoTag>> tags = m_channel->GetEpgTags();
  for (const auto& tag : tags)
  {
    m_vecItems->Add(std::make_shared<CFileItem>(tag));
  }

  m_viewControl.SetItems(*m_vecItems);

  CGUIDialogPVRItemsViewBase::OnInitWindow();

  // select the active entry
  unsigned int iSelectedItem = 0;
  for (int iEpgPtr = 0; iEpgPtr < m_vecItems->Size(); ++iEpgPtr)
  {
    const CFileItemPtr entry = m_vecItems->Get(iEpgPtr);
    if (entry->HasEPGInfoTag() && entry->GetEPGInfoTag()->IsActive())
    {
      iSelectedItem = iEpgPtr;
      break;
    }
  }
  m_viewControl.SetSelectedItem(iSelectedItem);
}

void CGUIDialogPVRChannelGuide::OnDeinitWindow(int nextWindowID)
{
  CGUIDialogPVRItemsViewBase::OnDeinitWindow(nextWindowID);
  m_channel.reset();
}
