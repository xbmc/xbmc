/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>

namespace PVR
{
  class CPVRDatabase;
  typedef std::shared_ptr<CPVRDatabase> CPVRDatabasePtr;

  class CPVREpgDatabase;
  typedef std::shared_ptr<CPVREpgDatabase> CPVREpgDatabasePtr;

  class CPVRChannel;
  typedef std::shared_ptr<CPVRChannel> CPVRChannelPtr;

  class CPVRChannelGroup;
  typedef std::shared_ptr<CPVRChannelGroup> CPVRChannelGroupPtr;

  class CPVRChannelGroupsContainer;
  typedef std::shared_ptr<CPVRChannelGroupsContainer> CPVRChannelGroupsContainerPtr;

  class CPVRClients;
  typedef std::shared_ptr<CPVRClients> CPVRClientsPtr;

  class CPVRRadioRDSInfoTag;
  typedef std::shared_ptr<CPVRRadioRDSInfoTag> CPVRRadioRDSInfoTagPtr;

  class CPVRRecording;
  typedef std::shared_ptr<CPVRRecording> CPVRRecordingPtr;

  class CPVRRecordings;
  typedef std::shared_ptr<CPVRRecordings> CPVRRecordingsPtr;

  class CPVRTimerInfoTag;
  typedef std::shared_ptr<CPVRTimerInfoTag> CPVRTimerInfoTagPtr;

  class CPVRTimerType;
  typedef std::shared_ptr<CPVRTimerType> CPVRTimerTypePtr;

  class CPVRTimers;
  typedef std::shared_ptr<CPVRTimers> CPVRTimersPtr;

  class CPVRGUIActions;
  typedef std::shared_ptr<CPVRGUIActions> CPVRGUIActionsPtr;

  class CPVREpg;
  typedef std::shared_ptr<CPVREpg> CPVREpgPtr;

  class CPVREpgInfoTag;
  typedef std::shared_ptr<CPVREpgInfoTag> CPVREpgInfoTagPtr;
  typedef std::shared_ptr<const CPVREpgInfoTag> CConstPVREpgInfoTagPtr;

} // namespace PVR

