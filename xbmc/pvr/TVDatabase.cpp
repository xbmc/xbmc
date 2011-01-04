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

#include "TVDatabase.h"
#include "utils/GUIInfoManager.h"
#include "AdvancedSettings.h"
#include "utils/log.h"

#include "PVREpgInfoTag.h"
#include "PVRChannelGroups.h"
#include "PVRChannelGroup.h"

using namespace std;
using namespace dbiplus;

CTVDatabase::CTVDatabase(void)
{
  lastScanTime.SetValid(false);
}

CTVDatabase::~CTVDatabase(void)
{
}

bool CTVDatabase::Open()
{
  return CDatabase::Open(g_advancedSettings.m_databaseTV);
}

bool CTVDatabase::CreateTables()
{
  try
  {
    CDatabase::CreateTables();

    CLog::Log(LOGINFO, "%s - creating tables", __FUNCTION__);

    CLog::Log(LOGDEBUG, "%s - creating table 'Clients'", __FUNCTION__);
    m_pDS->exec(
        "CREATE TABLE Clients ("
          "ClientDbId integer primary key, "
          "Name       text, "
          "ClientId   text)"
        "\n"
    );
    m_pDS->exec("CREATE INDEX ix_ClientId on Clients(ClientId)\n");

    CLog::Log(LOGDEBUG, "%s - creating table 'Channels'", __FUNCTION__);
    m_pDS->exec(
        "CREATE TABLE Channels ("
          "ChannelId           integer primary key, "
          "UniqueId            integer, "
          "ChannelNumber       integer, "
          "GroupId             integer, "
          "IsRadio             bool, "
          "IsHidden            bool, "
          "IconPath            text, "
          "ChannelName         text, "
          "IsVirtual           bool, "
          "EPGEnabled          bool, "
          "EPGScraper          text, "
          "ClientId            integer, "
          "ClientChannelNumber integer, "
          "InputFormat         text, "
          "StreamURL           text, "
          "EncryptionSystem    integer"
        ")\n"
    );
    m_pDS->exec("CREATE INDEX ix_ChannelClientId on Channels(ClientId)\n");
    m_pDS->exec("CREATE INDEX ix_ChannelNumber on Channels(ChannelNumber)\n");
    m_pDS->exec("CREATE INDEX ix_ChannelIsRadio on Channels(IsRadio)\n");
    m_pDS->exec("CREATE INDEX ix_ChannelIsHidden on Channels(IsHidden)\n");

    CLog::Log(LOGDEBUG, "%s - creating table 'LastChannel'", __FUNCTION__);
    m_pDS->exec(
        "CREATE TABLE LastChannel ("
          "ChannelId integer primary key, "
          "Number    integer, "
          "Name      text"
        ")\n"
    );

    CLog::Log(LOGDEBUG, "%s - creating table 'ChannelSettings'", __FUNCTION__);
    m_pDS->exec(
        "CREATE TABLE ChannelSettings ("
          "ChannelId           integer primary key, "
          "InterlaceMethod     integer, "
          "ViewMode            integer, "
          "CustomZoomAmount    float, "
          "PixelRatio          float, "
          "AudioStream         integer, "
          "SubtitleStream      integer,"
          "SubtitleDelay       float, "
          "SubtitlesOn         bool, "
          "Brightness          float, "
          "Contrast            float, "
          "Gamma               float,"
          "VolumeAmplification float, "
          "AudioDelay          float, "
          "OutputToAllSpeakers bool, "
          "Crop                bool, "
          "CropLeft            integer, "
          "CropRight           integer, "
          "CropTop             integer, "
          "CropBottom          integer, "
          "Sharpness           float, "
          "NoiseReduction      float"
        ")\n"
    );

    CLog::Log(LOGDEBUG, "%s - creating table 'ChannelGroup'", __FUNCTION__);
    m_pDS->exec(
        "CREATE TABLE ChannelGroup ("
          "GroupId   integer primary key,"
          "IsRadio   bool, "
          "Name      text,"
          "SortOrder integer"
        ")\n"
    );
    m_pDS->exec("CREATE INDEX ix_ChannelGroupIsRadio on ChannelGroup(IsRadio)\n");
    m_pDS->exec("CREATE INDEX ix_ChannelGroupName on ChannelGroup(Name)\n");

    CLog::Log(LOGDEBUG, "%s - creating table 'EpgData'", __FUNCTION__);
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

    CLog::Log(LOGDEBUG, "%s - creating table 'LastEPGScan'", __FUNCTION__);
    m_pDS->exec("CREATE TABLE LastEPGScan (idScan integer primary key, ScanTime datetime)\n");
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s unable to create TV tables:%i", __FUNCTION__, (int)GetLastError());
    return false;
  }

  return true;
}

bool CTVDatabase::UpdateOldVersion(int iVersion)
{
  if (iVersion < 5)
  {
    /* TODO since sqlite doesn't support all ALTER TABLE statements, we have to rename things by code. not supported at the moment */
    CLog::Log(LOGERROR, "%s - Incompatible database version!", __FUNCTION__);
    return false;
  }
  return true;
}

/********** Channel methods **********/

bool CTVDatabase::EraseChannels()
{
  return DeleteValues("Channels");
}

bool CTVDatabase::EraseClientChannels(long iClientId)
{
  CStdString strWhereClause = FormatSQL("ClientID = %u", iClientId);
  return DeleteValues("Channels", strWhereClause);
}

long CTVDatabase::UpdateChannel(const CPVRChannel &channel)
{
  long iReturn = -1;

  /* invalid channel */
  if (channel.UniqueID() < 0)
    return iReturn;

  CStdString strQuery;

  if (channel.ChannelID() <= 0)
  {
    /* new channel */
    strQuery = FormatSQL("INSERT INTO Channels ("
        "UniqueId, ChannelNumber, GroupId, IsRadio, IsHidden, "
        "IconPath, ChannelName, IsVirtual, EPGEnabled, EPGScraper, ClientId, "
        "ClientChannelNumber, InputFormat, StreamURL, EncryptionSystem) "
        "VALUES (%i, %i, %i, %i, %i, '%s', '%s', %i, %i, '%s', %i, %i, '%s', '%s', %i)",
        channel.UniqueID(), channel.ChannelNumber(), channel.GroupID(), (channel.IsRadio() ? 1 :0), (channel.IsHidden() ? 1 : 0),
        channel.IconPath().c_str(), channel.ChannelName().c_str(), (channel.IsVirtual() ? 1 : 0), (channel.EPGEnabled() ? 1 : 0), channel.EPGScraper().c_str(), channel.ClientID(),
        channel.ClientChannelNumber(), channel.InputFormat().c_str(), channel.StreamURL().c_str(), channel.EncryptionSystem());
  }
  else
  {
    /* update channel */
    strQuery = FormatSQL("UPDATE Channels SET"
        "ChannelId = %i, UniqueId = %i, ChannelNumber = %i, GroupId = %i, IsRadio = %i, IsHidden = %i, "
        "IconPath = '%s', ChannelName = '%s', IsVirtual = %i, EPGEnabled = %i, EPGScraper = '%s', ClientId = %i, "
        "ClientChannelNumber = %i, InputFormat = '%s', StreamURL = '%s', EncryptionSystem = %i",
        channel.ChannelID(), channel.UniqueID(), channel.ChannelNumber(), channel.GroupID(), (channel.IsRadio() ? 1 :0), (channel.IsHidden() ? 1 : 0),
        channel.IconPath().c_str(), channel.ChannelName().c_str(), (channel.IsVirtual() ? 1 : 0), (channel.EPGEnabled() ? 1 : 0), channel.EPGScraper().c_str(), channel.ClientID(),
        channel.ClientChannelNumber(), channel.InputFormat().c_str(), channel.StreamURL().c_str(), channel.EncryptionSystem());
  }

  if (ExecuteQuery(strQuery))
    iReturn = (channel.ChannelID() <= 0) ? (long) m_pDS->lastinsertid() : channel.ChannelID();
  else
    iReturn = -1;

  return iReturn;
}

bool CTVDatabase::RemoveChannel(const CPVRChannel &channel)
{
  /* invalid channel */
  if (channel.ChannelID() < 0)
    return false;

  CStdString strWhereClause = FormatSQL("ChannelId = '%u'", channel.ChannelID());
  return DeleteValues("Channels", strWhereClause);
}

int CTVDatabase::GetChannels(CPVRChannels &results, bool bIsRadio)
{
  int iReturn = -1;

  CStdString strQuery = FormatSQL("SELECT * FROM Channels WHERE IsRadio = %u ORDER BY ChannelNumber", bIsRadio);
  int iNumRows = ResultQuery(strQuery);

  if (iNumRows > 0)
  {
    try
    {
      while (!m_pDS->eof())
      {
        CPVRChannel *channel = new CPVRChannel();

        channel->m_iChannelId              = m_pDS->fv("ChannelId").get_asInt();
        channel->m_iUniqueId               = m_pDS->fv("UniqueId").get_asInt();
        channel->m_iChannelNumber          = m_pDS->fv("ChannelNumber").get_asInt();
        channel->m_iChannelGroupId         = m_pDS->fv("GroupId").get_asInt();
        channel->m_bIsRadio                = m_pDS->fv("IsRadio").get_asBool();
        channel->m_bIsHidden               = m_pDS->fv("IsHidden").get_asBool();
        channel->m_strIconPath             = m_pDS->fv("IconPath").get_asString();
        channel->m_strChannelName          = m_pDS->fv("ChannelName").get_asString();
        channel->m_bIsVirtual              = m_pDS->fv("IsVirtual").get_asBool();
        channel->m_bEPGEnabled             = m_pDS->fv("EPGEnabled").get_asBool();
        channel->m_strEPGScraper           = m_pDS->fv("EPGScraper").get_asString();
        channel->m_iClientId               = m_pDS->fv("ClientId").get_asInt();
        channel->m_iClientChannelNumber    = m_pDS->fv("ClientChannelNumber").get_asInt();
        channel->m_strInputFormat          = m_pDS->fv("InputFormat").get_asString();
        channel->m_strStreamURL            = m_pDS->fv("StreamURL").get_asString();
        channel->m_iClientEncryptionSystem = m_pDS->fv("EncryptionSystem").get_asInt();

        channel->UpdatePath();

        CLog::Log(LOGDEBUG, "%s - loaded database channel '%s'",
            __FUNCTION__, channel->m_strChannelName.c_str());
        results.push_back(channel);
        m_pDS->next();
        ++iReturn;
      }
    }
    catch (...)
    {
      CLog::Log(LOGERROR, "%s - couldn't load channels from the database", __FUNCTION__);
    }
  }

  m_pDS->close();
  return iReturn;
}

int CTVDatabase::GetChannelCount(bool bRadio, bool bHidden /* = false */)
{
  int iReturn = -1;
  CStdString strQuery = FormatSQL("SELECT COUNT(1) FROM Channels WHERE IsRadio = %u AND IsHidden = %u",
      (bRadio ? 1 : 0), (bHidden ? 1 : 0));

  if (ResultQuery(strQuery))
  {
    iReturn = m_pDS->fv(0).get_asInt();
  }
  m_pDS->close();

  return iReturn;
}

int CTVDatabase::GetLastChannel()
{
  CStdString strValue = GetSingleValue("LastChannel", "ChannelId");

  if (strValue.IsEmpty())
    return -1;

  return atoi(strValue.c_str());
}

bool CTVDatabase::UpdateLastChannel(const CPVRChannel &info)
{
  if (info.ChannelID() < 0)
  {
    /* invalid channel */
    return false;
  }

  CStdString strQuery = FormatSQL("REPLACE INTO LastChannel (ChannelId, Number, Name) VALUES (%i, %i, '%s')",
      info.ChannelID(), info.ChannelNumber(), info.ChannelName().c_str());

  return ExecuteQuery(strQuery);
}


bool CTVDatabase::EraseChannelSettings()
{
  return DeleteValues("ChannelSettings");
}

bool CTVDatabase::GetChannelSettings(unsigned int iChannelId, CVideoSettings &settings)
{
  bool bReturn = false;
  CStdString strQuery = FormatSQL("SELECT * FROM ChannelSettings WHERE ChannelId = %u", iChannelId);

  if (ResultQuery(strQuery))
  {
    settings.m_AudioDelay           = m_pDS->fv("AudioDelay").get_asFloat();
    settings.m_AudioStream          = m_pDS->fv("AudioStream").get_asInt();
    settings.m_Brightness           = m_pDS->fv("Brightness").get_asFloat();
    settings.m_Contrast             = m_pDS->fv("Contrast").get_asFloat();
    settings.m_CustomPixelRatio     = m_pDS->fv("PixelRatio").get_asFloat();
    settings.m_NoiseReduction       = m_pDS->fv("NoiseReduction").get_asFloat();
    settings.m_Sharpness            = m_pDS->fv("Sharpness").get_asFloat();
    settings.m_CustomZoomAmount     = m_pDS->fv("CustomZoomAmount").get_asFloat();
    settings.m_Gamma                = m_pDS->fv("Gamma").get_asFloat();
    settings.m_SubtitleDelay        = m_pDS->fv("SubtitleDelay").get_asFloat();
    settings.m_SubtitleOn           = m_pDS->fv("SubtitlesOn").get_asBool();
    settings.m_SubtitleStream       = m_pDS->fv("SubtitleStream").get_asInt();
    settings.m_ViewMode             = m_pDS->fv("ViewMode").get_asInt();
    settings.m_Crop                 = m_pDS->fv("Crop").get_asBool();
    settings.m_CropLeft             = m_pDS->fv("CropLeft").get_asInt();
    settings.m_CropRight            = m_pDS->fv("CropRight").get_asInt();
    settings.m_CropTop              = m_pDS->fv("CropTop").get_asInt();
    settings.m_CropBottom           = m_pDS->fv("CropBottom").get_asInt();
    settings.m_InterlaceMethod      = (EINTERLACEMETHOD)m_pDS->fv("InterlaceMethod").get_asInt();
    settings.m_VolumeAmplification  = m_pDS->fv("VolumeAmplification").get_asFloat();
    settings.m_OutputToAllSpeakers  = m_pDS->fv("OutputToAllSpeakers").get_asBool();
    settings.m_SubtitleCached       = false;

    bReturn = true;
  }
  m_pDS->close();

  return bReturn;
}

bool CTVDatabase::SetChannelSettings(unsigned int iChannelId, const CVideoSettings &settings)
{
  CStdString strQuery = FormatSQL(
      "REPLACE INTO ChannelSettings "
        "(ChannelId, InterlaceMethod, ViewMode, CustomZoomAmount, PixelRatio, AudioStream, SubtitleStream, SubtitleDelay, "
         "SubtitlesOn, Brightness, Contrast, Gamma, VolumeAmplification, AudioDelay, OutputToAllSpeakers, Crop, CropLeft, "
         "CropRight, CropTop, CropBottom, Sharpness, NoiseReduction) VALUES "
         "(%i, %i, %i, %f, %f, %i, %i, %f, %i, %f, %f, %f, %f, %f, %i, %i, %i, %i, %i, %i, %f, %f)\n",
         iChannelId, settings.m_InterlaceMethod, settings.m_ViewMode, settings.m_CustomZoomAmount, settings.m_CustomPixelRatio,
       settings.m_AudioStream, settings.m_SubtitleStream, settings.m_SubtitleDelay, settings.m_SubtitleOn,
       settings.m_Brightness, settings.m_Contrast, settings.m_Gamma, settings.m_VolumeAmplification, settings.m_AudioDelay,
       settings.m_OutputToAllSpeakers, settings.m_Crop, settings.m_CropLeft, settings.m_CropRight, settings.m_CropTop,
       settings.m_CropBottom, settings.m_Sharpness, settings.m_NoiseReduction);

  return ExecuteQuery(strQuery);
}

/********** Channel group methods **********/

bool CTVDatabase::EraseChannelGroups(bool bRadio /* = false */)
{
  CStdString strWhereClause = FormatSQL("IsRadio = %u", (bRadio ? 1 : 0));
  return DeleteValues("ChannelGroup", strWhereClause);
}

long CTVDatabase::AddChannelGroup(const CStdString &strGroupName, int iSortOrder, bool bRadio /* = false */)
{
  long iReturn = -1;

  if (strGroupName.IsEmpty())
      return iReturn;

  iReturn = GetChannelGroupId(strGroupName, bRadio);
  if (iReturn < 0)
  {
    CStdString strQuery = FormatSQL("INSERT INTO ChannelGroup (GroupId, Name, SortOrder, IsRadio) VALUES (NULL, '%s', %i, %i)",
        strGroupName.c_str(), iSortOrder, (bRadio ? 1 : 0));

    if (ExecuteQuery(strQuery))
      iReturn = (long) m_pDS->lastinsertid();
  }

  return iReturn;
}

bool CTVDatabase::DeleteChannelGroup(int iGroupId, bool bRadio /* = false */)
{
  CStdString strWhereClause = FormatSQL("GroupId = %u AND IsRadio = %u", iGroupId, bRadio);
  return DeleteValues("ChannelGroup", strWhereClause);
}


bool CTVDatabase::GetChannelGroupList(CPVRChannelGroups &results, bool bRadio /* = false */)
{
  bool bReturn = false;
  CStdString strQuery = FormatSQL("SELECT * from ChannelGroup WHERE IsRadio = %u ORDER BY sortOrder", bRadio);
  int iNumRows = ResultQuery(strQuery);

  if (iNumRows > 0)
  {
    try
    {
      while (!m_pDS->eof())
      {
        CPVRChannelGroup data;

        data.SetGroupID(m_pDS->fv("GroupId").get_asInt());
        data.SetGroupName(m_pDS->fv("Name").get_asString());
        data.SetSortOrder(m_pDS->fv("SortOrder").get_asInt());

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

bool CTVDatabase::SetChannelGroupName(int iGroupId, const CStdString &strNewName, bool bRadio /* = false */)
{
  bool bReturn = false;

  if (iGroupId < 0)
    return bReturn;

  CStdString strQuery = FormatSQL("SELECT COUNT(1) FROM ChannelGroup WHERE GroupId = %u AND IsRadio = %u", iGroupId, (bRadio ? 1 : 0));
  if (ResultQuery(strQuery))
  {
    if (m_pDS->fv(0).get_asInt() > 0)
    {
      strQuery = FormatSQL("UPDATE ChannelGroup SET Name = '%s' WHERE GroupId = %i AND IsRadio = %u", strNewName.c_str(), iGroupId, (bRadio ? 1 : 0));
      bReturn = ExecuteQuery(strQuery);
    }
  }

  return bReturn;
}


bool CTVDatabase::SetChannelGroupSortOrder(int iGroupId, int iSortOrder, bool bRadio /* = false */)
{
  bool bReturn = false;

  if (iGroupId < 0)
    return bReturn;

  CStdString strQuery = FormatSQL("SELECT COUNT(1) FROM ChannelGroup WHERE GroupId = %u AND IsRadio = %u", iGroupId, (bRadio ? 1 : 0));
  if (ResultQuery(strQuery))
  {
    if (m_pDS->fv(0).get_asInt() > 0)
    {
      strQuery = FormatSQL("UPDATE ChannelGroup SET SortOrder = %i WHERE GroupId = %i AND IsRadio = %u", iSortOrder, iGroupId, (bRadio ? 1 : 0));
      bReturn = ExecuteQuery(strQuery);
    }
  }

  return bReturn;
}


long CTVDatabase::GetChannelGroupId(const CStdString &strGroupName, bool bRadio /* = false */)
{
  CStdString strWhereClause = FormatSQL("Name LIKE '%s' AND IsRadio = %u", strGroupName.c_str(), (bRadio ? 1 : 0));
  CStdString strReturn = GetSingleValue("ChannelGroup", "GroupId", strWhereClause);

  m_pDS->close();

  return atoi(strReturn);
}

/********** Client methods **********/

bool CTVDatabase::EraseClients()
{
  return DeleteValues("Clients") &&
      DeleteValues("LastChannel");
}

long CTVDatabase::AddClient(const CStdString &strClientName, const CStdString &strClientUid)
{
  long iReturn = -1;
  CStdString strQuery = FormatSQL("INSERT INTO Clients (ClientDbId, Name, ClientId) VALUES (NULL, '%s', '%s')\n",
      strClientName.c_str(), strClientUid.c_str());

  if (ExecuteQuery(strQuery))
  {
    iReturn = (long) m_pDS->lastinsertid();
  }

  return iReturn;
}

bool CTVDatabase::RemoveClient(const CStdString &strClientUid)
{
  CStdString strWhereClause = FormatSQL("ClientId = '%s'", strClientUid.c_str());
  return DeleteValues("Clients", strWhereClause);
}

long CTVDatabase::GetClientId(const CStdString &strClientUid)
{
  CStdString strWhereClause = FormatSQL("ClientId = '%s'", strClientUid.c_str());
  CStdString strValue = GetSingleValue("Clients", "ClientDbId", strWhereClause);

  if (strValue.IsEmpty())
    return -1;

  return atoi(strValue.c_str());
}

/********** EPG methods **********/

bool CTVDatabase::EraseEpg()
{
  return DeleteValues("EpgData");
}

bool CTVDatabase::EraseEpgForChannel(const CPVRChannel &channel, const CDateTime &start /* = NULL */, const CDateTime &end /* = NULL */)
{
  CStdString strWhereClause;
  strWhereClause = FormatSQL("ChannelId = %u", channel.ChannelID());

  if (start != NULL)
    strWhereClause.append(FormatSQL(" AND StartTime < %u", start.GetAsDBDateTime().c_str()).c_str());

  if (end != NULL)
    strWhereClause.append(FormatSQL(" AND EndTime > %u", end.GetAsDBDateTime().c_str()).c_str());

  return DeleteValues("EpgData", strWhereClause);
}

bool CTVDatabase::EraseOldEpgEntries()
{
  CDateTime yesterday = CDateTime::GetCurrentDateTime() - CDateTimeSpan(1, 0, 0, 0);
  CStdString strWhereClause = FormatSQL("EndTime < '%s'", yesterday.GetAsDBDateTime().c_str());

  return DeleteValues("EpgData", strWhereClause);
}

bool CTVDatabase::RemoveEpgEntry(const CPVREpgInfoTag &tag)
{
  CStdString strWhereClause;

  if (tag.BroadcastId() > 0)
    strWhereClause = FormatSQL("BroadcastId = %u", tag.BroadcastId());
  else
    strWhereClause = FormatSQL("BroadcastUid = %u", tag.UniqueBroadcastID());

  return DeleteValues("EpgData", strWhereClause);
}


int CTVDatabase::GetEpgForChannel(CPVREpg *epg, const CDateTime &start /* = NULL */, const CDateTime &end /* = NULL */)
{
  int iReturn = -1;
  CPVRChannel *channel = epg->Channel();
  if (!channel)
    return iReturn;

  CStdString strWhereClause;
  strWhereClause = FormatSQL("ChannelId = %u", channel->ChannelID());

  if (start != NULL)
    strWhereClause.append(FormatSQL(" AND StartTime < %u", start.GetAsDBDateTime().c_str()).c_str());

  if (end != NULL)
    strWhereClause.append(FormatSQL(" AND EndTime > %u", end.GetAsDBDateTime().c_str()).c_str());

  CStdString strQuery;
  strQuery.Format("SELECT * FROM EpgData WHERE %s ORDER BY StartTime ASC", strWhereClause.c_str());

  int iNumRows = ResultQuery(strQuery);

  if (iNumRows > 0)
  {
    try
    {
      while (!m_pDS->eof())
      {
        int iBroadcastUid = m_pDS->fv("BroadcastUid").get_asInt();

        CDateTime startTime, endTime, firstAired;
        startTime.SetFromDBDateTime (m_pDS->fv("StartTime").get_asString());
        endTime.SetFromDBDateTime   (m_pDS->fv("EndTime").get_asString());
        firstAired.SetFromDBDateTime(m_pDS->fv("FirstAired").get_asString());

        CPVREpgInfoTag newTag(iBroadcastUid);
        newTag.SetBroadcastId   (m_pDS->fv("BroadcastId").get_asInt());
        newTag.SetTitle         (m_pDS->fv("Title").get_asString().c_str());
        newTag.SetPlotOutline   (m_pDS->fv("PlotOutline").get_asString().c_str());
        newTag.SetPlot          (m_pDS->fv("Plot").get_asString().c_str());
        newTag.SetStart         (startTime);
        newTag.SetEnd           (endTime);
        newTag.SetGenre         (m_pDS->fv("GenreType").get_asInt(),
                                 m_pDS->fv("GenreSubType").get_asInt());
        newTag.SetFirstAired    (firstAired);
        newTag.SetParentalRating(m_pDS->fv("ParentalRating").get_asInt());
        newTag.SetStarRating    (m_pDS->fv("StarRating").get_asInt());
        newTag.SetNotify        (m_pDS->fv("Notify").get_asBool());
        newTag.SetEpisodeNum    (m_pDS->fv("EpisodeId").get_asString().c_str());
        newTag.SetEpisodePart   (m_pDS->fv("EpisodePart").get_asString().c_str());
        newTag.SetEpisodeName   (m_pDS->fv("EpisodeName").get_asString().c_str());

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

CDateTime CTVDatabase::GetEpgDataStart(int iChannelId)
{
  CDateTime firstProgramme;
  CStdString strWhereClause;

  if (iChannelId != -1)
    strWhereClause = FormatSQL("ChannelId = '%u'", iChannelId);

  CStdString strReturn = GetSingleValue("EpgData", "StartTime", strWhereClause, "StartTime ASC");
  if (!strReturn.IsEmpty())
    firstProgramme.SetFromDBDateTime(strReturn);

  if (!firstProgramme.IsValid())
    return CDateTime::GetCurrentDateTime();
  else
    return firstProgramme;
}

CDateTime CTVDatabase::GetEpgDataEnd(int iChannelId)
{
  CDateTime lastProgramme;
  CStdString strWhereClause;

  if (iChannelId != -1)
    strWhereClause = FormatSQL("ChannelId = '%u'", iChannelId);

  CStdString strReturn = GetSingleValue("EpgData", "EndTime", strWhereClause, "EndTime DESC");
  if (!strReturn.IsEmpty())
    lastProgramme.SetFromDBDateTime(strReturn);

  if (!lastProgramme.IsValid())
    return CDateTime::GetCurrentDateTime();
  else
    return lastProgramme;
}

CDateTime CTVDatabase::GetLastEpgScanTime()
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

bool CTVDatabase::UpdateLastEpgScanTime(void)
{
  CDateTime now = CDateTime::GetCurrentDateTime();
  CLog::Log(LOGDEBUG, "%s - updating last scan time to '%s'",
      __FUNCTION__, now.GetAsDBDateTime().c_str());
  lastScanTime = now;

  bool bReturn = true;
  CStdString strQuery = FormatSQL("REPLACE INTO LastEPGScan (ScanTime) VALUES ('%s')\n",
      now.GetAsDBDateTime().c_str());

  bReturn = ExecuteQuery(strQuery);

  return bReturn;
}

bool CTVDatabase::UpdateEpgEntry(const CPVREpgInfoTag &info, bool bSingleUpdate /* = true */, bool bLastUpdate /* = false */)
{
  int bReturn = false;

  if (info.UniqueBroadcastID() < 0)
    return bReturn;

  int iBroadcastId = info.BroadcastId();
  if (iBroadcastId)
  {
    CStdString strWhereClause = FormatSQL("(BroadcastUid = '%u' OR StartTime = '%s') AND ChannelId = '%u'",
        info.UniqueBroadcastID(), info.Start().GetAsDBDateTime().c_str(), info.ChannelTag()->ChannelID());
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
        info.ChannelTag()->ChannelID(), info.Start().GetAsDBDateTime().c_str(), info.End().GetAsDBDateTime().c_str(),
        info.Title().c_str(), info.PlotOutline().c_str(), info.Plot().c_str(), info.GenreType(), info.GenreSubType(),
        info.FirstAired().GetAsDBDateTime().c_str(), info.ParentalRating(), info.StarRating(), info.Notify(),
        info.SeriesNum().c_str(), info.EpisodeNum().c_str(), info.EpisodePart().c_str(), info.EpisodeName().c_str(),
        info.UniqueBroadcastID());
  }
  else
  {
    strQuery = FormatSQL("REPLACE INTO EpgData (ChannelId, StartTime, "
        "EndTime, Title, PlotOutline, Plot, GenreType, GenreSubType, "
        "FirstAired, ParentalRating, StarRating, Notify, SeriesId, "
        "EpisodeId, EpisodePart, EpisodeName, BroadcastUid, BroadcastId) "
        "VALUES (%i, '%s', '%s', '%s', '%s', '%s', %i, %i, '%s', %i, %i, %i, '%s', '%s', '%s', '%s', %i, %i)\n",
        info.ChannelTag()->ChannelID(), info.Start().GetAsDBDateTime().c_str(), info.End().GetAsDBDateTime().c_str(),
        info.Title().c_str(), info.PlotOutline().c_str(), info.Plot().c_str(), info.GenreType(), info.GenreSubType(),
        info.FirstAired().GetAsDBDateTime().c_str(), info.ParentalRating(), info.StarRating(), info.Notify(),
        info.SeriesNum().c_str(), info.EpisodeNum().c_str(), info.EpisodePart().c_str(), info.EpisodeName().c_str(),
        info.UniqueBroadcastID(), iBroadcastId);
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

  if ((bSingleUpdate || bLastUpdate) && GetEpgDataEnd(info.ChannelTag()->ChannelID()) > info.End())
    EraseEpgForChannel(*info.ChannelTag(), NULL, info.End());

  return bReturn;
}
