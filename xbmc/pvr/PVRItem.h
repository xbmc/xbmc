/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>

#include "pvr/PVRTypes.h"

class CFileItem;
typedef std::shared_ptr<CFileItem> CFileItemPtr;

namespace PVR
{
  class CPVRItem
  {
  public:
    explicit CPVRItem(const CFileItemPtr &item) : m_item(item.get()) {}
    explicit CPVRItem(const CFileItem *item) : m_item(item) {}

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
