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
#include "PVRChannels.h"
#include "PVREpgInfoTag.h"
#include "PVRChannelGroups.h"
#include "PVRChannelGroup.h"

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
        ");\n"
    );
    m_pDS->exec("CREATE INDEX idx_clients_sUid on clients(sUid);\n");

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
        ");\n"
    );
    m_pDS->exec("CREATE UNIQUE INDEX idx_unique_iChannelNumber on channels(iChannelNumber, bIsRadio);\n");
    m_pDS->exec("CREATE INDEX idx_idClient on channels(idClient);\n");
    m_pDS->exec("CREATE INDEX idx_iChannelNumber on channels(iChannelNumber);\n");
    m_pDS->exec("CREATE INDEX idx_iLastWatched on channels(iLastWatched);\n");
    m_pDS->exec("CREATE INDEX idx_bIsRadio on channels(bIsRadio);\n");
    m_pDS->exec("CREATE INDEX idx_bIsHidden on channels(bIsHidden);\n");

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
    //        ");\n"
    //    );
    //    m_pDS->exec("CREATE UNIQUE INDEX idx_idChannel_idClient on map_channels_clients(idChannel, idClient);\n");

    CLog::Log(LOGDEBUG, "PVRDB - %s - creating view 'vw_last_watched'", __FUNCTION__);
    m_pDS->exec(
        "CREATE VIEW vw_last_watched "
        "AS SELECT idChannel, iChannelNumber, sChannelName "
        "FROM Channels "
        "ORDER BY iLastWatched DESC;\n"
    );

    CLog::Log(LOGDEBUG, "PVRDB - %s - creating table 'channelgroups'", __FUNCTION__);
    m_pDS->exec(
        "CREATE TABLE channelgroups ("
          "idGroup   integer primary key,"
          "bIsRadio   bool, "
          "sName      text,"
          "iSortOrder integer"
        ")\n"
    );
    m_pDS->exec("CREATE INDEX idx_channelgroups_bIsRadio on channelgroups(bIsRadio)\n");
    m_pDS->exec("CREATE INDEX idx_channelgroups_sName on channelgroups(sName)\n");

    // TODO use a mapping table so multiple groups per channel can be implemented
    // replaces idGroup in the channels table
    //    CLog::Log(LOGDEBUG, "PVRDB - %s - creating table 'map_channelgroups_channels'", __FUNCTION__);
    //    m_pDS->exec(
    //        "CREATE TABLE map_channelgroups_channels ("
    //          "idChannel integer, "
    //          "idGroup   integer"
    //        ");\n"
    //    );
    //    m_pDS->exec("CREATE UNIQUE INDEX idx_idChannel_idGroup on map_channelgroups_channels(idChannel, idGroup);\n");

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
        ");\n"
    );

    CLog::Log(LOGDEBUG, "PVRDB - %s - creating table 'EpgData'", __FUNCTION__);
    m_pDS->exec(
        "CREATE TABLE EpgData ("
          "BroadcastId integer primary key, "
          "BroadcastUid integer, "
          "ChannelId integer, "
          "Title text, "
          "PlotOutline text, "
          "Plot text, "
          "StartTime datetime, "
          "EndTime datetime, "
          "GenreType integer, "
          "GenreSubType integer, "
          "FirstAired datetime, "
          "ParentalRating integer, "
          "StarRating integer, "
          "Notify bool, "
          "SeriesId text, "
          "EpisodeId text, "
          "EpisodePart text, "
          "EpisodeName text"
        ")\n"
    );
    m_pDS->exec("CREATE UNIQUE INDEX ix_EpgUniqueStartTime on EpgData(ChannelId, StartTime desc)\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_EpgBroadcastUid on EpgData(BroadcastUid)\n");
    m_pDS->exec("CREATE INDEX ix_EpgChannelId on EpgData(ChannelId)\n");
    m_pDS->exec("CREATE INDEX ix_EpgStartTime on EpgData(StartTime)\n");
    m_pDS->exec("CREATE INDEX ix_EpgEndTime on EpgData(EndTime)\n");

    CLog::Log(LOGDEBUG, "PVRDB - %s - creating table 'LastEPGScan'", __FUNCTION__);
    m_pDS->exec("CREATE TABLE LastEPGScan (idScan integer primary key, ScanTime datetime)\n");

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

  CStdString strWhereClause = FormatSQL("idClient = %u", iClientId);
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
        "sIconPath, sChannelName, bIsVirtual, bEPGEnabled, sEPGScraper, idClient, "
        "iClientChannelNumber, sInputFormat, sStreamURL, iEncryptionSystem) "
        "VALUES (%i, %i, %i, %i, %i, '%s', '%s', %i, %i, '%s', %i, %i, '%s', '%s', %i)\n",
        channel.UniqueID(), channel.ChannelNumber(), channel.GroupID(), (channel.IsRadio() ? 1 :0), (channel.IsHidden() ? 1 : 0),
        channel.IconPath().c_str(), channel.ChannelName().c_str(), (channel.IsVirtual() ? 1 : 0), (channel.EPGEnabled() ? 1 : 0), channel.EPGScraper().c_str(), channel.ClientID(),
        channel.ClientChannelNumber(), channel.InputFormat().c_str(), channel.StreamURL().c_str(), channel.EncryptionSystem());
  }
  else
  {
    /* update channel */
    strQuery = FormatSQL("REPLACE INTO channels ("
        "iUniqueId, iChannelNumber, idGroup, bIsRadio, bIsHidden, "
        "sIconPath, sChannelName, bIsVirtual, bEPGEnabled, sEPGScraper, idClient, "
        "iClientChannelNumber, sInputFormat, sStreamURL, iEncryptionSystem, idChannel) "
        "VALUES (%i, %i, %i, %i, %i, '%s', '%s', %i, %i, '%s', %i, %i, '%s', '%s', %i, %i)\n",
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

int CPVRDatabase::GetChannels(CPVRChannels &results, bool bIsRadio)
{
  int iReturn = -1;

  CStdString strQuery = FormatSQL("SELECT * FROM channels WHERE bIsRadio = %u ORDER BY iChannelNumber\n", bIsRadio);
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
        channel->m_iClientId               = m_pDS->fv("idClient").get_asInt();
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
  CStdString strQuery = FormatSQL("SELECT COUNT(1) FROM channels WHERE bIsRadio = %u AND bIsHidden = %u\n",
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

  CStdString strQuery = FormatSQL("SELECT * FROM channelsettings WHERE idChannel = %u\n", channel.ChannelID());

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
         "(%i, %i, %i, %f, %f, %i, %i, %f, %i, %f, %f, %f, %f, %f, %i, %i, %i, %i, %i, %i, %f, %f)\n",
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
    CStdString strQuery = FormatSQL("INSERT INTO channelgroups (idGroup, sName, iSortOrder, bIsRadio) VALUES (NULL, '%s', %i, %i)\n",
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


bool CPVRDatabase::GetChannelGroupList(CPVRChannelGroups &results, bool bRadio /* = false */)
{
  bool bReturn = false;
  CStdString strQuery = FormatSQL("SELECT * from channelgroups WHERE bIsRadio = %u ORDER BY iSortOrder\n", bRadio);
  int iNumRows = ResultQuery(strQuery);

  if (iNumRows > 0)
  {
    try
    {
      while (!m_pDS->eof())
      {
        CPVRChannelGroup data;

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
  }

  m_pDS->close();
  return bReturn;
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

  CStdString strQuery = FormatSQL("SELECT COUNT(1) FROM channelgroups WHERE idGroup = %u AND bIsRadio = %u\n", iGroupId, (bRadio ? 1 : 0));
  if (ResultQuery(strQuery))
  {
    if (m_pDS->fv(0).get_asInt() > 0)
    {
      strQuery = FormatSQL("UPDATE channelgroups SET Name = '%s' WHERE idGroup = %i AND bIsRadio = %u\n", strNewName.c_str(), iGroupId, (bRadio ? 1 : 0));
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

  CStdString strQuery = FormatSQL("SELECT COUNT(1) FROM channelgroups WHERE idGroup = %u AND bIsRadio = %u\n", iGroupId, (bRadio ? 1 : 0));
  if (ResultQuery(strQuery))
  {
    if (m_pDS->fv(0).get_asInt() > 0)
    {
      strQuery = FormatSQL("UPDATE channelgroups SET iSortOrder = %i WHERE idGroup = %i AND bIsRadio = %u\n", iSortOrder, iGroupId, (bRadio ? 1 : 0));
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
    CStdString strQuery = FormatSQL("INSERT INTO clients (sName, sUid) VALUES ('%s', '%s')\n",
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

  if (DeleteValues("EpgData") && DeleteValues("LastEPGScan"))
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
  strWhereClause = FormatSQL("ChannelId = %u", channel.ChannelID());

  if (start != NULL)
    strWhereClause.append(FormatSQL(" AND StartTime < %u", start.GetAsDBDateTime().c_str()).c_str());

  if (end != NULL)
    strWhereClause.append(FormatSQL(" AND EndTime > %u", end.GetAsDBDateTime().c_str()).c_str());

  return DeleteValues("EpgData", strWhereClause);
}

bool CPVRDatabase::EraseOldEpgEntries()
{
  CDateTime yesterday = CDateTime::GetCurrentDateTime() - CDateTimeSpan(1, 0, 0, 0);
  CStdString strWhereClause = FormatSQL("EndTime < '%s'", yesterday.GetAsDBDateTime().c_str());

  return DeleteValues("EpgData", strWhereClause);
}

bool CPVRDatabase::RemoveEpgEntry(const CPVREpgInfoTag &tag)
{
  /* invalid tag */
  if (tag.BroadcastId() <= 0 && tag.UniqueBroadcastID() <= 0)
  {
    CLog::Log(LOGERROR, "PVRDB - %s - invalid EPG tag", __FUNCTION__);
    return false;
  }

  CStdString strWhereClause;

  if (tag.BroadcastId() > 0)
    strWhereClause = FormatSQL("BroadcastId = %u", tag.BroadcastId());
  else
    strWhereClause = FormatSQL("BroadcastUid = %u", tag.UniqueBroadcastID());

  return DeleteValues("EpgData", strWhereClause);
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
  strWhereClause = FormatSQL("ChannelId = %u", channel->ChannelID());

  if (start != NULL)
    strWhereClause.append(FormatSQL(" AND StartTime < %u", start.GetAsDBDateTime().c_str()).c_str());

  if (end != NULL)
    strWhereClause.append(FormatSQL(" AND EndTime > %u", end.GetAsDBDateTime().c_str()).c_str());

  CStdString strQuery;
  strQuery.Format("SELECT * FROM EpgData WHERE %s ORDER BY StartTime ASC\n", strWhereClause.c_str());

  int iNumRows = ResultQuery(strQuery);

  if (iNumRows > 0)
  {
    try
    {
      while (!m_pDS->eof())
      {
        int iBroadcastUid =          m_pDS->fv("BroadcastUid").get_asInt();

        CDateTime startTime, endTime, firstAired;
        startTime.SetFromDBDateTime (m_pDS->fv("StartTime").get_asString());
        endTime.SetFromDBDateTime   (m_pDS->fv("EndTime").get_asString());
        firstAired.SetFromDBDateTime(m_pDS->fv("FirstAired").get_asString());

        CPVREpgInfoTag newTag(iBroadcastUid);
        newTag.SetBroadcastId       (m_pDS->fv("BroadcastId").get_asInt());
        newTag.SetTitle             (m_pDS->fv("Title").get_asString().c_str());
        newTag.SetPlotOutline       (m_pDS->fv("PlotOutline").get_asString().c_str());
        newTag.SetPlot              (m_pDS->fv("Plot").get_asString().c_str());
        newTag.SetStart             (startTime);
        newTag.SetEnd               (endTime);
        newTag.SetGenre             (m_pDS->fv("GenreType").get_asInt(),
                                     m_pDS->fv("GenreSubType").get_asInt());
        newTag.SetFirstAired        (firstAired);
        newTag.SetParentalRating    (m_pDS->fv("ParentalRating").get_asInt());
        newTag.SetStarRating        (m_pDS->fv("StarRating").get_asInt());
        newTag.SetNotify            (m_pDS->fv("Notify").get_asBool());
        newTag.SetEpisodeNum        (m_pDS->fv("EpisodeId").get_asString().c_str());
        newTag.SetEpisodePart       (m_pDS->fv("EpisodePart").get_asString().c_str());
        newTag.SetEpisodeName       (m_pDS->fv("EpisodeName").get_asString().c_str());

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
  CDateTime firstProgramme;
  CStdString strWhereClause;

  if (iChannelId > 0)
    strWhereClause = FormatSQL("ChannelId = '%u'", iChannelId);

  CStdString strReturn = GetSingleValue("EpgData", "StartTime", strWhereClause, "StartTime ASC");
  if (!strReturn.IsEmpty())
    firstProgramme.SetFromDBDateTime(strReturn);

  if (!firstProgramme.IsValid())
    return CDateTime::GetCurrentDateTime();
  else
    return firstProgramme;
}

CDateTime CPVRDatabase::GetEpgDataEnd(long iChannelId /* = -1 */)
{
  CDateTime lastProgramme;
  CStdString strWhereClause;

  if (iChannelId > 0)
    strWhereClause = FormatSQL("ChannelId = '%u'", iChannelId);

  CStdString strReturn = GetSingleValue("EpgData", "EndTime", strWhereClause, "EndTime DESC");
  if (!strReturn.IsEmpty())
    lastProgramme.SetFromDBDateTime(strReturn);

  if (!lastProgramme.IsValid())
    return CDateTime::GetCurrentDateTime();
  else
    return lastProgramme;
}

CDateTime CPVRDatabase::GetLastEpgScanTime()
{
  if (lastScanTime.IsValid())
    return lastScanTime;

  CStdString strValue = GetSingleValue("LastEPGScan", "ScanTime");

  if (strValue.IsEmpty())
    return -1;

  CDateTime lastTime;
  lastTime.SetFromDBDateTime(strValue);
  lastScanTime = lastTime;

  return lastTime;
}

bool CPVRDatabase::UpdateLastEpgScanTime(void)
{
  CDateTime now = CDateTime::GetCurrentDateTime();
  CLog::Log(LOGDEBUG, "PVRDB - %s - updating last scan time to '%s'",
      __FUNCTION__, now.GetAsDBDateTime().c_str());
  lastScanTime = now;

  bool bReturn = true;
  CStdString strQuery = FormatSQL("REPLACE INTO LastEPGScan (ScanTime) VALUES ('%s')\n",
      now.GetAsDBDateTime().c_str());

  bReturn = ExecuteQuery(strQuery);

  return bReturn;
}

bool CPVRDatabase::UpdateEpgEntry(const CPVREpgInfoTag &tag, bool bSingleUpdate /* = true */, bool bLastUpdate /* = false */)
{
  int bReturn = false;

  /* invalid tag */
  if (tag.UniqueBroadcastID() <= 0)
  {
    CLog::Log(LOGERROR, "PVRDB - %s - invalid EPG tag", __FUNCTION__);
    return bReturn;
  }

  int iBroadcastId = tag.BroadcastId();
  if (iBroadcastId)
  {
    CStdString strWhereClause = FormatSQL("(BroadcastUid = '%u' OR StartTime = '%s') AND ChannelId = '%u'",
        tag.UniqueBroadcastID(), tag.Start().GetAsDBDateTime().c_str(), tag.ChannelTag()->ChannelID());
    CStdString strValue = GetSingleValue("EpgData", "BroadcastId", strWhereClause);

    if (!strValue.IsEmpty())
      iBroadcastId = atoi(strValue);
  }

  CStdString strQuery;

  if (iBroadcastId < 0)
  {
    strQuery = FormatSQL("INSERT INTO EpgData (ChannelId, StartTime, "
        "EndTime, Title, PlotOutline, Plot, GenreType, GenreSubType, "
        "FirstAired, ParentalRating, StarRating, Notify, SeriesId, "
        "EpisodeId, EpisodePart, EpisodeName, BroadcastUid) "
        "VALUES (%i, '%s', '%s', '%s', '%s', '%s', %i, %i, '%s', %i, %i, %i, '%s', '%s', '%s', '%s', %i)\n",
        tag.ChannelTag()->ChannelID(), tag.Start().GetAsDBDateTime().c_str(), tag.End().GetAsDBDateTime().c_str(),
        tag.Title().c_str(), tag.PlotOutline().c_str(), tag.Plot().c_str(), tag.GenreType(), tag.GenreSubType(),
        tag.FirstAired().GetAsDBDateTime().c_str(), tag.ParentalRating(), tag.StarRating(), tag.Notify(),
        tag.SeriesNum().c_str(), tag.EpisodeNum().c_str(), tag.EpisodePart().c_str(), tag.EpisodeName().c_str(),
        tag.UniqueBroadcastID());
  }
  else
  {
    strQuery = FormatSQL("REPLACE INTO EpgData (ChannelId, StartTime, "
        "EndTime, Title, PlotOutline, Plot, GenreType, GenreSubType, "
        "FirstAired, ParentalRating, StarRating, Notify, SeriesId, "
        "EpisodeId, EpisodePart, EpisodeName, BroadcastUid, BroadcastId) "
        "VALUES (%i, '%s', '%s', '%s', '%s', '%s', %i, %i, '%s', %i, %i, %i, '%s', '%s', '%s', '%s', %i, %i)\n",
        tag.ChannelTag()->ChannelID(), tag.Start().GetAsDBDateTime().c_str(), tag.End().GetAsDBDateTime().c_str(),
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
