/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>

class CFileItem;

namespace PVR
{
class CPVRChannel;
class CPVREpgInfoTag;
class CPVRRecording;
class CPVRTimerInfoTag;

class CPVRItem
{
public:
  explicit CPVRItem(const std::shared_ptr<CFileItem>& item) : m_item(item.get()) {}
  explicit CPVRItem(const CFileItem* item) : m_item(item) {}
  explicit CPVRItem(const CFileItem& item) : m_item(&item) {}

  std::shared_ptr<CPVREpgInfoTag> GetEpgInfoTag() const;
  std::shared_ptr<CPVREpgInfoTag> GetNextEpgInfoTag() const;
  std::shared_ptr<CPVRChannel> GetChannel() const;
  std::shared_ptr<CPVRTimerInfoTag> GetTimerInfoTag() const;
  std::shared_ptr<CPVRRecording> GetRecording() const;

  bool IsRadio() const;

private:
  const CFileItem* m_item;
};

} // namespace PVR
