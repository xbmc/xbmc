/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "FileItem.h"
#include "ServiceBroker.h"
#include "pvr/PVRManager.h"
#include "pvr/guilib/PVRGUIActionsPlayback.h"
#include "video/guilib/VideoPlayActionProcessor.h"

namespace PVR
{
class CGUIPVRRecordingsPlayActionProcessor
  : public KODI::VIDEO::GUILIB::CVideoPlayActionProcessorBase
{
public:
  explicit CGUIPVRRecordingsPlayActionProcessor(const std::shared_ptr<CFileItem>& item)
    : CVideoPlayActionProcessorBase(item)
  {
  }

protected:
  bool OnResumeSelected() override
  {
    m_item->SetStartOffset(STARTOFFSET_RESUME);
    Play();
    return true;
  }

  bool OnPlaySelected() override
  {
    Play();
    return true;
  }

private:
  void Play()
  {
    CServiceBroker::GetPVRManager().Get<PVR::GUI::Playback>().PlayRecording(
        *m_item, false /* no resume check */);
  }
};
} // namespace PVR
