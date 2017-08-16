#pragma once
/*
 *      Copyright (C) 2016 Team Kodi
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <memory>

#include "pvr/PVRTypes.h"

class CFileItem;
typedef std::shared_ptr<CFileItem> CFileItemPtr;

namespace PVR
{
  class CPVRItem
  {
  public:
    explicit CPVRItem(const CFileItemPtr &item) : m_item(item) {}

    CPVREpgInfoTagPtr GetEpgInfoTag() const;
    CPVRChannelPtr GetChannel() const;
    CPVRTimerInfoTagPtr GetTimerInfoTag() const;
    CPVRRecordingPtr GetRecording() const;

    bool IsRadio() const;

  private:
    CFileItemPtr m_item;
  };

} // namespace PVR
