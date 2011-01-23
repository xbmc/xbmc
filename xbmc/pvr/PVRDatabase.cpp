/*
 *      Copyright (C) 2005-2010 Team XBMC
 *      http://www.xbmc.org
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

#include "PVRDatabase.h"
#include "AdvancedSettings.h"
#include "settings/VideoSettings.h"
#include "utils/log.h"

#include "PVREpgs.h"
#include "PVREpgInfoTag.h"
#include "PVRChannelGroupsContainer.h"
#include "PVRChannelGroupInternal.h"

using namespace std;
using namespace dbiplus;

CPVRDatabase::CPVRDatabase(void)
{
  lastScanTime.SetValid(false);
}

CPVRDatabase::~CPVRDatabase(void)
{
}

bool CPVRDatabase::Open()
{
  return CDatabase::Open(g_advancedSettings.m_databaseTV);
}

bool CPVRDatabase::CreateTables()
{
  bool bReturn = false;

  try
  {
    CDatabase::CreateTables();

    CLog::Log(LOGINFO, "PVRDB - %s - creating tables", __FUNCTION__);

    CLog::Log(LOGDEBUG, "PVRDB - %s - creating table 'clients'", __FUNCTION__);
    m_pDS->exec(
        "CREATE TABLE clients ("
          "idClient integer primary key, "
          "sName    text, "
          "sUid     text"
        ");"
    );

    CLog::Log(LOGDEBUG, "PVRDB - %s - creating table 'channels'", __FUNCTION__);
    m_pDS->exec(
        "CREATE TABLE channels ("
          "idChannel            integer primary key, "
          "iUniqueId            integer, "
          "iChannelNumber       integer, "
          "idGroup              integer, " // TODO use mapping table
          "bIsRadio             bool, "
          "bIsHidden            bool, "
          "sIconPath            text, "
          "sChannelName         text, "
          "bIsVirtual           bool, "
          "bEPGEnabled          bool, "
          "sEPGScraper          text, "
          "iLastWatched         integer,"

          // TODO use mapping table
          "iClientId            integer, "
          "iClientChannelNumber integer, "
          "sInputFormat         text, "
          "sStreamURL           text, "
          "iEncryptionSystem    integer"
        ");"
    );
    m_pDS->exec("CREATE UNIQUE INDEX idx_channels_iChannelNumber_bIsRadio on channels(iChannelNumber, bIsRadio);");
    m_pDS->exec("CREATE INDEX idx_channels_iClientId on channels(iClientId);");
    m_pDS->exec("CREATE INDEX idx_channels_iChannelNumber on channels(iChannelNumber);");
    m_pDS->exec("CREATE INDEX idx_channels_iLastWatched on channels(iLastWatched);");
    m_pDS->exec("CREATE INDEX idx_channels_bIsRadio on channels(bIsRadio);");
    m_pDS->exec("CREATE INDEX idx_channels_bIsHidden on channels(bIsHidden);");

    // TODO use a mapping table so multiple backends per channel can be implemented
    //    CLog::Log(LOGDEBUG, "PVRDB - %s - creating table 'map_channels_clients'", __FUNCTION__);
    //    m_pDS->exec(
    //        "CREATE TABLE map_channels_clients ("
    //          "idChannel             integer primary key, "
    //          "idClient              integer, "
    //          "iClientChannelNumber  integer,"
    //          "sInputFormat          string,"
    //          "sStreamURL            string,"
    //          "iEncryptionSystem     integer"
    //        ");"
    //    );
    //    m_pDS->exec("CREATE UNIQUE INDEX idx_idChannel_idClient on map_channels_clients(idChannel, idClient);");

    CLog::Log(LOGDEBUG, "PVRDB - %s - creating view 'vw_last_watched'", __FUNCTION__);
    m_pDS->exec(
        "CREATE VIEW vw_last_watched "
        "AS SELECT idChannel, iChannelNumber, sChannelName "
        "FROM Channels "
        "ORDER BY iLastWatched DESC;"
    );

    CLog::Log(LOGDEBUG, "PVRDB - %s - creating table 'channelgroups'", __FUNCTION__);
    m_pDS->exec(
        "CREATE TABLE channelgroups ("
          "idGroup    integer primary key,"
          "bIsRadio   bool, "
          "sName      text,"
          "iSortOrder integer"
        ");"
    );
    m_pDS->exec("CREATE INDEX idx_channelgroups_bIsRadio on channelgroups(bIsRadio);");

    // TODO use a mapping table so multiple groups per channel can be implemented
    // replaces idGroup in the channels table
    //    CLog::Log(LOGDEBUG, "PVRDB - %s - creating table 'map_channelgroups_channels'", __FUNCTION__);
    //    m_pDS->exec(
    //        "CREATE TABLE map_channelgroups_channels ("
    //          "idChannel integer, "
    //          "idGroup   integer"
    //        ");"
    //    );
    //    m_pDS->exec("CREATE UNIQUE INDEX idx_idChannel_idGroup on map_channelgroups_channels(idChannel, idGroup);");

    CLog::Log(LOGDEBUG, "PVRDB - %s - creating table 'channelsettings'", __FUNCTION__);
    m_pDS->exec(
        "CREATE TABLE channelsettings ("
          "idChannel            integer primary key, "
          "iInterlaceMethod     integer, "
          "iViewMode            integer, "
          "fCustomZoomAmount    float, "
          "fPixelRatio          float, "
          "iAudioStream         integer, "
          "iSubtitleStream      integer,"
          "fSubtitleDelay       float, "
          "bSubtitles           bool, "
          "fBrightness          float, "
          "fContrast            float, "
          "fGamma               float,"
          "fVolumeAmplification float, "
          "fAudioDelay          float, "
          "bOutputToAllSpeakers bool, "
          "bCrop                bool, "
          "iCropLeft            integer, "
          "iCropRight           integer, "
          "iCropTop             integer, "
          "iCropBottom          integer, "
          "fSharpness           float, "
          "fNoiseReduction      float"
        ");"
    );

    CLog::Log(LOGDEBUG, "PVRDB - %s - creating table 'epg'", __FUNCTION__);
    m_pDS->exec(
        "CREATE TABLE epg ("
          "idBroadcast     integer primary key, "
          "iBroadcastUid   integer, "
          "idChannel       integer, "
          "sTitle          text, "
          "sPlotOutline    text, "
          "sPlot           text, "
          "iStartTime      integer, "
          "iEndTime        integer, "
          "iGenreType      integer, "
          "iGenreSubType   integer, "
          "iFirstAired     integer, "
          "iParentalRating integer, "
          "iStarRating     integer, "
          "bNotify         bool, "
          "sSeriesId       text, "
          "sEpisodeId      text, "
          "sEpisodePart    text, "
          "sEpisodeName    text"
        ");"
    );
    m_pDS->exec("CREATE UNIQUE INDEX idx_epg_idChannel_iStartTime on epg(idChannel, iStartTime desc);");
    m_pDS->exec("CREATE INDEX idx_epg_iBroadcastUid on epg(iBroadcastUid);");
    m_pDS->exec("CREATE INDEX idx_epg_idChannel on epg(idChannel);");
    m_pDS->exec("CREATE INDEX idx_epg_iStartTime on epg(iStartTime);");
    m_pDS->exec("CREATE INDEX idx_epg_iEndTime on epg(iEndTime);");

    // TODO keep separate value per epg table collection
    CLog::Log(LOGDEBUG, "PVRDB - %s - creating table 'lastepgscan'", __FUNCTION__);
    m_pDS->exec("CREATE TABLE lastepgscan ("
          "idEpg integer primary key, "
          "iLastScan integer"
        ")"
    );

    bReturn = true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "PVRDB - %s - unable to create TV tables:%i",
        __FUNCTION__, (int)GetLastError());
    bReturn = false;
  }

  return bReturn;
}

bool CPVRDatabase::UpdateOldVersion(int iVersion)
{
  if (iVersion < 7)
  {
    /* TODO since sqlite doesn't support all ALTER TABLE statements, we have to rename things by code. not supported at the moment */
    CLog::Log(LOGERROR, "%s - Incompatible database version!", __FUNCTION__);
    return false;
  }

  if (iVersion == 7)
  {
    m_pDS->exec("DROP INDEX idx_epg_iBroadcastUid;");
    m_pDS->exec("CREATE INDEX idx_epg_iBroadcastUid on epg(iBroadcastUid);");
  }

  return true;
}

/********** Channel methods **********/

bool CPVRDatabase::EraseChannels()
{
  CLog::Log(LOGDEBUG, "PVRDB - %s - deleting all channels from the database", __FUNCTION__);

  return DeleteValues("channels");
}

bool CPVRDatabase::EraseClientChannels(long iClientId)
{
  /* invalid client Id */
  if (iClientId <= 0)
  {
    CLog::Log(LOGERROR, "PVRDB - %s - invalid client id: %li",
        __FUNCTION__, iClientId);
    return false;
  }

  CLog::Log(LOGDEBUG, "PVRDB - %s - deleting all channels from client '%li' from the database",
      __FUNCTION__, iClientId);

  CStdString strWhereClause = FormatSQL("iClientId = %u", iClientId);
  return DeleteValues("channels", strWhereClause);
}

long CPVRDatabase::UpdateChannel(const CPVRChannel &channel, bool bQueueWrite /* = false */)
{
  long iReturn = -1;

  /* invalid channel */
  if (channel.UniqueID() <= 0)
  {
    CLog::Log(LOGERROR, "PVRDB - %s - invalid channel uid: %d",
        __FUNCTION__, channel.UniqueID());
    return iReturn;
  }

  CStdString strQuery;

  if (channel.ChannelID() <= 0)
  {
    /* new channel */
    strQuery = FormatSQL("INSERT INTO channels ("
        "iUniqueId, iChannelNumber, idGroup, bIsRadio, bIsHidden, "
        "sIconPath, sChannelName, bIsVirtual, bEPGEnabled, sEPGScraper, iClientId, "
        "iClientChannelNumber, sInputFormat, sStreamURL, iEncryptionSystem) "
        "VALUES (%i, %i, %i, %i, %i, '%s', '%s', %i, %i, '%s', %i, %i, '%s', '%s', %i);",
        channel.UniqueID(), channel.ChannelNumber(), channel.GroupID(), (channel.IsRadio() ? 1 :0), (channel.IsHidden() ? 1 : 0),
        channel.IconPath().c_str(), channel.ChannelName().c_str(), (channel.IsVirtual() ? 1 : 0), (channel.EPGEnabled() ? 1 : 0), channel.EPGScraper().c_str(), channel.ClientID(),
        channel.ClientChannelNumber(), channel.InputFormat().c_str(), channel.StreamURL().c_str(), channel.EncryptionSystem());
  }
  else
  {
    /* update channel */
    strQuery = FormatSQL("REPLACE INTO channels ("
        "iUniqueId, iChannelNumber, idGroup, bIsRadio, bIsHidden, "
        "sIconPath, sChannelName, bIsVirtual, bEPGEnabled, sEPGScraper, iClientId, "
        "iClientChannelNumber, sInputFormat, sStreamURL, iEncryptionSystem, idChannel) "
        "VALUES (%i, %i, %i, %i, %i, '%s', '%s', %i, %i, '%s', %i, %i, '%s', '%s', %i, %i);",
        channel.UniqueID(), channel.ChannelNumber(), channel.GroupID(), (channel.IsRadio() ? 1 :0), (channel.IsHidden() ? 1 : 0),
        channel.IconPath().c_str(), channel.ChannelName().c_str(), (channel.IsVirtual() ? 1 : 0), (channel.EPGEnabled() ? 1 : 0), channel.EPGScraper().c_str(), channel.ClientID(),
        channel.ClientChannelNumber(), channel.InputFormat().c_str(), channel.StreamURL().c_str(), channel.EncryptionSystem(), channel.ChannelID());
  }

  if (bQueueWrite)
  {
    QueueInsertQuery(strQuery);
    iReturn = 0;
  }
  else if (ExecuteQuery(strQuery))
  {
    iReturn = (channel.ChannelID() <= 0) ? (long) m_pDS->lastinsertid() : channel.ChannelID();
  }

  return iReturn;
}

bool CPVRDatabase::RemoveChannel(const CPVRChannel &channel)
{
  /* invalid channel */
  if (channel.ChannelID() <= 0)
  {
    CLog::Log(LOGERROR, "PVRDB - %s - invalid channel id: %li",
        __FUNCTION__, channel.ChannelID());
    return false;
  }

  CStdString strWhereClause = FormatSQL("idChannel = %u", channel.ChannelID());
  return DeleteValues("channels", strWhereClause);
}

int CPVRDatabase::GetChannels(CPVRChannelGroupInternal &results, bool bIsRadio)
{
  int iReturn = -1;

  CStdString strQuery = FormatSQL("SELECT * FROM channels WHERE bIsRadio = %u ORDER BY iChannelNumber;", bIsRadio);
  int iNumRows = ResultQuery(strQuery);

  if (iNumRows > 0)
  {
    try
    {
      while (!m_pDS->eof())
      {
        CPVRChannel *channel = new CPVRChannel();

        channel->m_iChannelId              = m_pDS->fv("idChannel").get_asInt();
        channel->m_iUniqueId               = m_pDS->fv("iUniqueId").get_asInt();
        channel->m_iChannelNumber          = m_pDS->fv("iChannelNumber").get_asInt();
        channel->m_iChannelGroupId         = m_pDS->fv("idGroup").get_asInt();
        channel->m_bIsRadio                = m_pDS->fv("bIsRadio").get_asBool();
        channel->m_bIsHidden               = m_pDS->fv("bIsHidden").get_asBool();
        channel->m_strIconPath             = m_pDS->fv("sIconPath").get_asString();
        channel->m_strChannelName          = m_pDS->fv("sChannelName").get_asString();
        channel->m_bIsVirtual              = m_pDS->fv("bIsVirtual").get_asBool();
        channel->m_bEPGEnabled             = m_pDS->fv("bEPGEnabled").get_asBool();
        channel->m_strEPGScraper           = m_pDS->fv("sEPGScraper").get_asString();
        channel->m_iClientId               = m_pDS->fv("iClientId").get_asInt();
        channel->m_iClientChannelNumber    = m_pDS->fv("iClientChannelNumber").get_asInt();
        channel->m_strInputFormat          = m_pDS->fv("sInputFormat").get_asString();
        channel->m_strStreamURL            = m_pDS->fv("sStreamURL").get_asString();
        channel->m_iClientEncryptionSystem = m_pDS->fv("iEncryptionSystem").get_asInt();

        channel->UpdatePath();

        CLog::Log(LOGDEBUG, "PVRDB - %s - channel '%s' loaded from the database",
            __FUNCTION__, channel->m_strChannelName.c_str());
        results.push_back(channel);
        m_pDS->next();
        ++iReturn;
      }
    }
    catch (...)
    {
      CLog::Log(LOGERROR, "PVRDB - %s - couldn't load channels from the database", __FUNCTION__);
    }
  }

  m_pDS->close();
  return iReturn;
}

int CPVRDatabase::GetChannelCount(bool bRadio, bool bHidden /* = false */)
{
  int iReturn = -1;
  CStdString strQuery = FormatSQL("SELECT COUNT(1) FROM channels WHERE bIsRadio = %u AND bIsHidden = %u;",
      (bRadio ? 1 : 0), (bHidden ? 1 : 0));

  if (ResultQuery(strQuery))
  {
    iReturn = m_pDS->fv(0).get_asInt();
  }
  m_pDS->close();

  return iReturn;
}

int CPVRDatabase::GetLastChannel()
{
  CStdString strValue = GetSingleValue("vw_last_watched", "idChannel");

  if (strValue.IsEmpty())
    return -1;

  return atoi(strValue.c_str());
}

bool CPVRDatabase::UpdateLastChannel(const CPVRChannel &channel)
{
  /* invalid channel */
  if (channel.ChannelID() <= 0)
  {
    CLog::Log(LOGERROR, "PVRDB - %s - invalid channel id: %li",
        __FUNCTION__, channel.ChannelID());
    return false;
  }

  time_t tNow;
  CDateTime::GetCurrentDateTime().GetAsTime(tNow);
  CStdString strQuery = FormatSQL("UPDATE channels SET iLastWatched = %u WHERE idChannel = %u",
      tNow, channel.ChannelID());

  return ExecuteQuery(strQuery);
}


bool CPVRDatabase::EraseChannelSettings()
{
  CLog::Log(LOGDEBUG, "PVRDB - %s - deleting all channel settings from the database", __FUNCTION__);
  return DeleteValues("channelsettings");
}

bool CPVRDatabase::GetChannelSettings(const CPVRChannel &channel, CVideoSettings &settings)
{
  bool bReturn = false;

  /* invalid channel */
  if (channel.ChannelID() <= 0)
  {
    CLog::Log(LOGERROR, "PVRDB - %s - invalid channel id: %li",
        __FUNCTION__, channel.ChannelID());
    return bReturn;
  }

  CStdString strQuery = FormatSQL("SELECT * FROM channelsettings WHERE idChannel = %u;", channel.ChannelID());

  if (ResultQuery(strQuery))
  {
    settings.m_AudioDelay           = m_pDS->fv("fAudioDelay").get_asFloat();
    settings.m_AudioStream          = m_pDS->fv("iAudioStream").get_asInt();
    settings.m_Brightness           = m_pDS->fv("fBrightness").get_asFloat();
    settings.m_Contrast             = m_pDS->fv("fContrast").get_asFloat();
    settings.m_CustomPixelRatio     = m_pDS->fv("fPixelRatio").get_asFloat();
    settings.m_NoiseReduction       = m_pDS->fv("fNoiseReduction").get_asFloat();
    settings.m_Sharpness            = m_pDS->fv("fSharpness").get_asFloat();
    settings.m_CustomZoomAmount     = m_pDS->fv("fCustomZoomAmount").get_asFloat();
    settings.m_Gamma                = m_pDS->fv("fGamma").get_asFloat();
    settings.m_SubtitleDelay        = m_pDS->fv("fSubtitleDelay").get_asFloat();
    settings.m_SubtitleOn           = m_pDS->fv("bSubtitles").get_asBool();
    settings.m_SubtitleStream       = m_pDS->fv("iSubtitleStream").get_asInt();
    settings.m_ViewMode             = m_pDS->fv("iViewMode").get_asInt();
    settings.m_Crop                 = m_pDS->fv("bCrop").get_asBool();
    settings.m_CropLeft             = m_pDS->fv("iCropLeft").get_asInt();
    settings.m_CropRight            = m_pDS->fv("iCropRight").get_asInt();
    settings.m_CropTop              = m_pDS->fv("iCropTop").get_asInt();
    settings.m_CropBottom           = m_pDS->fv("iCropBottom").get_asInt();
    settings.m_InterlaceMethod      = (EINTERLACEMETHOD)m_pDS->fv("iInterlaceMethod").get_asInt();
    settings.m_VolumeAmplification  = m_pDS->fv("fVolumeAmplification").get_asFloat();
    settings.m_OutputToAllSpeakers  = m_pDS->fv("bOutputToAllSpeakers").get_asBool();
    settings.m_SubtitleCached       = false;

    bReturn = true;
  }
  m_pDS->close();

  return bReturn;
}

bool CPVRDatabase::SetChannelSettings(const CPVRChannel &channel, const CVideoSettings &settings)
{
  /* invalid channel */
  if (channel.ChannelID() <= 0)
  {
    CLog::Log(LOGERROR, "PVRDB - %s - invalid channel id: %li",
        __FUNCTION__, channel.ChannelID());
    return false;
  }

  CStdString strQuery = FormatSQL(
      "REPLACE INTO channelsettings "
        "(idChannel, iInterlaceMethod, iViewMode, fCustomZoomAmount, fPixelRatio, iAudioStream, iSubtitleStream, fSubtitleDelay, "
         "bSubtitles, fBrightness, fContrast, fGamma, fVolumeAmplification, fAudioDelay, bOutputToAllSpeakers, bCrop, iCropLeft, "
         "iCropRight, iCropTop, iCropBottom, fSharpness, fNoiseReduction) VALUES "
         "(%i, %i, %i, %f, %f, %i, %i, %f, %i, %f, %f, %f, %f, %f, %i, %i, %i, %i, %i, %i, %f, %f);",
       channel.ChannelID(), settings.m_InterlaceMethod, settings.m_ViewMode, settings.m_CustomZoomAmount, settings.m_CustomPixelRatio,
       settings.m_AudioStream, settings.m_SubtitleStream, settings.m_SubtitleDelay, settings.m_SubtitleOn,
       settings.m_Brightness, settings.m_Contrast, settings.m_Gamma, settings.m_VolumeAmplification, settings.m_AudioDelay,
       settings.m_OutputToAllSpeakers, settings.m_Crop, settings.m_CropLeft, settings.m_CropRight, settings.m_CropTop,
       settings.m_CropBottom, settings.m_Sharpness, settings.m_NoiseReduction);

  return ExecuteQuery(strQuery);
}

/********** Channel group methods **********/

bool CPVRDatabase::EraseChannelGroups(bool bRadio /* = false */)
{
  CLog::Log(LOGDEBUG, "PVRDB - %s - deleting all channel groups from the database", __FUNCTION__);

  CStdString strWhereClause = FormatSQL("bIsRadio = %u", (bRadio ? 1 : 0));
  return DeleteValues("channelgroups", strWhereClause);
}

long CPVRDatabase::AddChannelGroup(const CStdString &strGroupName, int iSortOrder, bool bRadio /* = false */)
{
  long iReturn = -1;

  /* invalid group name */
  if (strGroupName.IsEmpty())
  {
    CLog::Log(LOGERROR, "PVRDB - %s - invalid group name", __FUNCTION__);
    return iReturn;
  }

  iReturn = GetChannelGroupId(strGroupName, bRadio);
  if (iReturn <= 0)
  {
    CStdString strQuery = FormatSQL("INSERT INTO channelgroups (sName, iSortOrder, bIsRadio) VALUES ('%s', %i, %i);",
        strGroupName.c_str(), iSortOrder, (bRadio ? 1 : 0));

    if (ExecuteQuery(strQuery))
      iReturn = (long) m_pDS->lastinsertid();
  }

  return iReturn;
}

bool CPVRDatabase::DeleteChannelGroup(int iGroupId, bool bRadio /* = false */)
{
  /* invalid group id */
  if (iGroupId <= 0)
  {
    CLog::Log(LOGERROR, "PVRDB - %s - invalid group id: %d",
        __FUNCTION__, iGroupId);
    return false;
  }

  CStdString strWhereClause = FormatSQL("idGroup = %u AND bIsRadio = %u", iGroupId, bRadio);
  return DeleteValues("channelgroups", strWhereClause);
}

bool CPVRDatabase::GetChannelGroupList(CPVRChannelGroups &results)
{
  bool bReturn = false;
  CStdString strQuery = FormatSQL("SELECT * from channelgroups ORDER BY idGroup;");
  ResultQuery(strQuery);

  try
  {
    while (!m_pDS->eof())
    {
      CPVRChannelGroup data(m_pDS->fv("bIsRadio").get_asBool());

      data.SetGroupID(m_pDS->fv("idGroup").get_asInt());
      data.SetGroupName(m_pDS->fv("sName").get_asString());
      data.SetSortOrder(m_pDS->fv("iSortOrder").get_asInt());

      results.push_back(data);
      m_pDS->next();
    }
    bReturn = true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s - couldn't load channels from the database", __FUNCTION__);
  }

  return bReturn;
}

bool CPVRDatabase::GetChannelGroupList(CPVRChannelGroups &results, bool bRadio)
{
  bool bReturn = false;
  CStdString strQuery = FormatSQL("SELECT * from channelgroups WHERE bIsRadio = %u ORDER BY idGroup;", bRadio);
  ResultQuery(strQuery);

  try
  {
    while (!m_pDS->eof())
    {
      CPVRChannelGroup data(m_pDS->fv("bIsRadio").get_asBool());

      data.SetGroupID(m_pDS->fv("idGroup").get_asInt());
      data.SetGroupName(m_pDS->fv("sName").get_asString());
      data.SetSortOrder(m_pDS->fv("iSortOrder").get_asInt());

      results.push_back(data);
      m_pDS->next();
    }
    bReturn = true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s - couldn't load channels from the database", __FUNCTION__);
  }

  m_pDS->close();
  return bReturn;
}

int CPVRDatabase::GetChannelsInGroup(CPVRChannelGroup *group)
{
  int iReturn = -1;

  /* invalid group id */
  if (group->GroupID() < 0)
  {
    CLog::Log(LOGERROR, "PVRDB - %s - invalid group id: %ld",
        __FUNCTION__, group->GroupID());
    return -1;
  }

  CStdString strQuery = FormatSQL("SELECT idChannel FROM channels WHERE idGroup = %u", group->GroupID());
  if (ResultQuery(strQuery))
  {
    iReturn = 0;

    while (!m_pDS->eof())
    {
      CPVRChannel *channel = g_PVRChannelGroups.GetGroupAll(group->IsRadio())->GetByChannelIDFromAll(m_pDS->fv("idChannel").get_asInt());

      if (channel && group->AddToGroup(channel))
        ++iReturn;
    }
  }

  return iReturn;
}

bool CPVRDatabase::SetChannelGroupName(int iGroupId, const CStdString &strNewName, bool bRadio /* = false */)
{
  bool bReturn = false;

  /* invalid group id */
  if (iGroupId <= 0)
  {
    CLog::Log(LOGERROR, "PVRDB - %s - invalid group id: %d",
        __FUNCTION__, iGroupId);
    return bReturn;
  }

  CStdString strQuery = FormatSQL("SELECT COUNT(1) FROM channelgroups WHERE idGroup = %u AND bIsRadio = %u;", iGroupId, (bRadio ? 1 : 0));
  if (ResultQuery(strQuery))
  {
    if (m_pDS->fv(0).get_asInt() > 0)
    {
      strQuery = FormatSQL("UPDATE channelgroups SET Name = '%s' WHERE idGroup = %i AND bIsRadio = %u;", strNewName.c_str(), iGroupId, (bRadio ? 1 : 0));
      bReturn = ExecuteQuery(strQuery);
    }
  }

  return bReturn;
}


bool CPVRDatabase::SetChannelGroupSortOrder(int iGroupId, int iSortOrder, bool bRadio /* = false */)
{
  bool bReturn = false;

  /* invalid group id */
  if (iGroupId <= 0)
  {
    CLog::Log(LOGERROR, "PVRDB - %s - invalid group id: %d",
        __FUNCTION__, iGroupId);
    return bReturn;
  }

  CStdString strQuery = FormatSQL("SELECT COUNT(1) FROM channelgroups WHERE idGroup = %u AND bIsRadio = %u;", iGroupId, (bRadio ? 1 : 0));
  if (ResultQuery(strQuery))
  {
    if (m_pDS->fv(0).get_asInt() > 0)
    {
      strQuery = FormatSQL("UPDATE channelgroups SET iSortOrder = %i WHERE idGroup = %i AND bIsRadio = %u;", iSortOrder, iGroupId, (bRadio ? 1 : 0));
      bReturn = ExecuteQuery(strQuery);
    }
  }

  return bReturn;
}


long CPVRDatabase::GetChannelGroupId(const CStdString &strGroupName, bool bRadio /* = false */)
{
  CStdString strWhereClause = FormatSQL("sName LIKE '%s' AND bIsRadio = %u", strGroupName.c_str(), (bRadio ? 1 : 0));
  CStdString strReturn = GetSingleValue("channelgroups", "idGroup", strWhereClause);

  m_pDS->close();

  return atoi(strReturn);
}

long CPVRDatabase::UpdateChannelGroup(const CPVRChannelGroup &group, bool bQueueWrite /* = false */)
{
  long iReturn = -1;

  CStdString strQuery;

  if (group.GroupID() < 0)
  {
    /* new group */
    strQuery = FormatSQL("INSERT INTO channelgroups ("
        "bIsRadio, sName, iSortOrder) "
        "VALUES (%i, '%s', %i);",
        (group.IsRadio() ? 1 :0), group.GroupName().c_str(), group.SortOrder());
  }
  else
  {
    /* update group */
    strQuery = FormatSQL("REPLACE INTO channelgroups ("
        "idGroup, bIsRadio, sName, iSortOrder) "
        "VALUES (%i, %i, '%s', %i);",
        group.GroupID(), (group.IsRadio() ? 1 :0), group.GroupName().c_str(), group.SortOrder());
  }

  if (bQueueWrite)
  {
    QueueInsertQuery(strQuery);
    iReturn = 0;
  }
  else if (ExecuteQuery(strQuery))
  {
    iReturn = (group.GroupID() < 0) ? (long) m_pDS->lastinsertid() : group.GroupID();
  }

  return iReturn;
}

/********** Client methods **********/

bool CPVRDatabase::EraseClients()
{
  CLog::Log(LOGDEBUG, "PVRDB - %s - deleting all clients from the database", __FUNCTION__);

  return DeleteValues("clients") &&
      DeleteValues("map_channels_clients");
}

long CPVRDatabase::AddClient(const CStdString &strClientName, const CStdString &strClientUid)
{
  long iReturn = -1;

  /* invalid client uid or name */
  if (strClientName.IsEmpty() || strClientUid.IsEmpty())
  {
    CLog::Log(LOGERROR, "PVRDB - %s - invalid client uid or name", __FUNCTION__);
    return iReturn;
  }

  /* only add this client if it's not already in the database */
  iReturn = GetClientId(strClientUid);
  if (iReturn <= 0)
  {
    CStdString strQuery = FormatSQL("INSERT INTO clients (sName, sUid) VALUES ('%s', '%s');",
        strClientName.c_str(), strClientUid.c_str());

    if (ExecuteQuery(strQuery))
    {
      iReturn = (long) m_pDS->lastinsertid();
    }
  }

  return iReturn;
}

bool CPVRDatabase::RemoveClient(const CStdString &strClientUid)
{
  /* invalid client uid */
  if (strClientUid.IsEmpty())
  {
    CLog::Log(LOGERROR, "PVRDB - %s - invalid client uid", __FUNCTION__);
    return false;
  }

  CStdString strWhereClause = FormatSQL("sUid = '%s'", strClientUid.c_str());
  return DeleteValues("clients", strWhereClause);
}

long CPVRDatabase::GetClientId(const CStdString &strClientUid)
{
  CStdString strWhereClause = FormatSQL("sUid = '%s'", strClientUid.c_str());
  CStdString strValue = GetSingleValue("clients", "idClient", strWhereClause);

  if (strValue.IsEmpty())
    return -1;

  return atol(strValue.c_str());
}

/********** EPG methods **********/

bool CPVRDatabase::EraseEpg()
{
  CLog::Log(LOGDEBUG, "PVRDB - %s - deleting all EPG data from the database", __FUNCTION__);

  if (DeleteValues("epg") && DeleteValues("lastepgscan"))
  {
    lastScanTime.SetValid(false);
    return true;
  }

  return false;
}

bool CPVRDatabase::EraseEpgForChannel(const CPVRChannel &channel, const CDateTime &start /* = NULL */, const CDateTime &end /* = NULL */)
{
  /* invalid channel */
  if (channel.ChannelID() <= 0)
  {
    CLog::Log(LOGERROR, "PVRDB - %s - invalid channel id: %li",
        __FUNCTION__, channel.ChannelID());
    return false;
  }

  CLog::Log(LOGDEBUG, "PVRDB - %s - clearing the EPG for channel '%s'",
      __FUNCTION__, channel.ChannelName().c_str());

  CStdString strWhereClause;
  strWhereClause = FormatSQL("idChannel = %u", channel.ChannelID());

  if (start != NULL)
  {
    time_t iStartTime;
    start.GetAsTime(iStartTime);
    strWhereClause.append(FormatSQL(" AND iStartTime < %u", iStartTime).c_str());
  }

  if (end != NULL)
  {
    time_t iEndTime;
    end.GetAsTime(iEndTime);
    strWhereClause.append(FormatSQL(" AND iEndTime > %u", iEndTime).c_str());
  }

  return DeleteValues("epg", strWhereClause);
}

bool CPVRDatabase::EraseOldEpgEntries()
{
  time_t iYesterday;
  CDateTime yesterday = CDateTime::GetCurrentDateTime() - CDateTimeSpan(1, 0, 0, 0);
  yesterday.GetAsTime(iYesterday);
  CStdString strWhereClause = FormatSQL("iEndTime < %u", iYesterday);

  return DeleteValues("epg", strWhereClause);
}

bool CPVRDatabase::RemoveEpgEntry(const CPVREpgInfoTag &tag)
{
  /* invalid tag */
  if (tag.BroadcastId() <= 0)
  {
    CLog::Log(LOGERROR, "PVRDB - %s - invalid EPG tag", __FUNCTION__);
    return false;
  }

  CStdString strWhereClause = FormatSQL("idBroadcast = %u", tag.BroadcastId());
  return DeleteValues("epg", strWhereClause);
}


int CPVRDatabase::GetEpgForChannel(CPVREpg *epg, const CDateTime &start /* = NULL */, const CDateTime &end /* = NULL */)
{
  int iReturn = -1;
  CPVRChannel *channel = epg->Channel();

  if (!channel)
  {
    CLog::Log(LOGERROR, "PVRDB - %s - EPG doesn't contain a channel tag", __FUNCTION__);
    return false;
  }

  CStdString strWhereClause;
  strWhereClause = FormatSQL("idChannel = %u", channel->ChannelID());

  if (start != NULL)
  {
    time_t iStartTime;
    start.GetAsTime(iStartTime);
    strWhereClause.append(FormatSQL(" AND iStartTime < %u", iStartTime).c_str());
  }

  if (end != NULL)
  {
    time_t iEndTime;
    end.GetAsTime(iEndTime);
    strWhereClause.append(FormatSQL(" AND iEndTime > %u", iEndTime).c_str());
  }

  CStdString strQuery;
  strQuery.Format("SELECT * FROM epg WHERE %s ORDER BY iStartTime ASC;", strWhereClause.c_str());

  int iNumRows = ResultQuery(strQuery);

  if (iNumRows > 0)
  {
    try
    {
      while (!m_pDS->eof())
      {
        CPVREpgInfoTag newTag;

        time_t iStartTime, iEndTime, iFirstAired;
        iStartTime = (time_t) m_pDS->fv("iStartTime").get_asInt();
        CDateTime startTime(iStartTime);
        newTag.SetStart(startTime);

        iEndTime = (time_t) m_pDS->fv("iEndTime").get_asInt();
        CDateTime endTime(iEndTime);
        newTag.SetEnd(endTime);

        iFirstAired = (time_t) m_pDS->fv("iFirstAired").get_asInt();
        CDateTime firstAired(iFirstAired);
        newTag.SetFirstAired(firstAired);

        newTag.SetUniqueBroadcastID(m_pDS->fv("iBroadcastUid").get_asInt());
        newTag.SetBroadcastId      (m_pDS->fv("idBroadcast").get_asInt());
        newTag.SetTitle            (m_pDS->fv("sTitle").get_asString().c_str());
        newTag.SetPlotOutline      (m_pDS->fv("sPlotOutline").get_asString().c_str());
        newTag.SetPlot             (m_pDS->fv("sPlot").get_asString().c_str());
        newTag.SetGenre            (m_pDS->fv("iGenreType").get_asInt(),
                                    m_pDS->fv("iGenreSubType").get_asInt());
        newTag.SetParentalRating   (m_pDS->fv("iParentalRating").get_asInt());
        newTag.SetStarRating       (m_pDS->fv("iStarRating").get_asInt());
        newTag.SetNotify           (m_pDS->fv("bNotify").get_asBool());
        newTag.SetEpisodeNum       (m_pDS->fv("sEpisodeId").get_asString().c_str());
        newTag.SetEpisodePart      (m_pDS->fv("sEpisodePart").get_asString().c_str());
        newTag.SetEpisodeName      (m_pDS->fv("sEpisodeName").get_asString().c_str());
        newTag.SetSeriesNum        (m_pDS->fv("sSeriesId").get_asString().c_str());

        epg->UpdateEntry(newTag, false);
        m_pDS->next();
        ++iReturn;
      }
    }
    catch (...)
    {
      CLog::Log(LOGERROR, "%s - couldn't load EPG data from the database", __FUNCTION__);
    }
  }
  return iReturn;
}

CDateTime CPVRDatabase::GetEpgDataStart(long iChannelId /* = -1 */)
{
  time_t iFirstProgramme;
  CStdString strWhereClause;

  if (iChannelId > 0)
    strWhereClause = FormatSQL("idChannel = '%u'", iChannelId);

  CStdString strReturn = GetSingleValue("epg", "iStartTime", strWhereClause, "iStartTime ASC");
  if (!strReturn.IsEmpty())
  {
    iFirstProgramme = atoi(strReturn);
    CDateTime firstProgramme(iFirstProgramme);

    if (firstProgramme.IsValid())
      return firstProgramme;
  }

  return CDateTime::GetCurrentDateTime();
}

CDateTime CPVRDatabase::GetEpgDataEnd(long iChannelId /* = -1 */)
{
  time_t iLastProgramme;
  CStdString strWhereClause;

  if (iChannelId > 0)
    strWhereClause = FormatSQL("idChannel = '%u'", iChannelId);

  CStdString strReturn = GetSingleValue("epg", "iEndTime", strWhereClause, "iEndTime DESC");
  if (!strReturn.IsEmpty())
  {
    iLastProgramme = atoi(strReturn);
    CDateTime lastProgramme(iLastProgramme);

    if (lastProgramme.IsValid())
      return lastProgramme;
  }

  return CDateTime::GetCurrentDateTime();
}

CDateTime CPVRDatabase::GetLastEpgScanTime()
{
  if (lastScanTime.IsValid())
    return lastScanTime;

  CStdString strValue = GetSingleValue("lastepgscan", "iLastScan", "idEpg = 0");

  if (strValue.IsEmpty())
    return -1;

  time_t iLastScan = atoi(strValue.c_str());
  CDateTime lastScan(iLastScan);
  lastScanTime = lastScan;

  return lastScan;
}

bool CPVRDatabase::UpdateLastEpgScanTime(void)
{
  CDateTime now = CDateTime::GetCurrentDateTime();
  CLog::Log(LOGDEBUG, "PVRDB - %s - updating last scan time to '%s'",
      __FUNCTION__, now.GetAsDBDateTime().c_str());
  lastScanTime = now;

  bool bReturn = true;
  time_t iLastScan;
  now.GetAsTime(iLastScan);
  CStdString strQuery = FormatSQL("REPLACE INTO lastepgscan(idEpg, iLastScan) VALUES (0, %u);",
      iLastScan);

  bReturn = ExecuteQuery(strQuery);

  return bReturn;
}

bool CPVRDatabase::UpdateEpgEntry(const CPVREpgInfoTag &tag, bool bSingleUpdate /* = true */, bool bLastUpdate /* = false */)
{
  int bReturn = false;

  time_t iStartTime, iEndTime, iFirstAired;
  tag.Start().GetAsTime(iStartTime);
  tag.End().GetAsTime(iEndTime);
  tag.FirstAired().GetAsTime(iFirstAired);

  int iBroadcastId = tag.BroadcastId();
  if (iBroadcastId <= 0)
  {
    CStdString strWhereClause;
    if (tag.UniqueBroadcastID() > 0)
    {
      strWhereClause = FormatSQL("(iBroadcastUid = '%u' OR iStartTime = %u) AND idChannel = '%u'",
          tag.UniqueBroadcastID(), iStartTime, tag.ChannelTag()->ChannelID());
    }
    else
    {
      strWhereClause = FormatSQL("iStartTime = %u AND idChannel = '%u'",
          iStartTime, tag.ChannelTag()->ChannelID());
    }
    CStdString strValue = GetSingleValue("epg", "idBroadcast", strWhereClause);

    if (!strValue.IsEmpty())
      iBroadcastId = atoi(strValue);
  }

  CStdString strQuery;

  if (iBroadcastId < 0)
  {
    strQuery = FormatSQL("INSERT INTO epg (idChannel, iStartTime, "
        "iEndTime, sTitle, sPlotOutline, sPlot, iGenreType, iGenreSubType, "
        "iFirstAired, iParentalRating, iStarRating, bNotify, sSeriesId, "
        "sEpisodeId, sEpisodePart, sEpisodeName, iBroadcastUid) "
        "VALUES (%u, %u, %u, '%s', '%s', '%s', %i, %i, %u, %i, %i, %i, '%s', '%s', '%s', '%s', %i);",
        tag.ChannelTag()->ChannelID(), iStartTime, iEndTime,
        tag.Title().c_str(), tag.PlotOutline().c_str(), tag.Plot().c_str(), tag.GenreType(), tag.GenreSubType(),
        iFirstAired, tag.ParentalRating(), tag.StarRating(), tag.Notify(),
        tag.SeriesNum().c_str(), tag.EpisodeNum().c_str(), tag.EpisodePart().c_str(), tag.EpisodeName().c_str(),
        tag.UniqueBroadcastID());
  }
  else
  {
    strQuery = FormatSQL("REPLACE INTO epg (idChannel, iStartTime, "
        "iEndTime, sTitle, sPlotOutline, sPlot, iGenreType, iGenreSubType, "
        "iFirstAired, iParentalRating, iStarRating, bNotify, sSeriesId, "
        "sEpisodeId, sEpisodePart, sEpisodeName, iBroadcastUid, idBroadcast) "
        "VALUES (%u, %u, %u, '%s', '%s', '%s', %i, %i, %u, %i, %i, %i, '%s', '%s', '%s', '%s', %i, %i);",
        tag.ChannelTag()->ChannelID(), iStartTime, iEndTime,
        tag.Title().c_str(), tag.PlotOutline().c_str(), tag.Plot().c_str(), tag.GenreType(), tag.GenreSubType(),
        tag.FirstAired().GetAsDBDateTime().c_str(), tag.ParentalRating(), tag.StarRating(), tag.Notify(),
        tag.SeriesNum().c_str(), tag.EpisodeNum().c_str(), tag.EpisodePart().c_str(), tag.EpisodeName().c_str(),
        tag.UniqueBroadcastID(), iBroadcastId);
  }

  if (bSingleUpdate)
  {
    bReturn = ExecuteQuery(strQuery);
  }
  else
  {
    bReturn = QueueInsertQuery(strQuery);

    if (bLastUpdate)
      CommitInsertQueries();
  }

  if ((bSingleUpdate || bLastUpdate) && GetEpgDataEnd(tag.ChannelTag()->ChannelID()) > tag.End())
    EraseEpgForChannel(*tag.ChannelTag(), NULL, tag.End());

  return bReturn;
}
