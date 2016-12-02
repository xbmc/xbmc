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

namespace PVR
{
  class CPVRChannel;
  typedef std::shared_ptr<CPVRChannel> CPVRChannelPtr;

  class CPVRChannelGroup;
  typedef std::shared_ptr<CPVRChannelGroup> CPVRChannelGroupPtr;

  class CPVRRadioRDSInfoTag;
  typedef std::shared_ptr<CPVRRadioRDSInfoTag> CPVRRadioRDSInfoTagPtr;

  class CPVRRecording;
  typedef std::shared_ptr<CPVRRecording> CPVRRecordingPtr;

  class CPVRTimerInfoTag;
  typedef std::shared_ptr<CPVRTimerInfoTag> CPVRTimerInfoTagPtr;

  class CPVRTimerType;
  typedef std::shared_ptr<CPVRTimerType> CPVRTimerTypePtr;

} // namespace PVR

