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
#include "utils/GUIInfoManager.h"
#include "util.h"

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
    CLog::Log(LOGINFO, "TV: Creating Sources table");
    m_pDS->exec("CREATE TABLE Sources (idSource integer primary key, CallSign text, Name text)\n");

    CLog::Log(LOGINFO, "TV: Creating Categories table");
    m_pDS->exec("CREATE TABLE categories (idCategory integer primary key, Name text)\n");

    CLog::Log(LOGINFO, "TV: Creating Bouquets table");
    m_pDS->exec("CREATE TABLE Bouquets (idBouquet integer primary key, idSource integer, Name text)\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_bouquetlinksource ON Bouquets (idBouquet, idSource)\n");

    CLog::Log(LOGINFO, "TV: Creating Channels table");
    m_pDS->exec("CREATE TABLE Channels (idSource integer, idBouquet integer, idChannel integer primary key, CallSign text, Name text,"
                "Number integer, PlayCount integer, HasSettings boolean)\n");

    CLog::Log(LOGINFO, "TV: Creating Programmes table");
    m_pDS->exec("CREATE TABLE Programmes (idProgramme integer primary key, idCategory integer, Title text, PlayCount integer, HasSettings boolean)"
                /*"StreamPIDs text, StreamTypes text, StreamNames text)*/"\n");

    CLog::Log(LOGINFO, "TV: Creating GuideData table");
    m_pDS->exec("CREATE TABLE GuideData (idSource integer, idBouquet integer, idChannel integer, idProgramme integer,"
                "idCategory integer, Subtitle text, Description text, EpisodeID text, SeriesID text, StartTime datetime,"
                "EndTime datetime, Year string)"
                /*"StreamPIDs text, StreamTypes text, StreamNames text)*/"\n");
    m_pDS->exec("CREATE UNIQUE INDEX idx_UniqueBroadcast on GuideData(idProgramme, idChannel, idBouquet, StartTime desc)\n");

    CLog::Log(LOGINFO, "TV: Creating ChannelSettings table");
    m_pDS->exec("CREATE TABLE ChannelSettings ( idSource integer, idChannel integer, Interleaved bool, NoCache bool, Deinterlace bool, FilmGrain integer,"
      "ViewMode integer, ZoomAmount float, PixelRatio float, AudioStream integer, SubtitleStream integer,"
      "SubtitleDelay float, SubtitlesOn bool, Brightness integer, Contrast integer, Gamma integer,"
      "VolumeAmplification float, AudioDelay float, OutputToAllSpeakers bool, ResumeTime integer, Crop bool, CropLeft integer,"
      "CropRight integer, CropTop integer, CropBottom integer)\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_ChannelSettings ON ChannelSettings (idChannel)\n");

    CLog::Log(LOGINFO, "TV: Creating ProgrammeSettings table");
    m_pDS->exec("CREATE TABLE ProgrammeSettings ( idSource integer, idChannel integer, idProgramme integer, Interleaved bool, NoCache bool, Deinterlace bool, FilmGrain integer,"
      "ViewMode integer, ZoomAmount float, PixelRatio float, AudioStream integer, SubtitleStream integer,"
      "SubtitleDelay float, SubtitlesOn bool, Brightness integer, Contrast integer, Gamma integer,"
      "VolumeAmplification float, AudioDelay float, OutputToAllSpeakers bool, ResumeTime integer, Crop bool, CropLeft integer,"
      "CropRight integer, CropTop integer, CropBottom integer)\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_ProgrammeSettings ON ProgrammeSettings (idProgramme)\n");
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
    g_infoManager.ResetPersistentCache();
    return true;
  }
  return false;
}

bool CTVDatabase::FillEPG(const CStdString &source, const CStdString &bouquet, const CStdString &channame, const CStdString &callsign, const int &channum, const CStdString &progTitle, const CStdString &progSubtitle,
                          const CStdString &progDescription, const CStdString &episode, const CStdString &series, const CDateTime &progStartTime, const CDateTime &progEndTime, const CStdString &category)
{
	try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;
    
    long sourceId = AddSource(source);
    if (sourceId < 0)
      return false;

    long bouquetId = AddBouquet(sourceId, bouquet);
    if (bouquetId < 0)
      return false;

    long channelId = AddChannel(sourceId, bouquetId, callsign, channame, channum);
    if (channelId < 0)
      return false;

    long categoryId = AddCategory(category);
    if (categoryId < 0)
      return false;

    long programmeId = AddProgramme(progTitle, categoryId);
    if (programmeId < 0)
      return false;

    CStdString SQL=FormatSQL("insert into GuideData (idSource, idBouquet, idChannel, idProgramme, idCategory,"
                             "Subtitle, Description, EpisodeID, SeriesID, StartTime, EndTime)"
                             "values ('%u', '%u', '%u', '%u', '%u', '%s', '%s', '%s', '%s', '%s', '%s')",
                             sourceId, bouquetId, channelId, programmeId, categoryId, progSubtitle.c_str(), progDescription.c_str(), episode.c_str(), 
                             series.c_str(), progStartTime.GetAsDBDateTime().c_str(), progEndTime.GetAsDBDateTime().c_str());
    m_pDS->exec(SQL.c_str());
    long progId = (long)sqlite3_last_insert_rowid(m_pDB->getHandle());

    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, source);
  }
  return false;
}

long CTVDatabase::AddSource(const CStdString &source)
{
	try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;
    
    long sourceId;
    sourceId = GetSourceId(source);
    if (sourceId < 0)
    {
      CStdString SQL=FormatSQL("insert into Sources (idSource, Callsign, Name) values (NULL, '%s', '%s')", source.c_str(), source.c_str());
      m_pDS->exec(SQL.c_str());
      sourceId = (long)sqlite3_last_insert_rowid(m_pDB->getHandle());
    }

    return sourceId;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, source);
  }
  return -1;
}

long CTVDatabase::AddBouquet(const long &idSource, const CStdString &bouquet)
{
	try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;
    
    long bouquetId;
    bouquetId = GetBouquetId(bouquet);
    if (bouquetId < 0)
    {
      CStdString SQL=FormatSQL("insert into Bouquets (idSource, idBouquet, Name) values ('%i', NULL, '%s')", idSource, bouquet.c_str());
      m_pDS->exec(SQL.c_str());
      bouquetId = (long)sqlite3_last_insert_rowid(m_pDB->getHandle());
    }

    return bouquetId;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, bouquet);
  }
  return -1;
}

long CTVDatabase::AddChannel(const long &idSource, const long &idBouquet, const CStdString &Callsign, 
                             const CStdString &Name, const int &Number)
{
	try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    long channelId;
    channelId = GetChannelId(Name);
    if (channelId < 0)
    {
      CStdString SQL=FormatSQL("insert into Channels (idSource, idBouquet, idChannel, CallSign, Name, Number, PlayCount, HasSettings) "
                               "values ('%i', '%i', NULL, '%s', '%s', '%i', NULL, NULL)", idSource, idBouquet, Callsign.c_str(), 
                               Name.c_str(), Number);
      m_pDS->exec(SQL.c_str());
      channelId = (long)sqlite3_last_insert_rowid(m_pDB->getHandle());
    }

    return channelId;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, Callsign);
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
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, Title);
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
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, category);
  }
  return -1;
}

CDateTime CTVDatabase::GetDataEnd()
{
  CDateTime lastProgramme;
  try
  {
    if (NULL == m_pDB.get()) return lastProgramme;
    if (NULL == m_pDS.get()) return lastProgramme;

    CStdString SQL=FormatSQL("SELECT GuideData.EndTime FROM GuideData "
                             "ORDER BY guidedata.EndTime DESC;");
    m_pDS->query( SQL.c_str() );
    if (!m_pDS->eof())
    {
      lastProgramme.SetFromDBDateTime(m_pDS->fv("EndTime").get_asString());
    }
    m_pDS->close();
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
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, channel);
    return false;
  }
}

/// \brief GetProgrammeSettings() obtains any saved video settings for the current programme.
/// \retval Returns true if the settings exist, false otherwise. */
bool CTVDatabase::GetProgrammeSettings(const CStdString &programme, CVideoSettings &settings)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    // obtain the ProgrammeID (if it exists)
    CStdString SQL;
    /*=FormatSQL("select * from settings, files, path where settings.idfile=files.idfile and path.idpath=files.idpath and path.strPath like '%s'"
               "and files.strFileName like '%s'", strPath.c_str() , strFileName.c_str());*/

    m_pDS->query( SQL.c_str() );
    if (m_pDS->num_rows() > 0)
    { // get the programme settings info
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
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return false;
}

/// \brief Sets the settings for a particular video file
void CTVDatabase::SetProgrammeSettings(const CStdString& programmeID, const CVideoSettings &setting)
{
  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    long programmeId = GetProgrammeId(programmeID);
    
    if (programmeId < 0)
    { // no match found, update required
      return ;
    }

    CStdString SQL;
    SQL.Format("select * from ProgrammeSettings where idProgramme=%i", programmeId);
    m_pDS->query( SQL.c_str() );
    if (m_pDS->num_rows() > 0)
    {
      m_pDS->close();
      // update the item
      SQL=FormatSQL("update ProgrammeSettings set Interleaved=%i,NoCache=%i,Deinterlace=%i,FilmGrain=%i,ViewMode=%i,ZoomAmount=%f,PixelRatio=%f,"
                       "AudioStream=%i,SubtitleStream=%i,SubtitleDelay=%f,SubtitlesOn=%i,Brightness=%i,Contrast=%i,Gamma=%i,"
                       "VolumeAmplification=%f,AudioDelay=%f,OutputToAllSpeakers=%i,",
                       setting.m_NonInterleaved, setting.m_NoCache, setting.m_InterlaceMethod, setting.m_FilmGrain, setting.m_ViewMode, 
                       setting.m_CustomZoomAmount, setting.m_CustomPixelRatio, setting.m_AudioStream, setting.m_SubtitleStream, setting.m_SubtitleDelay, 
                       setting.m_SubtitleOn, setting.m_Brightness, setting.m_Contrast, setting.m_Gamma, setting.m_VolumeAmplification, setting.m_AudioDelay,
                       setting.m_OutputToAllSpeakers);
      CStdString SQL2;
      SQL2=FormatSQL("ResumeTime=%i,Crop=%i,CropLeft=%i,CropRight=%i,CropTop=%i,CropBottom=%i where idFile=%i\n", setting.m_ResumeTime, 
                        setting.m_Crop, setting.m_CropLeft, setting.m_CropRight, setting.m_CropTop, setting.m_CropBottom, programmeId);
      SQL += SQL2;
      m_pDS->exec(SQL.c_str());
      return ;
    }
    else /// TODO
    { // add the items
      m_pDS->close();
      SQL=FormatSQL("insert into ProgrammeSettings ( idProgramme,Interleaved,NoCache,Deinterlace,FilmGrain,ViewMode,ZoomAmount,PixelRatio,"
                       "AudioStream,SubtitleStream,SubtitleDelay,SubtitlesOn,Brightness,Contrast,Gamma,"
                       "VolumeAmplification,AudioDelay,OutputToAllSpeakers,ResumeTime,Crop,CropLeft,CropRight,CropTop,CropBottom)"
                       " values (%i,%i,%i,%i,%i,%i,%f,%f,%i,%i,%f,%i,%i,%i,%i,%f,%f,",
                       programmeId, setting.m_NonInterleaved, setting.m_NoCache, setting.m_InterlaceMethod, setting.m_FilmGrain, setting.m_ViewMode, 
                       setting.m_CustomZoomAmount, setting.m_CustomPixelRatio, setting.m_AudioStream, setting.m_SubtitleStream, setting.m_SubtitleDelay, 
                       setting.m_SubtitleOn, setting.m_Brightness, setting.m_Contrast, setting.m_Gamma, setting.m_VolumeAmplification, setting.m_AudioDelay);
      CStdString SQL2;
      SQL2=FormatSQL("%i,%i,%i,%i,%i,%i,%i)\n", setting.m_OutputToAllSpeakers, setting.m_ResumeTime, setting.m_Crop, setting.m_CropLeft, setting.m_CropRight,
                    setting.m_CropTop, setting.m_CropBottom);
      SQL += SQL2;
      m_pDS->exec(SQL.c_str());
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, programmeID.c_str());
  }
}

void CTVDatabase::GetChannels(bool freeToAirOnly, VECFILEITEMS &channels)
{
  channels.clear();
  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;

    CStdString SQL=FormatSQL("select * from Channels");
    m_pDS->query( SQL.c_str() );

    while (!m_pDS->eof())
    {
      CFileItemPtr pItem(new CFileItem(m_pDS->fv("Name").get_asString()));
      pItem->SetLabel(m_pDS->fv("Name").get_asString());
      pItem->SetLabel2(m_pDS->fv("Callsign").get_asString());
      pItem->SetProperty("dbID", m_pDS->fv("idChannel").get_asInteger());
      pItem->GetEPGInfoTag()->m_playCount = m_pDS->fv("PlayCount").get_asInteger();
      pItem->SetProperty("HasSettings", m_pDS->fv("HasSettings").get_asBool());
      pItem->m_bIsFolder = true;
      pItem->m_bIsShareOrDrive = false;

      channels.push_back(pItem);

      m_pDS->next();
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
}

bool CTVDatabase::GetProgrammesByChannelName(const CStdString &channel, VECFILEITEMS &programmes, const CDateTime &start, const CDateTime &end)
{
  try
  {
    DWORD time = timeGetTime();
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    long channelId = GetChannelId(channel);
    if (channelId < 0 )
      return false;

    CDateTime     gridStart;
    CDateTime     gridEnd;
    CDateTimeSpan offset;

    gridStart = start;
    CDateTimeSpan hour;
    hour.SetDateTimeSpan(0, 1, 0, 0); /// BST hack
    gridStart -= hour;
    gridEnd = end;

    CStdString SQL=FormatSQL("SELECT Bouquets.Name, Channels.Name, Channels.Number, Categories.Name, Programmes.Title, "
      "GuideData.Subtitle, GuideData.Description, GuideData.StartTime, GuideData.EndTime, GuideData.EpisodeID, GuideData.SeriesID FROM GuideData "
      "JOIN Bouquets ON Bouquets.idBouquet=GuideData.idBouquet "
      "JOIN Programmes ON Programmes.idProgramme=GuideData.idProgramme "
      "JOIN Channels ON Channels.idChannel=GuideData.idChannel "
      "JOIN Categories ON Categories.idCategory=GuideData.idCategory "
      "WHERE GuideData.idChannel = '%u' AND GuideData.EndTime > '%s' AND GuideData.StartTime < '%s' "
      "ORDER BY GuideData.StartTime;", channelId, gridStart.GetAsDBDateTime().c_str(), gridEnd.GetAsDBDateTime().c_str());    
    
    //run query
    //CLog::Log(LOGDEBUG, "%s query: %s", __FUNCTION__, SQL.c_str());
    if (!m_pDS->query(SQL.c_str())) return false;
    int iRowsFound = m_pDS->num_rows();
    if (iRowsFound == 0)
    {
      m_pDS->close();
      return false; /// Don't currently want channels without any guide data
    }

    //CLog::Log(LOGDEBUG,"Time for (initial) SQL query = %ldms", timeGetTime() - time);
    
    time = timeGetTime();
    // get data from returned rows
    programmes.reserve(iRowsFound);
    while (!m_pDS->eof())
    {
      CEPGInfoTag programme = GetUniqueBroadcast(m_pDS);
      CFileItemPtr pItem(new CFileItem(programme));
      FillProperties(pItem.get());
      programmes.push_back(pItem);
      m_pDS->next();
    }

    // check for a very long previous program - first item will start after the requested range
    CDateTime itemStart;
    itemStart = programmes.begin()->get()->GetEPGInfoTag()->m_startTime;
    itemStart -= hour; // back to UTC

    if (itemStart > gridStart) // we missed a program starting before 'offset' /// instead use UniqueBroadcastID and just grab the ID-1
    {
      CLog::Log(LOGDEBUG, "TEMP: Missed a program");
      CStdString SQL=FormatSQL("SELECT bouquets.Name, channels.Name, channels.Number, categories.Name, programmes.Title,"
        "guidedata.Subtitle, guidedata.Description, guidedata.StartTime, guidedata.EndTime, GuideData.EpisodeID, GuideData.SeriesID FROM guidedata "
        "JOIN bouquets ON bouquets.idBouquet=guidedata.idBouquet "
        "JOIN programmes ON programmes.idProgramme=guidedata.idProgramme "
        "JOIN channels ON channels.idChannel=guidedata.idChannel "
        "JOIN categories ON categories.idCategory=guidedata.idCategory "
        "WHERE guidedata.idChannel = '%u' AND GuideData.StartTime <= '%s' "
        "ORDER BY guidedata.StartTime DESC;", channelId, gridStart.GetAsDBDateTime().c_str());
      m_pDS->query( SQL.c_str() );
      if (!m_pDS->eof())
      {
        CEPGInfoTag programme = GetUniqueBroadcast(m_pDS);
        CFileItemPtr pItem(new CFileItem(programme));
        FillProperties(pItem.get());
        programmes.insert(programmes.begin(), pItem); // insert at start of channel data
      }
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
  /* setting noHistory to false returns all available matched */
  if (episodeID.empty())
    return false;
  try
  {
    DWORD time = timeGetTime();
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CDateTime from;

    if (noHistory)
    {
      CDateTimeSpan hour;
      from = CDateTime::GetCurrentDateTime();
      hour.SetDateTimeSpan(0, 1, 0, 0); /// BST hack
      from -= hour;
    }
   

    CStdString SQL= FormatSQL("SELECT Bouquets.Name, Channels.Name, Channels.Number, Categories.Name, Programmes.Title, "
                              "GuideData.Subtitle, GuideData.Description, GuideData.StartTime, GuideData.EndTime, GuideData.EpisodeID, GuideData.SeriesID FROM GuideData "
                              "JOIN Bouquets ON Bouquets.idBouquet=GuideData.idBouquet "
                              "JOIN Programmes ON Programmes.idProgramme=GuideData.idProgramme "
                              "JOIN Channels ON Channels.idChannel=GuideData.idChannel "
                              "JOIN Categories ON Categories.idCategory=GuideData.idCategory "
                              "WHERE GuideData.EpisodeID = '%s' AND GuideData.EndTime > '%s'"
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
      CEPGInfoTag progInfo = GetUniqueBroadcast(m_pDS);
      CFileItemPtr item(new CFileItem(progInfo));
      FillProperties(item.get());
      items->Add(item);
      m_pDS->next();
    }

    CLog::Log(LOGDEBUG,"Time for SQL query = %ldms", timeGetTime() - time);

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
    DWORD time = timeGetTime();
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CDateTime from;

    if (noHistory)
    {
      CDateTimeSpan hour;
      from = CDateTime::GetCurrentDateTime();
      hour.SetDateTimeSpan(0, 1, 0, 0); /// BST hack
      from -= hour;
    }


    CStdString SQL= FormatSQL("SELECT Bouquets.Name, Channels.Name, Channels.Number, Categories.Name, Programmes.Title, "
      "GuideData.Subtitle, GuideData.Description, GuideData.StartTime, GuideData.EndTime, GuideData.EpisodeID, GuideData.SeriesID FROM GuideData "
      "JOIN Bouquets ON Bouquets.idBouquet=GuideData.idBouquet "
      "JOIN Programmes ON Programmes.idProgramme=GuideData.idProgramme "
      "JOIN Channels ON Channels.idChannel=GuideData.idChannel "
      "JOIN Categories ON Categories.idCategory=GuideData.idCategory "
      "WHERE GuideData.SubTitle = '%s' AND GuideData.EndTime > '%s' AND GuideData.EpisodeID = ''"
      "ORDER BY GuideData.StartTime;", subtitle.c_str(), from.GetAsDBDateTime().c_str()); 

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
      CEPGInfoTag progInfo = GetUniqueBroadcast(m_pDS);
      CFileItemPtr item(new CFileItem(progInfo));
      FillProperties(item.get());
      items->Add(item);
      m_pDS->next();
    }

    CLog::Log(LOGDEBUG,"Time for SQL query = %ldms", timeGetTime() - time);

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

long CTVDatabase::GetSourceId(const CStdString& source)
{
	CStdString SQL;
  try
  {
    long sourceId=-1;
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;
    
    SQL=FormatSQL("select idSource from Sources where Callsign like '%s'", source.c_str());
    m_pDS->query(SQL.c_str());
    if (!m_pDS->eof())
      sourceId = m_pDS->fv("Sources.idSource").get_asLong();

    m_pDS->close();
    return sourceId;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s unable to get SourceId (%s)", __FUNCTION__, SQL.c_str());
  }
  return -1;
}

long CTVDatabase::GetBouquetId(const CStdString& bouquet)
{
	CStdString SQL;
  try
  {
    long lBouquetId=-1;
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;
    
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

CEPGInfoTag CTVDatabase::GetUniqueBroadcast(auto_ptr<Dataset> &pDS)
{
  CEPGInfoTag details;

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
  programme->SetProperty("uniqueID", programme->GetEPGInfoTag()->GetID());
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
