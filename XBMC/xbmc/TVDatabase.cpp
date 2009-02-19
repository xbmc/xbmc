/*
 *      Copyright (C) 2005-2008 Team XBMC
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
#include "stdafx.h"
#include "TVDatabase.h"
#include "utils/EPG.h"
#include "utils/GUIInfoManager.h"

using namespace std;
using namespace dbiplus;

#define TV_DATABASE_VERSION 3
#define TV_DATABASE_OLD_VERSION 2
#define TV_DATABASE_NAME "MyTV1.db"

CTVDatabase::CTVDatabase(void)
{
  m_preV2version = TV_DATABASE_OLD_VERSION;
  m_version = TV_DATABASE_VERSION;
  m_strDatabaseFile = TV_DATABASE_NAME;
}

CTVDatabase::~CTVDatabase(void)
{
}

bool CTVDatabase::CreateTables()
{
  try
  {
    CDatabase::CreateTables();

    CLog::Log(LOGINFO, "TV: Creating tables");
    CLog::Log(LOGINFO, "TV: Creating Clients table");
    m_pDS->exec("CREATE TABLE Clients (idClient integer primary key, Plugin text, Name text)\n");

    CLog::Log(LOGINFO, "TV: Creating Bouquets table");
    m_pDS->exec("CREATE TABLE Bouquets (idBouquet integer primary key, idClient integer, Name text)\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_bouquetlinkClient ON Bouquets (idBouquet, idClient)\n");

    CLog::Log(LOGINFO, "TV: Creating Channels table");
    m_pDS->exec("CREATE TABLE Channels (idClient integer, idBouquet integer, idChannel integer primary key, CallSign text, Name text, "
                "XBMCNumber integer, ClientNumber integer, PlayCount integer, IconPath text, Encrypted bool, HasSettings boolean)\n");

    CLog::Log(LOGINFO, "TV: Creating Programmes table");
    m_pDS->exec("CREATE TABLE Programmes (idProgramme integer primary key, idCategory integer, Title text, PlayCount integer, HasSettings boolean)\n");

    CLog::Log(LOGINFO, "TV: Creating ChannelSettings table");
    m_pDS->exec("CREATE TABLE ChannelSettings ( idClient integer, idChannel integer, Interleaved bool, NoCache bool, Deinterlace bool, FilmGrain integer, "
      "ViewMode integer, ZoomAmount float, PixelRatio float, AudioStream integer, SubtitleStream integer, "
      "SubtitleDelay float, SubtitlesOn bool, Brightness integer, Contrast integer, Gamma integer, "
      "VolumeAmplification float, AudioDelay float, OutputToAllSpeakers bool, ResumeTime integer, Crop bool, CropLeft integer, "
      "CropRight integer, CropTop integer, CropBottom integer)\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_ChannelSettings ON ChannelSettings (idChannel)\n");
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s unable to create TV tables:%i", __FUNCTION__, (int)GetLastError());
    return false;
  }

  return true;
}

bool CTVDatabase::CommitTransaction()
{
  if (CDatabase::CommitTransaction())
  { // number of items in the db has likely changed, so reset the infomanager cache
    g_infoManager.ResetPersistentCache(); /// what is this
    return true;
  }
  return false;
}

long CTVDatabase::AddClient(const CStdString &plugin, const CStdString &client)
{
	try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;
    
    long clientId;
    clientId = GetClientId(client);
    if (clientId < 0)
    {
      CStdString SQL=FormatSQL("insert into Clients (idClient, Plugin, Name) values (NULL, '%s', '%s')", plugin.c_str(), client.c_str());
      m_pDS->exec(SQL.c_str());
      clientId = (long)sqlite3_last_insert_rowid(m_pDB->getHandle());
    }

    return clientId;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, client.c_str());
  }
  return -1;
}

long CTVDatabase::AddBouquet(const long &idClient, const CStdString &bouquet)
{
	try
  {
    if (bouquet == "") return -1;
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;
    
    long bouquetId;
    bouquetId = GetBouquetId(bouquet);
    if (bouquetId < 0)
    {
      CStdString SQL=FormatSQL("insert into Bouquets (idClient, idBouquet, Name) values ('%i', NULL, '%s')", idClient, bouquet.c_str());
      m_pDS->exec(SQL.c_str());
      bouquetId = (long)sqlite3_last_insert_rowid(m_pDB->getHandle());
    }

    return bouquetId;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, bouquet.c_str());
  }
  return -1;
}

long CTVDatabase::AddChannel(const long &idClient, const long &idBouquet, const CStdString &Callsign, 
                             const CStdString &Name, const int &Number, const CStdString &iconPath)
{
	try
  {
    if (idBouquet < 0 || idClient < 0) return -1;
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    long channelId;
    channelId = GetChannelId(Name);
    if (channelId < 0)
    {
      CStdString SQL=FormatSQL("insert into Channels (idClient, idBouquet, idChannel, CallSign, Name, Number, PlayCount, IconPath, HasSettings) "
                               "values ('%i', '%i', NULL, '%s', '%s', '%i', NULL, '%s', NULL)", idClient, idBouquet, Callsign.c_str(), 
                               Name.c_str(), Number, iconPath.c_str());
      m_pDS->exec(SQL.c_str());
      channelId = (long)sqlite3_last_insert_rowid(m_pDB->getHandle());
    }

    return channelId;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, Callsign.c_str());
  }
  return -1;
}

long CTVDatabase::AddProgramme(const CStdString &Title, const long &idCategory)
{
	try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    long programmeId;
    programmeId = GetProgrammeId(Title);
    if (programmeId < 0)
    {
      CStdString SQL=FormatSQL("insert into Programmes (idProgramme, Title, idCategory, PlayCount, HasSettings) "
                               "values (NULL, '%s', '%u', NULL, NULL)", Title.c_str(), idCategory);
      m_pDS->exec(SQL.c_str());
      programmeId = (long)sqlite3_last_insert_rowid(m_pDB->getHandle());
    }

    return programmeId;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, Title.c_str());
  }
  return -1;
}

long CTVDatabase::AddCategory(const CStdString &category)
{
	try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;
    
    long categoryId;
    categoryId = GetCategoryId(category);
    if (categoryId < 0)
    {
      CStdString SQL=FormatSQL("insert into categories (idCategory, Name) values (NULL, '%s')", category.c_str());
      m_pDS->exec(SQL.c_str());
      categoryId = (long)sqlite3_last_insert_rowid(m_pDB->getHandle());
    }

    return categoryId;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, category.c_str());
  }
  return -1;
}

CDateTime CTVDatabase::GetDataEnd(DWORD clientID)
{
  CDateTime lastProgramme;
  try
  {
    if (NULL == m_pDB.get()) return lastProgramme;
    if (NULL == m_pDS.get()) return lastProgramme;

    CStdString SQL=FormatSQL("SELECT GuideData.EndTime FROM GuideData "
                             "WHERE GuideData.idClient = '%u' "
                             "ORDER BY Guidedata.EndTime DESC;", clientID);
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

/// \brief GetChannelSettings() obtains any saved video settings for the current channel.
/// \retval Returns true if the settings exist, false otherwise. */
bool CTVDatabase::GetChannelSettings(const CStdString &channel, CVideoSettings &settings)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    // obtain the ChannelID (if it exists)
    long channelId = GetChannelId(channel);
    if (channelId < 0)
      return false;


    CStdString SQL=FormatSQL("select * from ChannelSettings where idChannel like '%u'", channelId);
    m_pDS->query( SQL.c_str() );
    if (m_pDS->num_rows() > 0)
    { // get the channel settings info
      settings.m_AudioDelay = m_pDS->fv("AudioDelay").get_asFloat();
      settings.m_AudioStream = m_pDS->fv("AudioStream").get_asInteger();
      settings.m_Brightness = m_pDS->fv("Brightness").get_asInteger();
      settings.m_Contrast = m_pDS->fv("Contrast").get_asInteger();
      settings.m_CustomPixelRatio = m_pDS->fv("PixelRatio").get_asFloat();
      settings.m_CustomZoomAmount = m_pDS->fv("ZoomAmount").get_asFloat();
      settings.m_Gamma = m_pDS->fv("Gamma").get_asInteger();
      settings.m_NonInterleaved = m_pDS->fv("Interleaved").get_asBool();
      settings.m_NoCache = m_pDS->fv("NoCache").get_asBool();
      settings.m_SubtitleDelay = m_pDS->fv("SubtitleDelay").get_asFloat();
      settings.m_SubtitleOn = m_pDS->fv("SubtitlesOn").get_asBool();
      settings.m_SubtitleStream = m_pDS->fv("SubtitleStream").get_asInteger();
      settings.m_ViewMode = m_pDS->fv("ViewMode").get_asInteger();
      settings.m_ResumeTime = m_pDS->fv("ResumeTime").get_asInteger();
      settings.m_Crop = m_pDS->fv("Crop").get_asBool();
      settings.m_CropLeft = m_pDS->fv("CropLeft").get_asInteger();
      settings.m_CropRight = m_pDS->fv("CropRight").get_asInteger();
      settings.m_CropTop = m_pDS->fv("CropTop").get_asInteger();
      settings.m_CropBottom = m_pDS->fv("CropBottom").get_asInteger();
      settings.m_InterlaceMethod = (EINTERLACEMETHOD)m_pDS->fv("Deinterlace").get_asInteger();
      settings.m_VolumeAmplification = m_pDS->fv("VolumeAmplification").get_asFloat();
      settings.m_OutputToAllSpeakers = m_pDS->fv("OutputToAllSpeakers").get_asBool();
      settings.m_SubtitleCached = false;
      m_pDS->close();
      return true;
    }
    else
    {
      m_pDS->close();
      return false;
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return false;
}

/// \brief Sets the settings for a particular video file
bool CTVDatabase::SetChannelSettings(const CStdString& channel, const CVideoSettings &setting)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    long channelId = GetChannelId(channel);
    
    if (channelId < 0)
    { // no match found, update required
      return false;
    }

    CStdString SQL;
    SQL.Format("select * from ChannelSettings where idChannel=%i", channelId);
    m_pDS->query( SQL.c_str() );
    if (m_pDS->num_rows() > 0)
    {
      m_pDS->close();
      // update the item
      SQL=FormatSQL("update ChannelSettings set Interleaved=%i,NoCache=%i,Deinterlace=%i,FilmGrain=%i,ViewMode=%i,ZoomAmount=%f,PixelRatio=%f,"
                       "AudioStream=%i,SubtitleStream=%i,SubtitleDelay=%f,SubtitlesOn=%i,Brightness=%i,Contrast=%i,Gamma=%i,"
                       "VolumeAmplification=%f,AudioDelay=%f,OutputToAllSpeakers=%i,",
                       setting.m_NonInterleaved, setting.m_NoCache, setting.m_InterlaceMethod, setting.m_FilmGrain, setting.m_ViewMode, 
                       setting.m_CustomZoomAmount, setting.m_CustomPixelRatio, setting.m_AudioStream, setting.m_SubtitleStream, setting.m_SubtitleDelay, 
                       setting.m_SubtitleOn, setting.m_Brightness, setting.m_Contrast, setting.m_Gamma, setting.m_VolumeAmplification, setting.m_AudioDelay,
                       setting.m_OutputToAllSpeakers);
      CStdString SQL2;
      SQL2=FormatSQL("ResumeTime=%i,Crop=%i,CropLeft=%i,CropRight=%i,CropTop=%i,CropBottom=%i where idFile=%i\n", setting.m_ResumeTime, 
                        setting.m_Crop, setting.m_CropLeft, setting.m_CropRight, setting.m_CropTop, setting.m_CropBottom, channelId);
      SQL += SQL2;
      m_pDS->exec(SQL.c_str());
      return true;
    }
    else 
    { // add the items
      m_pDS->close();
      SQL=FormatSQL("insert into ChannelSettings ( idChannel,Interleaved,NoCache,Deinterlace,FilmGrain,ViewMode,ZoomAmount,PixelRatio,"
                       "AudioStream,SubtitleStream,SubtitleDelay,SubtitlesOn,Brightness,Contrast,Gamma,"
                       "VolumeAmplification,AudioDelay,OutputToAllSpeakers,ResumeTime,Crop,CropLeft,CropRight,CropTop,CropBottom)"
                       " values (%i,%i,%i,%i,%i,%i,%f,%f,%i,%i,%f,%i,%i,%i,%i,%f,%f,",
                       channelId, setting.m_NonInterleaved, setting.m_NoCache, setting.m_InterlaceMethod, setting.m_FilmGrain, setting.m_ViewMode, 
                       setting.m_CustomZoomAmount, setting.m_CustomPixelRatio, setting.m_AudioStream, setting.m_SubtitleStream, setting.m_SubtitleDelay, 
                       setting.m_SubtitleOn, setting.m_Brightness, setting.m_Contrast, setting.m_Gamma, setting.m_VolumeAmplification, setting.m_AudioDelay);
      CStdString SQL2;
      SQL2=FormatSQL("%i,%i,%i,%i,%i,%i,%i)\n", setting.m_OutputToAllSpeakers, setting.m_ResumeTime, setting.m_Crop, setting.m_CropLeft, setting.m_CropRight,
                    setting.m_CropTop, setting.m_CropBottom);
      SQL += SQL2;
      m_pDS->exec(SQL.c_str());
      return true;
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, channel.c_str());
    return false;
  }
}

void CTVDatabase::NewChannel(DWORD clientID, CStdString bouquet, CStdString chanName, CStdString callsign, int chanNum, CStdString iconPath)
{
  long idBouquet = AddBouquet(clientID, bouquet);
  AddChannel(clientID, idBouquet, callsign, chanName, chanNum, iconPath);
}

void CTVDatabase::GetChannelList(DWORD clientID, EPGData &channels)
{
  channels.clear();
  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;

    CStdString SQL=FormatSQL("select * from Channels WHERE Channels.idClient=%u", clientID);
    m_pDS->query( SQL.c_str() );

    while (!m_pDS->eof())
    {
      int num = m_pDS->fv("Channels.Number").get_asInteger();
      long idBouquet = m_pDS->fv("Channels.idBouquet").get_asLong();
      long idChannel = m_pDS->fv("Channels.idChannel").get_asLong();
      CStdString name = m_pDS->fv("Channels.Name").get_asString();
      CStdString callsign = m_pDS->fv("Channels.Callsign").get_asString();
      CStdString iconPath = m_pDS->fv("Channels.IconPath").get_asString();
  
      CTVChannel* channel = new CTVChannel(clientID, idBouquet, idChannel, num, name, callsign, iconPath);
      channels.push_back(channel);

      m_pDS->next();
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
}

int CTVDatabase::GetNumChannels(DWORD clientID)
{
  try
  {
    if (NULL == m_pDB.get()) return 0;
    if (NULL == m_pDS.get()) return 0;

    CStdString SQL=FormatSQL("select * from Channels WHERE Channels.idClient=%u", clientID);
    m_pDS->query( SQL.c_str() );
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


bool CTVDatabase::HasChannel(DWORD clientID, const CStdString &name)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString SQL=FormatSQL("select * from Channels WHERE Channels.Name = '%s' AND Channels.idClient = '%u'", name.c_str(), clientID);
    m_pDS->query( SQL.c_str() );
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

void CTVDatabase::AddChannelData(CFileItemList &channel)
{
  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;

    long channelId = GetChannelId(channel.Get(0)->GetEPGInfoTag()->m_strChannel);
    long bouquetId = GetBouquetId(channel.Get(0)->GetEPGInfoTag()->m_strBouquet);
    long clientId  = 0; /*channel.Get(0)->GetEPGInfoTag()->GetSourceID();*/

    DWORD tick = timeGetTime();
    BeginTransaction();
    int numItems = channel.Size();
    for (int i = 0; i < numItems; i++)
    {
      CFileItemPtr item = channel.Get(i);
      long categoryId = AddCategory(item->GetEPGInfoTag()->m_strGenre);
      long programmeId = AddProgramme(item->GetEPGInfoTag()->m_strTitle, categoryId);

      CStdString outline, description, episodeID, seriesID;
      CDateTime  start, end;

      outline = item->GetEPGInfoTag()->m_strPlotOutline;
      description = item->GetEPGInfoTag()->m_strPlot;
      episodeID = item->GetEPGInfoTag()->m_episodeID;
      seriesID = item->GetEPGInfoTag()->m_seriesID;
      start = item->GetEPGInfoTag()->m_startTime;
      end = item->GetEPGInfoTag()->m_endTime;

      CStdString SQL=FormatSQL("insert into GuideData (idClient, idBouquet, idChannel, idProgramme, idCategory, "
        "Subtitle, Description, EpisodeID, SeriesID, StartTime, EndTime, idUniqueBroadcast) "
        "values ('%u', '%u', '%u', '%u', '%u', '%s', '%s', '%s', '%s', '%s', '%s', NULL)",
        clientId, bouquetId, channelId, programmeId, categoryId, outline.c_str(), description.c_str(), episodeID.c_str(), 
        seriesID.c_str(), start.GetAsDBDateTime().c_str(), end.GetAsDBDateTime().c_str());
      
      m_pDS->exec(SQL.c_str());
    }
    CommitTransaction();
    CLog::Log(LOGDEBUG, "%s - %u items in %ums", __FUNCTION__, numItems, timeGetTime()-tick);
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
}

bool CTVDatabase::GetProgrammesByChannelName(const CStdString &channel, CFileItemList &matches, const CDateTime &start, const CDateTime &end)
{
  try
  {
    /*DWORD time = timeGetTime();*/
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    long channelId = GetChannelId(channel);
    if (channelId < 0 )
      return false;

    CDateTime     gridStart;
    CDateTime     gridEnd;
    CDateTimeSpan offset;

    gridStart = start;
    gridEnd = end;

    CStdString SQL=FormatSQL("SELECT Bouquets.Name, Channels.Name, Channels.Number, Categories.Name, Programmes.Title, "
      "GuideData.Subtitle, GuideData.Description, GuideData.StartTime, GuideData.EndTime, GuideData.EpisodeID, GuideData.SeriesID, GuideData.idUniqueBroadcast FROM GuideData "
      "JOIN Bouquets ON Bouquets.idBouquet=GuideData.idBouquet "
      "JOIN Programmes ON Programmes.idProgramme=GuideData.idProgramme "
      "JOIN Channels ON Channels.idChannel=GuideData.idChannel "
      "JOIN Categories ON Categories.idCategory=GuideData.idCategory "
      "WHERE GuideData.idChannel = '%u' AND GuideData.EndTime > '%s' AND GuideData.StartTime < '%s' "
      "ORDER BY GuideData.StartTime;", channelId, gridStart.GetAsDBDateTime().c_str(), gridEnd.GetAsDBDateTime().c_str());    
    
    //run query
    /*CLog::Log(LOGDEBUG, "%s query: %s", __FUNCTION__, SQL.c_str());*/
    if (!m_pDS->query(SQL.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return false; /// Don't currently want channels without any guide data
    }

    //CLog::Log(LOGDEBUG,"Time for (initial) SQL query = %ldms", timeGetTime() - time);
    
    /*time = timeGetTime();*/
    // get data from returned rows
    while (!m_pDS->eof())
    {
      CTVEPGInfoTag programme = GetUniqueBroadcast(m_pDS);
      CFileItemPtr pItem(new CFileItem(programme));
      FillProperties(pItem.get());
      matches.Add(pItem);
      m_pDS->next();
    }
    //CLog::Log(LOGDEBUG,"Time to retrieve programmes from dataset = %ldms", timeGetTime() - time);

    // cleanup
    m_pDS->close();
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, channel.c_str());
  }
  return false;
}

bool CTVDatabase::GetProgrammesByEpisodeID(const CStdString& episodeID, CFileItemList* items, bool noHistory)
{
  /* Finds unique broadcasts with matching EpisodeIDs. Default behaviour is to return all programs with end times later than now */
  /* setting noHistory to false returns all matching items */
  if (episodeID.empty())
    return false;
  try
  {
    /*DWORD time = timeGetTime();*/
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CDateTime from;

    if (!noHistory)
    {
      from = CDateTime::GetCurrentDateTime();
    }
    else
    {
      from.SetFromDBDate("1970-01-01");
    }
   

    CStdString SQL= FormatSQL("SELECT Bouquets.Name, Channels.Name, Channels.Number, Categories.Name, Programmes.Title, "
                              "GuideData.Subtitle, GuideData.Description, GuideData.StartTime, GuideData.EndTime, GuideData.EpisodeID, GuideData.SeriesID, GuideData.idUniqueBroadcast FROM GuideData "
                              "JOIN Bouquets ON Bouquets.idBouquet=GuideData.idBouquet "
                              "JOIN Programmes ON Programmes.idProgramme=GuideData.idProgramme "
                              "JOIN Channels ON Channels.idChannel=GuideData.idChannel "
                              "JOIN Categories ON Categories.idCategory=GuideData.idCategory "
                              "WHERE GuideData.EpisodeID = '%s' AND GuideData.EndTime > '%s' "
                              "ORDER BY GuideData.StartTime;", episodeID.c_str(), from.GetAsDBDateTime().c_str()); 

    CLog::Log(LOGDEBUG, "%s query: %s", __FUNCTION__, SQL.c_str());
    //run query
    m_pDS->query( SQL.c_str() );

    int rowsFound = m_pDS->num_rows();
    if (rowsFound == 0)
    {
      m_pDS->close();
      return false;
    }

    items->Reserve(rowsFound + items->Size()); // because items could already be populated
    while (!m_pDS->eof())
    {
      CTVEPGInfoTag progInfo = GetUniqueBroadcast(m_pDS);
      CFileItemPtr item(new CFileItem(progInfo));
      FillProperties(item.get());
      items->Add(item);
      m_pDS->next();
    }

    /*CLog::Log(LOGDEBUG,"Time for SQL query = %ldms", timeGetTime() - time);*/

    m_pDS->close();

    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
    return false;
  }

}

bool CTVDatabase::GetProgrammesBySubtitle(const CStdString& subtitle, CFileItemList* items, bool noHistory)
{
  /* Finds unique broadcasts with matching SubTitles. Default behaviour is to return all programs with end times later than now */
  /* setting noHistory to false returns all available matched */
  if (subtitle.empty())
    return false;
  try
  {
    /*DWORD time = timeGetTime();*/
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CDateTime from;

    if (noHistory)
    {
      CDateTimeSpan hour;
      from = CDateTime::GetCurrentDateTime();
    }


    CStdString SQL= FormatSQL("SELECT Bouquets.Name, Channels.Name, Channels.Number, Categories.Name, Programmes.Title, "
      "GuideData.Subtitle, GuideData.Description, GuideData.StartTime, GuideData.EndTime, GuideData.EpisodeID, GuideData.SeriesID, GuideData.idUniqueBroadcast FROM GuideData "
      "JOIN Bouquets ON Bouquets.idBouquet=GuideData.idBouquet "
      "JOIN Programmes ON Programmes.idProgramme=GuideData.idProgramme "
      "JOIN Channels ON Channels.idChannel=GuideData.idChannel "
      "JOIN Categories ON Categories.idCategory=GuideData.idCategory "
      "WHERE GuideData.SubTitle = '%s' AND GuideData.EndTime > '%s' AND GuideData.EpisodeID = '' "
      "ORDER BY GuideData.StartTime;", subtitle.c_str(), from.GetAsDBDateTime().c_str()); 

    /*CLog::Log(LOGDEBUG, "%s query: %s", __FUNCTION__, SQL.c_str());*/
    //run query
    m_pDS->query( SQL.c_str() );

    int rowsFound = m_pDS->num_rows();
    if (rowsFound == 0)
    {
      m_pDS->close();
      return false;
    }

    items->Reserve(rowsFound + items->Size()); // because items could already be populated
    while (!m_pDS->eof())
    {
      CTVEPGInfoTag progInfo = GetUniqueBroadcast(m_pDS);
      CFileItemPtr item(new CFileItem(progInfo));
      FillProperties(item.get());
      items->Add(item);
      m_pDS->next();
    }

    /*CLog::Log(LOGDEBUG,"Time for SQL query = %ldms", timeGetTime() - time);*/

    m_pDS->close();

    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
    return false;
  }

}

void CTVDatabase::GetProgrammesByName(const CStdString& progName, CFileItemList& items, bool noHistory)
{

}

long CTVDatabase::GetClientId(const CStdString& client)
{
	CStdString SQL;
  try
  {
    long clientId=-1;
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;
    
    SQL=FormatSQL("select idClient from Clients where Callsign like '%s'", client.c_str());
    m_pDS->query(SQL.c_str());
    if (!m_pDS->eof())
      clientId = m_pDS->fv("Clients.idClient").get_asLong();

    m_pDS->close();
    return clientId;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s unable to get ClientId (%s)", __FUNCTION__, SQL.c_str());
  }
  return -1;
}

long CTVDatabase::GetBouquetId(const CStdString& bouquet)
{
	CStdString SQL;
  try
  {
    long lBouquetId=-1;
    if (NULL == m_pDB.get()) return lBouquetId;
    if (NULL == m_pDS.get()) return lBouquetId;
    
    SQL=FormatSQL("select idBouquet from Bouquets where Name like '%s'", bouquet.c_str());
    m_pDS->query(SQL.c_str());
    if (!m_pDS->eof())
      lBouquetId = m_pDS->fv("Bouquets.idBouquet").get_asLong();

    m_pDS->close();
    return lBouquetId;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s unable to get BouquetId (%s)", __FUNCTION__, SQL.c_str());
  }
  return -1;
}

long CTVDatabase::GetChannelId(const CStdString& channel)
{
	CStdString SQL;
  try
  {
    long channelId=-1;
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;
    
    SQL=FormatSQL("select idChannel from Channels where Name like '%s'", channel.c_str());
    m_pDS->query(SQL.c_str());
    if (!m_pDS->eof())
      channelId = m_pDS->fv("Channels.idChannel").get_asLong();

    m_pDS->close();
    return channelId;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s unable to get ChannelId (%s)", __FUNCTION__, SQL.c_str());
  }
  return -1;
}

long CTVDatabase::GetProgrammeId(const CStdString& programme)
{
	CStdString SQL;
  try
  {
    long programmeId=-1;
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;
    
    SQL=FormatSQL("select idProgramme from Programmes where Title like '%s'", programme.c_str());
    m_pDS->query(SQL.c_str());
    if (!m_pDS->eof())
      programmeId = m_pDS->fv("Programmes.idProgramme").get_asLong();

    m_pDS->close();
    return programmeId;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s unable to get ProgrammeId (%s)", __FUNCTION__, SQL.c_str());
  }
  return -1;
}

long CTVDatabase::GetCategoryId(const CStdString& category)
{
	CStdString SQL;
  try
  {
    long categoryId=-1;
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;
    
    SQL=FormatSQL("select idCategory from categories where Name like '%s'", category.c_str());
    m_pDS->query(SQL.c_str());
    if (!m_pDS->eof())
      categoryId = m_pDS->fv("categories.idCategory").get_asLong();

    m_pDS->close();
    return categoryId;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s unable to get CategoryId (%s)", __FUNCTION__, SQL.c_str());
  }
  return -1;
}

CTVEPGInfoTag CTVDatabase::GetUniqueBroadcast(auto_ptr<Dataset> &pDS)
{
  // Dataset columns:
  // 0:  Bouquets.Name
  // 1:  Channels.Name
  // 2:  Channels.Number
  // 3:  Categories.Name
  // 4:  Programmes.Title
  // 5:  GuideData.Subtitle
  // 6:  GuideData.Description
  // 7:  GuideData.StartTime
  // 8:  GuideData.EndTime
  // 9:  GuideData.EpisodeID
  //10:  GuideData.SeriesID
  //11:  GuideData.UniqueBroadcastID

  CTVEPGInfoTag details(m_pDS->fv(11).get_asLong());

  details.m_strBouquet = m_pDS->fv(0).get_asString();
  details.m_strChannel = m_pDS->fv(1).get_asString();
  details.m_channelNum = m_pDS->fv(2).get_asInteger();
  details.m_strGenre   = m_pDS->fv(3).get_asString();
  details.m_strTitle   = m_pDS->fv(4).get_asString();
  details.m_strPlotOutline = m_pDS->fv(5).get_asString();
  details.m_strPlot    = m_pDS->fv(6).get_asString();
  details.m_startTime.SetFromDBDateTime(m_pDS->fv(7).get_asString());
  details.m_endTime.SetFromDBDateTime(m_pDS->fv(8).get_asString());
  if (details.m_startTime.IsValid() && details.m_endTime.IsValid())
  {
    details.m_duration = details.m_endTime - details.m_startTime;
  }
  details.m_episodeID = m_pDS->fv(9).get_asString();
  details.m_seriesID  = m_pDS->fv(10).get_asString();

  return details;
}

void CTVDatabase::FillProperties(CFileItem* programme)
{
  /* Fills the properties map with properties needed for skins */
  programme->SetProperty("uniqueID", (int)programme->GetEPGInfoTag()->GetDbID());
  programme->SetProperty("Bouquet", programme->GetEPGInfoTag()->m_strBouquet);
  programme->SetProperty("Channel", programme->GetEPGInfoTag()->m_strChannel);
  programme->SetProperty("Genre", programme->GetEPGInfoTag()->m_strGenre);
  programme->SetProperty("Title", programme->GetEPGInfoTag()->m_strTitle);
  programme->SetProperty("PlotOutline", programme->GetEPGInfoTag()->m_strPlotOutline);
  programme->SetProperty("Plot", programme->GetEPGInfoTag()->m_strPlot);
  programme->SetProperty("ChannelNum", programme->GetEPGInfoTag()->m_channelNum);
  /// temp: get a short datetime
  CDateTime startTime = programme->GetEPGInfoTag()->m_startTime;
  SYSTEMTIME dateTime;
  startTime.GetAsSystemTime(dateTime);
  CStdString day;
  switch (dateTime.wDayOfWeek)
  {
  case 1 : day = g_localizeStrings.Get(11); break;
  case 2 : day = g_localizeStrings.Get(12); break;
  case 3 : day = g_localizeStrings.Get(13); break;
  case 4 : day = g_localizeStrings.Get(14); break;
  case 5 : day = g_localizeStrings.Get(15); break;
  case 6 : day = g_localizeStrings.Get(16); break;
  default: day = g_localizeStrings.Get(17); break;
  }
  CStdString shortTime;
  shortTime = startTime.GetAsLocalizedTime("HH:mm", false);
  programme->SetProperty("LocaleDateTime", shortTime + " " + day);
}
