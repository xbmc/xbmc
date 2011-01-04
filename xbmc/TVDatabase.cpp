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

using namespace std;

using namespace dbiplus;

CTVDatabase::CTVDatabase(void)
{
  oneWriteSQLString = "";
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

    CLog::Log(LOGINFO, "TV-Database: Creating tables");

    CLog::Log(LOGINFO, "TV-Database: Creating Clients table");
    m_pDS->exec("CREATE TABLE Clients (idClient integer primary key, Name text, GUID text)\n");

    CLog::Log(LOGINFO, "TV-Database: Creating LastChannel table");
    m_pDS->exec("CREATE TABLE LastChannel (idChannel integer primary key, Number integer, Name text)\n");

    CLog::Log(LOGINFO, "TV-Database: Creating LastEPGScan table");
    m_pDS->exec("CREATE TABLE LastEPGScan (idScan integer primary key, ScanTime datetime)\n");

    CLog::Log(LOGINFO, "TV-Database: Creating GuideData table");
    m_pDS->exec("CREATE TABLE GuideData (idDatabaseBroadcast integer primary key, idUniqueBroadcast integer, idChannel integer, StartTime datetime, "
                "EndTime datetime, strTitle text, strPlotOutline text, strPlot text, GenreType integer, GenreSubType integer, "
                "firstAired datetime, parentalRating integer, starRating integer, notify integer, seriesNum text, "
                "episodeNum text, episodePart text, episodeName text)\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_GuideData on GuideData(idChannel, StartTime desc)\n"); /// pointless?

    CLog::Log(LOGINFO, "TV-Database: Creating Channels table");
    m_pDS->exec("CREATE TABLE Channels (idChannel integer primary key, Name text, Number integer, ClientName text, "
                "ClientNumber integer, idClient integer, UniqueId integer, IconPath text, GroupID integer, countWatched integer, "
                "timeWatched integer, lastTimeWatched datetime, encryption integer, radio bool, hide bool, grabEpg bool, EpgGrabber text, "
                "lastGrabTime datetime, Virtual bool, strInputFormat text, strStreamURL text)\n");

    CLog::Log(LOGINFO, "TV-Database: Creating ChannelLinkageMap table");
    m_pDS->exec("CREATE TABLE ChannelLinkageMap (idMapping integer primary key, idPortalChannel integer, idLinkedChannel integer)\n");

    CLog::Log(LOGINFO, "TV-Database: Creating ChannelGroup table");
    m_pDS->exec("CREATE TABLE ChannelGroup (idGroup integer primary key, groupName text, sortOrder integer)\n");

    CLog::Log(LOGINFO, "TV-Database: Creating RadioChannelGroup table");
    m_pDS->exec("CREATE TABLE RadioChannelGroup (idGroup integer primary key, groupName text, sortOrder integer)\n");

    CLog::Log(LOGINFO, "TV-Database: Creating ChannelSettings table");
    m_pDS->exec("CREATE TABLE ChannelSettings ( idChannel integer primary key, Deinterlace integer,"
                "ViewMode integer,ZoomAmount float, PixelRatio float, AudioStream integer, SubtitleStream integer,"
                "SubtitleDelay float, SubtitlesOn bool, Brightness float, Contrast float, Gamma float,"
                "VolumeAmplification float, AudioDelay float, OutputToAllSpeakers bool, Crop bool, CropLeft integer,"
                "CropRight integer, CropTop integer, CropBottom integer, Sharpness float, NoiseReduction float)\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_ChannelSettings ON ChannelSettings (idChannel)\n");
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
  BeginTransaction();

  try
  {

  }
  catch (...)
  {
    CLog::Log(LOGERROR, "Error attempting to update the database version!");
    RollbackTransaction();
    return false;
  }
  CommitTransaction();
  return true;
}

CDateTime CTVDatabase::GetLastEPGScanTime()
{
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    CStdString SQL=FormatSQL("select * from LastEPGScan");

    m_pDS->query(SQL.c_str());

    if (m_pDS->num_rows() > 0)
    {
      CDateTime lastTime;
      lastTime.SetFromDBDateTime(m_pDS->fv("ScanTime").get_asString());
      m_pDS->close();
      return lastTime;
    }
    else
    {
      m_pDS->close();
      return NULL;
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }

  return -1;
}

bool CTVDatabase::UpdateLastEPGScan(const CDateTime lastScan)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString SQL=FormatSQL("select * from LastEPGScan");
    m_pDS->query(SQL.c_str());

    if (m_pDS->num_rows() > 0)
    {
      m_pDS->close();
      // update the item
      CStdString SQL=FormatSQL("update LastEPGScan set ScanTime='%s'", lastScan.GetAsDBDateTime().c_str());

      m_pDS->exec(SQL.c_str());
      return true;
    }
    else   // add the items
    {
      m_pDS->close();
      SQL=FormatSQL("insert into LastEPGScan ( ScanTime ) values ('%s')\n", lastScan.GetAsDBDateTime().c_str());
      m_pDS->exec(SQL.c_str());
      return true;
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
    return false;
  }
}

int CTVDatabase::GetLastChannel()
{
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    CStdString SQL=FormatSQL("select * from LastChannel");

    m_pDS->query(SQL.c_str());

    if (m_pDS->num_rows() > 0)
    {
      int channelId = m_pDS->fv("idChannel").get_asInt();

      m_pDS->close();
      return channelId;
    }
    else
    {
      m_pDS->close();
      return -1;
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }

  return -1;
}

bool CTVDatabase::UpdateLastChannel(const cPVRChannelInfoTag &info)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    if (info.ChannelID() < 0)   // no match found, update required
    {
      return false;
    }

    CStdString SQL;

    SQL=FormatSQL("select * from LastChannel");
    m_pDS->query(SQL.c_str());

    if (m_pDS->num_rows() > 0)
    {
      m_pDS->close();
      // update the item
      CStdString SQL=FormatSQL("update LastChannel set Number=%i,Name='%s',idChannel=%i", info.Number(), info.Name().c_str(), info.ChannelID());

      m_pDS->exec(SQL.c_str());
      return true;
    }
    else   // add the items
    {
      m_pDS->close();
      SQL=FormatSQL("insert into LastChannel ( idChannel,Number,Name ) values ('%i','%i','%s')\n", info.ChannelID(), info.Number(), info.Name().c_str());
      m_pDS->exec(SQL.c_str());
      return true;
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%li) failed", __FUNCTION__, info.ChannelID());
    return false;
  }
}

bool CTVDatabase::EraseClients()
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL=FormatSQL("delete from Clients");
    m_pDS->exec(strSQL.c_str());
    strSQL=FormatSQL("delete from LastChannel");
    m_pDS->exec(strSQL.c_str());
    strSQL=FormatSQL("delete from LastEPGScan");
    m_pDS->exec(strSQL.c_str());
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
    return false;
  }
}

long CTVDatabase::AddClient(const CStdString &client, const CStdString &guid)
{
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    long clientId = GetClientId(guid);
    if (clientId < 0)
    {
      CStdString SQL=FormatSQL("insert into Clients (idClient, Name, GUID) values (NULL, '%s', '%s')\n", client.c_str(), guid.c_str());
      m_pDS->exec(SQL.c_str());
      clientId = (long)m_pDS->lastinsertid();

      CLog::Log(LOGNOTICE, "TVDatabase: Added new PVR Client ID '%li' with Name '%s' and GUID '%s'", clientId, client.c_str(), guid.c_str());
    }

    return clientId;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (Name: %s, GUID: %s) failed", __FUNCTION__, client.c_str(), guid.c_str());
  }

  return -1;
}

long CTVDatabase::GetClientId(const CStdString& guid)
{
  CStdString SQL;

  try
  {
    long clientId = -1;

    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    SQL = FormatSQL("select idClient from Clients where GUID like '%s'", guid.c_str());

    m_pDS->query(SQL.c_str());

    if (!m_pDS->eof())
      clientId = m_pDS->fv("Clients.idClient").get_asInt();

    m_pDS->close();

    return clientId;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s unable to get ClientId (%s)", __FUNCTION__, SQL.c_str());
  }

  return -1;
}

bool CTVDatabase::EraseEPG()
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL=FormatSQL("delete from GuideData");

    m_pDS->exec(strSQL.c_str());
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
    return false;
  }
}

bool CTVDatabase::EraseChannelEPG(long channelID)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL=FormatSQL("delete from GuideData WHERE GuideData.idChannel = '%u'", channelID);

    m_pDS->exec(strSQL.c_str());

    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
    return false;
  }
}

bool CTVDatabase::EraseChannelEPGAfterTime(long channelID, CDateTime after)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL=FormatSQL("delete from GuideData WHERE GuideData.idChannel = '%u' AND GuideData.EndTime > '%s'", channelID, after.GetAsDBDateTime().c_str());

    m_pDS->exec(strSQL.c_str());

    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
    return false;
  }
}

bool CTVDatabase::EraseOldEPG()
{
  //delete programs from database that are more than 1 day old
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CDateTime yesterday = CDateTime::GetCurrentDateTime()-CDateTimeSpan(1, 0, 0, 0);
    CStdString strSQL = FormatSQL("DELETE FROM GuideData WHERE EndTime < '%s'", yesterday.GetAsDBDateTime().c_str());
    m_pDS->exec(strSQL.c_str());
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
    return false;
  }
}

long CTVDatabase::AddEPGEntry(const cPVREPGInfoTag &info, bool oneWrite, bool firstWrite, bool lastWrite)
{
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    if (!oneWrite && firstWrite)
      m_pDS->insert();

    if (info.GetUniqueBroadcastID() > 0)
    {
      CStdString SQL = FormatSQL("INSERT INTO GuideData (idDatabaseBroadcast, idChannel, StartTime, "
                                 "EndTime, strTitle, strPlotOutline, strPlot, GenreType, GenreSubType, "
                                 "firstAired, parentalRating, starRating, notify, seriesNum, "
                                 "episodeNum, episodePart, episodeName, idUniqueBroadcast) "
                                 "VALUES (NULL,'%i','%s','%s','%s','%s','%s','%i','%i','%s','%i','%i','%i','%s','%s','%s','%s','%i')\n",
                                 info.ChannelID(), info.Start().GetAsDBDateTime().c_str(), info.End().GetAsDBDateTime().c_str(),
                                 info.Title().c_str(), info.PlotOutline().c_str(), info.Plot().c_str(), info.GenreType(), info.GenreSubType(),
                                 info.FirstAired().GetAsDBDateTime().c_str(), info.ParentalRating(), info.StarRating(), info.Notify(),
                                 info.SeriesNum().c_str(), info.EpisodeNum().c_str(), info.EpisodePart().c_str(), info.EpisodeName().c_str(),
                                 info.GetUniqueBroadcastID());

      if (oneWrite)
      {
        m_pDS->exec(SQL.c_str());
        return (long)m_pDS->lastinsertid();
      }
      else
        m_pDS->add_insert_sql(SQL);
    }

    if (!oneWrite && lastWrite)
    {
      m_pDS->post();
      m_pDS->clear_insert_sql();
    }
    return 0;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, info.Title().c_str());
  }

  return -1;
}

bool CTVDatabase::UpdateEPGEntry(const cPVREPGInfoTag &info, bool oneWrite, bool firstWrite, bool lastWrite)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;
    if (NULL == m_pDS2.get()) return false;

    if (!oneWrite && firstWrite)
      m_pDS2->insert();

    if (info.GetUniqueBroadcastID() > 0)
    {
      CStdString SQL = FormatSQL("select idDatabaseBroadcast from GuideData WHERE (GuideData.idUniqueBroadcast = '%u' OR GuideData.StartTime = '%s') AND GuideData.idChannel = '%u'", info.GetUniqueBroadcastID(), info.Start().GetAsDBDateTime().c_str(), info.ChannelID());
      m_pDS->query(SQL.c_str());

      if (m_pDS->num_rows() > 0)
      {
        int id = m_pDS->fv("idDatabaseBroadcast").get_asInt();
        m_pDS->close();
        // update the item
        SQL = FormatSQL("update GuideData set idChannel=%i, StartTime='%s', EndTime='%s', strTitle='%s', strPlotOutline='%s', "
                        "strPlot='%s', GenreType=%i, GenreSubType=%i, firstAired='%s', parentalRating=%i, starRating=%i, "
                        "notify=%i, seriesNum='%s', episodeNum='%s', episodePart='%s', episodeName='%s', "
                        "idUniqueBroadcast=%i WHERE idDatabaseBroadcast=%i",
                        info.ChannelID(), info.Start().GetAsDBDateTime().c_str(), info.End().GetAsDBDateTime().c_str(),
                        info.Title().c_str(), info.PlotOutline().c_str(), info.Plot().c_str(), info.GenreType(), info.GenreSubType(),
                        info.FirstAired().GetAsDBDateTime().c_str(), info.ParentalRating(), info.StarRating(), info.Notify(),
                        info.SeriesNum().c_str(), info.EpisodeNum().c_str(), info.EpisodePart().c_str(), info.EpisodeName().c_str(),
                        info.GetUniqueBroadcastID(), id);

        if (oneWrite)
          m_pDS->exec(SQL.c_str());
        else
          m_pDS2->add_insert_sql(SQL);

        return true;
      }
      else   // add the items
      {
        m_pDS->close();
        SQL = FormatSQL("insert into GuideData (idDatabaseBroadcast, idChannel, StartTime, "
                        "EndTime, strTitle, strPlotOutline, strPlot, GenreType, GenreSubType, "
                        "firstAired, parentalRating, starRating, notify, seriesNum, "
                        "episodeNum, episodePart, episodeName, idUniqueBroadcast) "
                        "values (NULL,'%i','%s','%s','%s','%s','%s','%i','%i','%s','%i','%i','%i','%s','%s','%s','%s','%i')\n",
                        info.ChannelID(), info.Start().GetAsDBDateTime().c_str(), info.End().GetAsDBDateTime().c_str(),
                        info.Title().c_str(), info.PlotOutline().c_str(), info.Plot().c_str(), info.GenreType(), info.GenreSubType(),
                        info.FirstAired().GetAsDBDateTime().c_str(), info.ParentalRating(), info.StarRating(), info.Notify(),
                        info.SeriesNum().c_str(), info.EpisodeNum().c_str(), info.EpisodePart().c_str(), info.EpisodeName().c_str(),
                        info.GetUniqueBroadcastID());

        if (oneWrite)
        {
          m_pDS->exec(SQL.c_str());
        }
        else
        {
          if (firstWrite)
            m_pDS2->insert();

          m_pDS2->add_insert_sql(SQL);
        }

        if (GetEPGDataEnd(info.ChannelID()) > info.End())
        {
          CLog::Log(LOGNOTICE, "TV-Database: erasing epg data due to event change on channel %s", info.ChannelName().c_str());
          EraseChannelEPGAfterTime(info.ChannelID(), info.End());
        }
      }
    }

    if (!oneWrite && lastWrite)
    {
      m_pDS2->post();
      m_pDS2->clear_insert_sql();
    }
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s on Channel %s) failed", __FUNCTION__, info.Title().c_str(), info.ChannelName().c_str());
  }
  return false;
}

bool CTVDatabase::RemoveEPGEntry(const cPVREPGInfoTag &info)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;


    if (info.GetUniqueBroadcastID() < 0)   // no match found, update required
      return false;

    CStdString strSQL=FormatSQL("delete from GuideData WHERE GuideData.idUniqueBroadcast = '%u'", info.GetUniqueBroadcastID() );

    m_pDS->exec(strSQL.c_str());
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, info.Title().c_str());
    return false;
  }
}

bool CTVDatabase::RemoveEPGEntries(unsigned int channelID, const CDateTime &start, const CDateTime &end)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL;
    if (channelID < 0)
    {
      strSQL=FormatSQL("delete from GuideData WHERE GuideData.StartTime < '%s' AND GuideData.EndTime > '%s'",
                       start.GetAsDBDateTime().c_str(), end.GetAsDBDateTime().c_str());
    }
    else
    {
      strSQL=FormatSQL("delete from GuideData WHERE GuideData.idChannel = '%u' AND GuideData.StartTime < '%s' AND GuideData.EndTime > '%s'",
                       channelID, start.GetAsDBDateTime().c_str(), end.GetAsDBDateTime().c_str());
    }
    m_pDS->exec(strSQL.c_str());
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (ID=%i) failed", __FUNCTION__, channelID);
    return false;
  }
}

bool CTVDatabase::GetEPGForChannel(const cPVRChannelInfoTag &channelinfo, cPVREpg *epg, const CDateTime &start, const CDateTime &end)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString SQL=FormatSQL("select * from GuideData WHERE GuideData.idChannel = '%u' AND GuideData.EndTime > '%s' "
                             "AND GuideData.StartTime < '%s' ORDER BY GuideData.StartTime;", channelinfo.ChannelID(), start.GetAsDBDateTime().c_str(), end.GetAsDBDateTime().c_str());
    m_pDS->query( SQL.c_str() );

    if (m_pDS->num_rows() <= 0)
      return false;

    while (!m_pDS->eof())
    {
      PVR_PROGINFO broadcast;
      CDateTime startTime, endTime;
      time_t startTime_t, endTime_t;
      broadcast.uid             = m_pDS->fv("idUniqueBroadcast").get_asInt();
      broadcast.title           = m_pDS->fv("strTitle").get_asString().c_str();
      broadcast.subtitle        = m_pDS->fv("strPlotOutline").get_asString().c_str();
      broadcast.description     = m_pDS->fv("strPlot").get_asString().c_str();
      broadcast.genre_type      = m_pDS->fv("GenreType").get_asInt();
      broadcast.genre_sub_type  = m_pDS->fv("GenreSubType").get_asInt();
      broadcast.parental_rating = m_pDS->fv("parentalRating").get_asInt();
      startTime.SetFromDBDateTime(m_pDS->fv("StartTime").get_asString());
      endTime.SetFromDBDateTime(m_pDS->fv("EndTime").get_asString());
      startTime.GetAsTime(startTime_t);
      endTime.GetAsTime(endTime_t);
      broadcast.starttime = startTime_t;
      broadcast.endtime = endTime_t;

      cPVREpg::Add(&broadcast, epg);

      m_pDS->next();
    }
    m_pDS->close();
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
    return false;
  }
  return false;
}

CDateTime CTVDatabase::GetEPGDataStart(int channelID)
{
  CDateTime lastProgramme;
  try
  {
    CStdString SQL;

    if (NULL == m_pDB.get()) return lastProgramme;
    if (NULL == m_pDS.get()) return lastProgramme;

    if (channelID != -1)
    {
      SQL=FormatSQL("SELECT GuideData.StartTime FROM GuideData WHERE GuideData.idChannel = '%u' ORDER BY Guidedata.StartTime DESC;", channelID);
    }
    else {
      SQL=FormatSQL("SELECT GuideData.StartTime FROM GuideData ORDER BY Guidedata.StartTime DESC;");
    }
    m_pDS->query( SQL.c_str() );
    if (!m_pDS->eof())
    {
      lastProgramme.SetFromDBDateTime(m_pDS->fv("StartTime").get_asString());
    }
    m_pDS->close();

    if (!lastProgramme.IsValid())
      return CDateTime::GetCurrentDateTime();
    else
      return lastProgramme;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
    return lastProgramme;
  }
}

CDateTime CTVDatabase::GetEPGDataEnd(int channelID)
{
  CDateTime lastProgramme;
  try
  {
    CStdString SQL;

    if (NULL == m_pDB.get()) return lastProgramme;
    if (NULL == m_pDS.get()) return lastProgramme;

    if (channelID != -1)
    {
      SQL=FormatSQL("SELECT GuideData.EndTime FROM GuideData WHERE GuideData.idChannel = '%u' ORDER BY Guidedata.EndTime DESC;", channelID);
    }
    else
    {
      SQL=FormatSQL("SELECT GuideData.EndTime FROM GuideData ORDER BY Guidedata.EndTime DESC;");
    }
    m_pDS->query( SQL.c_str() );
    if (!m_pDS->eof())
    {
      lastProgramme.SetFromDBDateTime(m_pDS->fv("EndTime").get_asString());
    }
    m_pDS->close();

    if (!lastProgramme.IsValid())
      return CDateTime::GetCurrentDateTime();
    else
      return lastProgramme;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
    return lastProgramme;
  }
}

bool CTVDatabase::EraseChannels()
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL=FormatSQL("delete from Channels");

    m_pDS->exec(strSQL.c_str());
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
    return false;
  }
}

bool CTVDatabase::EraseClientChannels(long clientID)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL=FormatSQL("delete from Channels WHERE Channels.idClient = '%u'", clientID);

    m_pDS->exec(strSQL.c_str());

    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
    return false;
  }
}

long CTVDatabase::AddDBChannel(const cPVRChannelInfoTag &info, bool oneWrite, bool firstWrite, bool lastWrite)
{
  try
  {
    if (info.ClientID() < 0) return -1;
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    long channelId = info.ChannelID();
    if (channelId < 0)
    {
      CStdString SQL = FormatSQL("insert into Channels (idChannel, idClient, Number, Name, ClientName, "
                                 "ClientNumber, UniqueId, IconPath, GroupID, encryption, radio, "
                                 "grabEpg, EpgGrabber, hide, Virtual, strInputFormat, strStreamURL) "
                                 "values (NULL, '%i', '%i', '%s', '%s', '%i', '%i', '%s', '%i', '%i', '%i', '%i', '%s', '%i', '%i', '%s', '%s')\n",
                                 info.ClientID(), info.Number(), info.Name().c_str(), info.ClientName().c_str(),
                                 info.ClientNumber(), info.UniqueID(), info.m_IconPath.c_str(), info.m_iGroupID,
                                 info.EncryptionSystem(), info.m_radio, info.m_grabEpg, info.m_grabber.c_str(),
                                 info.m_hide, info.m_bIsVirtual, info.m_strInputFormat.c_str(), info.m_strStreamURL.c_str());

      if (oneWrite)
      {
        m_pDS->exec(SQL.c_str());
        channelId = (long)m_pDS->lastinsertid();
      }
      else
      {
        if (firstWrite)
          m_pDS->insert();

        m_pDS->add_insert_sql(SQL);
        if (lastWrite)
          m_pDS->post();
      }
    }
    return channelId;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, info.m_strChannel.c_str());
  }

  return -1;
}

bool CTVDatabase::RemoveDBChannel(const cPVRChannelInfoTag &info)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    long channelId = info.ChannelID();
    long clientId = info.ClientID();

    if (channelId < 0 || clientId < 0)   // no match found, update required
    {
      return false;
    }

    CStdString strSQL=FormatSQL("delete from Channels WHERE Channels.idChannel = '%u' AND Channels.idClient = '%u'", channelId, clientId);

    m_pDS->exec(strSQL.c_str());
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, info.m_strChannel.c_str());
    return false;
  }
}

long CTVDatabase::UpdateDBChannel(const cPVRChannelInfoTag &info)
{
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    long channelId = info.m_iIdChannel;

    CStdString SQL;
    SQL=FormatSQL("select * from Channels WHERE Channels.idChannel = '%u'", channelId);
    m_pDS->query(SQL.c_str());

    if (m_pDS->num_rows() > 0)
    {
      m_pDS->close();
      // update the item
      SQL = FormatSQL("update Channels set idClient=%i,Number=%i,Name='%s',ClientName='%s',"
                      "ClientNumber=%i,UniqueId=%i,IconPath='%s',GroupID=%i,encryption=%i,radio=%i,"
                      "hide=%i,grabEpg=%i,EpgGrabber='%s',Virtual=%i,strInputFormat='%s',strStreamURL='%s' where idChannel=%i",
                      info.ClientID(), info.Number(), info.Name().c_str(), info.ClientName().c_str(),
                      info.ClientNumber(), info.UniqueID(), info.m_IconPath.c_str(), info.m_iGroupID,
                      info.EncryptionSystem(), info.m_radio, info.m_hide, info.m_grabEpg, info.m_grabber.c_str(),
                      info.m_bIsVirtual, info.m_strInputFormat.c_str(), info.m_strStreamURL.c_str(), channelId);

      m_pDS->exec(SQL.c_str());
      return channelId;
    }
    else   // add the items
    {
      m_pDS->close();
      SQL = FormatSQL("insert into Channels (idChannel, idClient, Number, Name, ClientName, "
                      "ClientNumber, UniqueId, IconPath, GroupID, encryption, radio, "
                      "grabEpg, EpgGrabber, hide, Virtual, strInputFormat, strStreamURL) "
                      "values (NULL, '%i', '%i', '%s', '%s', '%i', '%i', '%s', '%i', '%i', '%i', '%i', '%s', '%i', '%i', '%s', '%s')\n",
                      info.ClientID(), info.Number(), info.Name().c_str(), info.ClientName().c_str(),
                      info.ClientNumber(), info.UniqueID(), info.m_IconPath.c_str(), info.m_iGroupID,
                      info.EncryptionSystem(), info.m_radio, info.m_grabEpg, info.m_grabber.c_str(),
                      info.m_hide, info.m_bIsVirtual, info.m_strInputFormat.c_str(), info.m_strStreamURL.c_str());

      m_pDS->exec(SQL.c_str());
      channelId = (long)m_pDS->lastinsertid();
      return channelId;
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, info.m_strChannel.c_str());
    return false;
  }
}

bool CTVDatabase::HasChannel(const cPVRChannelInfoTag &info)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString SQL=FormatSQL("select * from Channels WHERE Channels.Name = '%s' AND Channels.ClientNumber = '%i'", info.m_strChannel.c_str(), info.m_iClientNum);

    m_pDS->query(SQL.c_str());

    int num = 0;

    num = m_pDS->num_rows();

    m_pDS->close();

    if (num != 0)
      return true;
    else
      return false;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
    return false;
  }
}

int CTVDatabase::GetDBNumChannels(bool radio)
{
  try
  {
    if (NULL == m_pDB.get()) return 0;
    if (NULL == m_pDS.get()) return 0;

    CStdString SQL=FormatSQL("select * from Channels WHERE Channels.radio=%u", radio);

    m_pDS->query(SQL.c_str());

    int num = m_pDS->num_rows();

    m_pDS->close();

    return num;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
    return 0;
  }
}

int CTVDatabase::GetNumHiddenChannels()
{
  try
  {
    if (NULL == m_pDB.get()) return 0;
    if (NULL == m_pDS.get()) return 0;

    CStdString SQL=FormatSQL("select * from Channels WHERE Channels.hide=%u", true);

    m_pDS->query(SQL.c_str());

    int num = m_pDS->num_rows();

    m_pDS->close();

    return num;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
    return 0;
  }
}

bool CTVDatabase::GetDBChannelList(cPVRChannels &results, bool radio)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString SQL=FormatSQL("select * from Channels WHERE Channels.radio=%u ORDER BY Channels.Number", radio);

    m_pDS->query(SQL.c_str());

    while (!m_pDS->eof())
    {
      cPVRChannelInfoTag channel;

      channel.m_clientID            = m_pDS->fv("idClient").get_asInt();
      channel.m_iIdChannel          = m_pDS->fv("idChannel").get_asInt();
      channel.m_iChannelNum         = m_pDS->fv("Number").get_asInt();
      channel.m_strChannel          = m_pDS->fv("Name").get_asString();
      channel.m_strClientName       = m_pDS->fv("ClientName").get_asString();
      channel.m_iClientNum          = m_pDS->fv("ClientNumber").get_asInt();
      channel.m_iIdUnique           = m_pDS->fv("UniqueId").get_asInt();
      channel.m_IconPath            = m_pDS->fv("IconPath").get_asString();
      channel.m_iGroupID            = m_pDS->fv("GroupID").get_asInt();
      channel.m_encryptionSystem    = m_pDS->fv("encryption").get_asInt();
      channel.m_radio               = m_pDS->fv("radio").get_asBool();
      channel.m_hide                = m_pDS->fv("hide").get_asBool();
      channel.m_grabEpg             = m_pDS->fv("grabEpg").get_asBool();
      channel.m_grabber             = m_pDS->fv("EpgGrabber").get_asString();
      channel.m_strInputFormat      = m_pDS->fv("strInputFormat").get_asString();
      channel.m_strStreamURL        = m_pDS->fv("strStreamURL").get_asString();
      channel.m_countWatched        = m_pDS->fv("countWatched").get_asInt();
      channel.m_secondsWatched      = m_pDS->fv("timeWatched").get_asInt();
      channel.m_bIsVirtual          = m_pDS->fv("Virtual").get_asBool();
      channel.m_lastTimeWatched.SetFromDBDateTime(m_pDS->fv("lastTimeWatched").get_asString());

      results.push_back(channel);
      m_pDS->next();
    }

    m_pDS->close();

    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
    return false;
  }

  return false;
}

bool CTVDatabase::EraseChannelLinkageMap()
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL=FormatSQL("delete from ChannelLinkageMap");

    m_pDS->exec(strSQL.c_str());
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
    return false;
  }
}

long CTVDatabase::AddChannelLinkage(int PortalChannel, int LinkedChannel)
{
  try
  {
    long idMapping;

    if (PortalChannel < 0)    return -1;
    if (LinkedChannel < 0)    return -1;
    if (NULL == m_pDB.get())  return -1;
    if (NULL == m_pDS.get())  return -1;

    CStdString SQL=FormatSQL("insert into ChannelLinkageMap (idMapping, idPortalChannel, idLinkedChannel) values (NULL, '%i', '%i')", PortalChannel, LinkedChannel);
    m_pDS->exec(SQL.c_str());
    idMapping = (long)m_pDS->lastinsertid();

    return idMapping;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%i-%i) failed", __FUNCTION__, PortalChannel, LinkedChannel);
  }
  return -1;
}

bool CTVDatabase::DeleteChannelLinkage(unsigned int channelId)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    if (channelId < 0)   // no match found, update required
      return false;

    CStdString strSQL=FormatSQL("delete from ChannelLinkageMap WHERE idPortalChannel='%u' OR idLinkedChannel='%u'", channelId, channelId);

    m_pDS->exec(strSQL.c_str());
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%i) failed", __FUNCTION__, channelId);
    return false;
  }
}

bool CTVDatabase::GetChannelLinkageMap(cPVRChannelInfoTag &channel)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    channel.ClearChannelLinkage();

    CStdString SQL = FormatSQL("select * from ChannelLinkageMap WHERE ChannelLinkageMap.idPortalChannel=%u ORDER BY ChannelLinkageMap.idLinkedChannel", channel.ChannelID());
    m_pDS->query(SQL.c_str());

    while (!m_pDS->eof())
    {
      channel.AddChannelLinkage(m_pDS->fv("idLinkedChannel").get_asInt());
      m_pDS->next();
    }

    m_pDS->close();
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
    return false;
  }
  return false;
}

bool CTVDatabase::EraseChannelGroups()
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL=FormatSQL("delete from ChannelGroup");

    m_pDS->exec(strSQL.c_str());
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
    return false;
  }
}

long CTVDatabase::AddChannelGroup(const CStdString &groupName, int sortOrder)
{
  try
  {
    if (groupName == "")      return -1;
    if (NULL == m_pDB.get())  return -1;
    if (NULL == m_pDS.get())  return -1;

    long groupId;
    groupId = GetChannelGroupId(groupName);
    if (groupId < 0)
    {
      CStdString SQL=FormatSQL("insert into ChannelGroup (idGroup, groupName, sortOrder) values (NULL, '%s', '%i')", groupName.c_str(), sortOrder);
      m_pDS->exec(SQL.c_str());
      groupId = (long)m_pDS->lastinsertid();
    }

    return groupId;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, groupName.c_str());
  }
  return -1;
}

bool CTVDatabase::DeleteChannelGroup(unsigned int GroupId)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    if (GroupId < 0)   // no match found, update required
      return false;

    CStdString strSQL=FormatSQL("delete from ChannelGroup WHERE ChannelGroup.idGroup = '%u'", GroupId);

    m_pDS->exec(strSQL.c_str());
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%i) failed", __FUNCTION__, GroupId);
    return false;
  }
}

bool CTVDatabase::GetChannelGroupList(cPVRChannelGroups &results)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString SQL = FormatSQL("select * from ChannelGroup ORDER BY ChannelGroup.sortOrder");
    m_pDS->query(SQL.c_str());

    while (!m_pDS->eof())
    {
      cPVRChannelGroup data;

      data.SetGroupID(m_pDS->fv("idGroup").get_asInt());
      data.SetGroupName(m_pDS->fv("groupName").get_asString());
      data.SetSortOrder(m_pDS->fv("sortOrder").get_asInt());

      results.push_back(data);
      m_pDS->next();
    }

    m_pDS->close();
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
    return false;
  }
  return false;
}

bool CTVDatabase::SetChannelGroupName(unsigned int GroupId, const CStdString &newname)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    if (GroupId < 0)   // no match found, update required
    {
      return false;
    }

    CStdString SQL;

    SQL=FormatSQL("select * from ChannelGroup WHERE ChannelGroup.idGroup = '%u'", GroupId);
    m_pDS->query(SQL.c_str());

    if (m_pDS->num_rows() > 0)
    {
      m_pDS->close();
      // update the item
      CStdString SQL = FormatSQL("update ChannelGroup set groupName='%s' WHERE idGroup=%i", newname.c_str(), GroupId);
      m_pDS->exec(SQL.c_str());
      return true;
    }
    return false;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%i) failed", __FUNCTION__, GroupId);
    return false;
  }
}

bool CTVDatabase::SetChannelGroupSortOrder(unsigned int GroupId, int sortOrder)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    if (GroupId < 0)   // no match found, update required
      return false;

    CStdString SQL;

    SQL=FormatSQL("select * from ChannelGroup WHERE ChannelGroup.idGroup = '%u'", GroupId);
    m_pDS->query(SQL.c_str());

    if (m_pDS->num_rows() > 0)
    {
      m_pDS->close();
      // update the item
      CStdString SQL = FormatSQL("update ChannelGroup set sortOrder='%i' WHERE idGroup=%i", sortOrder, GroupId);
      m_pDS->exec(SQL.c_str());
      return true;
    }
    return false;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%i) failed", __FUNCTION__, GroupId);
  }
  return -1;
}

long CTVDatabase::GetChannelGroupId(const CStdString &groupname)
{
  CStdString SQL;

  try
  {
    long lGroupId = -1;
    if (NULL == m_pDB.get()) return lGroupId;
    if (NULL == m_pDS.get()) return lGroupId;

    SQL=FormatSQL("select idGroup from ChannelGroup where groupName like '%s'", groupname.c_str());
    m_pDS->query(SQL.c_str());
    if (!m_pDS->eof())
      lGroupId = m_pDS->fv("ChannelGroup.idGroup").get_asInt();

    m_pDS->close();
    return lGroupId;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s unable to get GroupId (%s)", __FUNCTION__, SQL.c_str());
  }
  return -1;
}

bool CTVDatabase::EraseRadioChannelGroups()
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL=FormatSQL("delete from RadioChannelGroup");

    m_pDS->exec(strSQL.c_str());
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
    return false;
  }
}

long CTVDatabase::AddRadioChannelGroup(const CStdString &groupName, int sortOrder)
{
  try
  {
    if (groupName == "")      return -1;
    if (NULL == m_pDB.get())  return -1;
    if (NULL == m_pDS.get())  return -1;

    long groupId;
    groupId = GetRadioChannelGroupId(groupName);
    if (groupId < 0)
    {
      CStdString SQL=FormatSQL("insert into RadioChannelGroup (idGroup, groupName, sortOrder) values (NULL, '%s', '%i')", groupName.c_str(), sortOrder);
      m_pDS->exec(SQL.c_str());
      groupId = (long)m_pDS->lastinsertid();
    }

    return groupId;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, groupName.c_str());
  }
  return -1;
}

bool CTVDatabase::DeleteRadioChannelGroup(unsigned int GroupId)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    if (GroupId < 0)   // no match found, update required
      return false;

    CStdString strSQL=FormatSQL("delete from RadioChannelGroup WHERE RadioChannelGroup.idGroup = '%u'", GroupId);

    m_pDS->exec(strSQL.c_str());
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%i) failed", __FUNCTION__, GroupId);
    return false;
  }
}

bool CTVDatabase::GetRadioChannelGroupList(cPVRChannelGroups &results)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString SQL = FormatSQL("select * from RadioChannelGroup ORDER BY RadioChannelGroup.sortOrder");
    m_pDS->query(SQL.c_str());

    while (!m_pDS->eof())
    {
      cPVRChannelGroup data;

      data.SetGroupID(m_pDS->fv("idGroup").get_asInt());
      data.SetGroupName(m_pDS->fv("groupName").get_asString());
      data.SetSortOrder(m_pDS->fv("sortOrder").get_asInt());

      results.push_back(data);
      m_pDS->next();
    }

    m_pDS->close();
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
    return false;
  }
  return false;
}

bool CTVDatabase::SetRadioChannelGroupName(unsigned int GroupId, const CStdString &newname)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    if (GroupId < 0)   // no match found, update required
    {
      return false;
    }

    CStdString SQL;

    SQL=FormatSQL("select * from RadioChannelGroup WHERE RadioChannelGroup.idGroup = '%u'", GroupId);
    m_pDS->query(SQL.c_str());

    if (m_pDS->num_rows() > 0)
    {
      m_pDS->close();
      // update the item
      CStdString SQL = FormatSQL("update RadioChannelGroup set groupName='%s' WHERE idGroup=%i", newname.c_str(), GroupId);
      m_pDS->exec(SQL.c_str());
      return true;
    }
    return false;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%i) failed", __FUNCTION__, GroupId);
    return false;
  }
}

bool CTVDatabase::SetRadioChannelGroupSortOrder(unsigned int GroupId, int sortOrder)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    if (GroupId < 0)   // no match found, update required
      return false;

    CStdString SQL;

    SQL=FormatSQL("select * from RadioChannelGroup WHERE RadioChannelGroup.idGroup = '%u'", GroupId);
    m_pDS->query(SQL.c_str());

    if (m_pDS->num_rows() > 0)
    {
      m_pDS->close();
      // update the item
      CStdString SQL = FormatSQL("update RadioChannelGroup set sortOrder='%i' WHERE idGroup=%i", sortOrder, GroupId);
      m_pDS->exec(SQL.c_str());
      return true;
    }
    return false;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%i) failed", __FUNCTION__, GroupId);
  }
  return -1;
}

long CTVDatabase::GetRadioChannelGroupId(const CStdString &groupname)
{
  CStdString SQL;

  try
  {
    long lGroupId = -1;
    if (NULL == m_pDB.get()) return lGroupId;
    if (NULL == m_pDS.get()) return lGroupId;

    SQL=FormatSQL("select idGroup from RadioChannelGroup where groupName like '%s'", groupname.c_str());
    m_pDS->query(SQL.c_str());
    if (!m_pDS->eof())
      lGroupId = m_pDS->fv("RadioChannelGroup.idGroup").get_asInt();

    m_pDS->close();
    return lGroupId;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s unable to get GroupId (%s)", __FUNCTION__, SQL.c_str());
  }
  return -1;
}

bool CTVDatabase::EraseChannelSettings()
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString strSQL=FormatSQL("delete from ChannelSettings");

    m_pDS->exec(strSQL.c_str());
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
    return false;
  }
}

bool CTVDatabase::GetChannelSettings(unsigned int channelID, CVideoSettings &settings)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;
    if (channelID < 0) return false;

    CStdString strSQL=FormatSQL("select * from ChannelSettings where idChannel like '%u'", channelID);

    m_pDS->query( strSQL.c_str() );
    if (m_pDS->num_rows() > 0)
    { // get the video settings info
      settings.m_AudioDelay           = m_pDS->fv("AudioDelay").get_asFloat();
      settings.m_AudioStream          = m_pDS->fv("AudioStream").get_asInt();
      settings.m_Brightness           = m_pDS->fv("Brightness").get_asFloat();
      settings.m_Contrast             = m_pDS->fv("Contrast").get_asFloat();
      settings.m_CustomPixelRatio     = m_pDS->fv("PixelRatio").get_asFloat();
      settings.m_NoiseReduction       = m_pDS->fv("NoiseReduction").get_asFloat();
      settings.m_Sharpness            = m_pDS->fv("Sharpness").get_asFloat();
      settings.m_CustomZoomAmount     = m_pDS->fv("ZoomAmount").get_asFloat();
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
      settings.m_InterlaceMethod      = (EINTERLACEMETHOD)m_pDS->fv("Deinterlace").get_asInt();
      settings.m_VolumeAmplification  = m_pDS->fv("VolumeAmplification").get_asFloat();
      settings.m_OutputToAllSpeakers  = m_pDS->fv("OutputToAllSpeakers").get_asBool();
      settings.m_SubtitleCached       = false;
      m_pDS->close();
      return true;
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return false;
}

bool CTVDatabase::SetChannelSettings(unsigned int channelID, const CVideoSettings &settings)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;
    if (channelID < 0)
    {
      // no match found, update required
      return false;
    }
    CStdString strSQL;
    strSQL.Format("select * from ChannelSettings where idChannel=%i", channelID);
    m_pDS->query( strSQL.c_str() );
    if (m_pDS->num_rows() > 0)
    {
      m_pDS->close();
      // update the item
      strSQL=FormatSQL("update ChannelSettings set Deinterlace=%i,ViewMode=%i,ZoomAmount=%f,PixelRatio=%f,"
                       "AudioStream=%i,SubtitleStream=%i,SubtitleDelay=%f,SubtitlesOn=%i,Brightness=%f,Contrast=%f,Gamma=%f,"
                       "VolumeAmplification=%f,AudioDelay=%f,OutputToAllSpeakers=%i,Sharpness=%f,NoiseReduction=%f,",
                       settings.m_InterlaceMethod, settings.m_ViewMode, settings.m_CustomZoomAmount, settings.m_CustomPixelRatio,
                       settings.m_AudioStream, settings.m_SubtitleStream, settings.m_SubtitleDelay, settings.m_SubtitleOn,
                       settings.m_Brightness, settings.m_Contrast, settings.m_Gamma, settings.m_VolumeAmplification, settings.m_AudioDelay,
                       settings.m_OutputToAllSpeakers,settings.m_Sharpness,settings.m_NoiseReduction);
      CStdString strSQL2;
      strSQL2=FormatSQL("Crop=%i,CropLeft=%i,CropRight=%i,CropTop=%i,CropBottom=%i where idChannel=%i\n", settings.m_Crop, settings.m_CropLeft, settings.m_CropRight, settings.m_CropTop, settings.m_CropBottom, channelID);
      strSQL += strSQL2;
      m_pDS->exec(strSQL.c_str());
      return true;
    }
    else
    { // add the items
      m_pDS->close();
      strSQL=FormatSQL("insert into ChannelSettings ( idChannel,Deinterlace,ViewMode,ZoomAmount,PixelRatio,"
                       "AudioStream,SubtitleStream,SubtitleDelay,SubtitlesOn,Brightness,Contrast,Gamma,"
                       "VolumeAmplification,AudioDelay,OutputToAllSpeakers,Crop,CropLeft,CropRight,CropTop,CropBottom,Sharpness,NoiseReduction)"
                       " values (%i,%i,%i,%f,%f,%i,%i,%f,%i,%f,%f,%f,%f,%f,",
                       channelID, settings.m_InterlaceMethod, settings.m_ViewMode, settings.m_CustomZoomAmount, settings.m_CustomPixelRatio,
                       settings.m_AudioStream, settings.m_SubtitleStream, settings.m_SubtitleDelay, settings.m_SubtitleOn,
                       settings.m_Brightness, settings.m_Contrast, settings.m_Gamma, settings.m_VolumeAmplification, settings.m_AudioDelay);
      CStdString strSQL2;
      strSQL2=FormatSQL("%i,%i,%i,%i,%i,%i,%f,%f)\n",  settings.m_OutputToAllSpeakers, settings.m_Crop, settings.m_CropLeft, settings.m_CropRight,
                    settings.m_CropTop, settings.m_CropBottom, settings.m_Sharpness, settings.m_NoiseReduction);
      strSQL += strSQL2;
      m_pDS->exec(strSQL.c_str());
      return true;
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (ID=%i) failed", __FUNCTION__, channelID);
    return false;
  }
}

