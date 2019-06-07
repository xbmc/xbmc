/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "pvr/PVRTypes.h"

#include <memory>

class CFileItem;

namespace PVR
{
  class CPVRItem
  {
  public:
    explicit CPVRItem(const std::shared_ptr<CFileItem>& item) : m_item(item.get()) {}
    explicit CPVRItem(const CFileItem* item) : m_item(item) {}

    CPVREpgInfoTagPtr GetEpgInfoTag() const;
    CPVREpgInfoTagPtr GetNextEpgInfoTag() const;
    CPVRChannelPtr GetChannel() const;
    CPVRTimerInfoTagPtr GetTimerInfoTag() const;
    CPVRRecordingPtr GetRecording() const;

    bool IsRadio() const;

  private:
    const CFileItem* m_item;
  };

} // namespace PVR
