#pragma once
/*
 *      Copyright (C) 2011 Pulse-Eight
 *      http://www.pulse-eight.com/
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include <vector>
#include "utils/StdString.h"
#include "client.h"

struct PVRDemoEpgEntry
{
  int         iBroadcastId;
  std::string strTitle;
  int         iChannelId;
  time_t      startTime;
  time_t      endTime;
  std::string strPlotOutline;
  std::string strPlot;
  std::string strIconPath;
  int         iGenreType;
  int         iGenreSubType;
//  time_t      firstAired;
//  int         iParentalRating;
//  int         iStarRating;
//  bool        bNotify;
//  int         iSeriesNumber;
//  int         iEpisodeNumber;
//  int         iEpisodePartNumber;
//  std::string strEpisodeName;
};

struct PVRDemoChannel
{
  bool                    bRadio;
  int                     iUniqueId;
  int                     iChannelNumber;
  int                     iEncryptionSystem;
  std::string             strChannelName;
  std::string             strIconPath;
  std::string             strStreamURL;
  std::vector<PVRDemoEpgEntry> epg;
};

struct PVRDemoChannelGroup
{
  bool             bRadio;
  int              iGroupId;
  std::string      strGroupName;
  std::vector<int> members;
};

class PVRDemoData
{
public:
  PVRDemoData(void);
  virtual ~PVRDemoData(void);

  virtual int GetChannelsAmount(void);
  virtual PVR_ERROR GetChannels(PVR_HANDLE handle, bool bRadio);
  virtual bool GetChannel(const PVR_CHANNEL &channel, PVRDemoChannel &myChannel);

  virtual int GetChannelGroupsAmount(void);
  virtual PVR_ERROR GetChannelGroups(PVR_HANDLE handle, bool bRadio);
  virtual PVR_ERROR GetChannelGroupMembers(PVR_HANDLE handle, const PVR_CHANNEL_GROUP &group);

  virtual PVR_ERROR GetEPGForChannel(PVR_HANDLE handle, const PVR_CHANNEL &channel, time_t iStart, time_t iEnd);

  virtual std::string GetSettingsFile() const;
protected:
  virtual bool LoadDemoData(void);
private:
  std::vector<PVRDemoChannelGroup> m_groups;
  std::vector<PVRDemoChannel>      m_channels;
  time_t                           m_iEpgStart;
  CStdString                       m_strDefaultIcon;
  CStdString                       m_strDefaultMovie;
};
