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
  m_dataEnd = NULL;
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

    CLog::Log(LOGINFO, "TV: Creating Sources table");
    m_pDS->exec("CREATE TABLE Sources (idSource integer primary key, CallSign text, Name text)\n");

    CLog::Log(LOGINFO, "TV: Creating categories table");
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
                "idCategory integer, ShortDesc text, LongDesc text, Episode text, Series text, StartTime datetime,"
                "Duration integer, AirDate datetime)"
                /*"StreamPIDs text, StreamTypes text, StreamNames text)*/"\n");
    m_pDS->exec("CREATE UNIQUE INDEX idx_UniqueBroadcast on GuideData(idProgramme, idChannel, idBouquet, StartTime desc)\n");
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

bool CTVDatabase::FillEPG(const CStdString &source, const CStdString &bouquet, const CStdString &channel, const int &channum, const CStdString &progTitle, const CStdString &progShortDesc,
                          const CStdString &progLongDesc, const CStdString &episode, const CStdString &series, const CDateTime &progStartTime, const int &progDuration, const CDateTime &progAirDate, const CStdString &category)
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

    long channelId = AddChannel(sourceId, bouquetId, "callsign", channel, channum);
    if (channelId < 0)
      return false;

    long categoryId = AddCategory(category);
    if (categoryId < 0)
      return false;

    long programmeId = AddProgramme(progTitle, categoryId);
    if (programmeId < 0)
      return false;

    CStdString SQL=FormatSQL("insert into GuideData (idSource, idBouquet, idChannel, idProgramme, idCategory,"
                             "ShortDesc, LongDesc, Episode, Series, StartTime, Duration, AirDate)"
                             "values ('%u', '%u', '%u', '%u', '%u', '%s', '%s', '%s', '%s', '%s', '%u', '%s')",
                             sourceId, bouquetId, channelId, programmeId, categoryId, progShortDesc.c_str(), progLongDesc.c_str(), episode.c_str(), 
                             series.c_str(), progStartTime.GetAsDBDateTime().c_str(), progDuration, progAirDate.GetAsDBDateTime().c_str());
    m_pDS->exec(SQL.c_str());
    long progId = (long)sqlite3_last_insert_rowid(m_pDB->getHandle());

    if (m_dataEnd < progStartTime)
    {
      CDateTimeSpan length;
      length.SetDateTimeSpan(0, 0, 0, progDuration);
      m_dataEnd = progStartTime + length;
    }
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

void CTVDatabase::AddToLinkTable(const char *table, const char *firstField, long firstID, const char *secondField, long secondID)
{
  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    CStdString SQL=FormatSQL("select * from %s where %s=%u and %s=%u", table, firstField, firstID, secondField, secondID);
    m_pDS->query(SQL.c_str());
    if (m_pDS->num_rows() == 0)
    {
      // doesnt exists, add it
      SQL=FormatSQL("insert into %s (%s,%s) values(%i,%i)", table, firstField, secondField, firstID, secondID);
      m_pDS->exec(SQL.c_str());
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
}


CDateTime CTVDatabase::GetDataEnd()
{
  CDateTime utc = NULL;
  try
  {
    if (NULL == m_pDB.get()) return utc;
    if (NULL == m_pDS.get()) return utc;

    CStdString SQL=FormatSQL("SELECT guidedata.StartTime, guidedata.Duration FROM guidedata "
                             "ORDER BY guidedata.StartTime DESC;");
    m_pDS->query( SQL.c_str() );
    if (!m_pDS->eof())
    {
      utc.SetFromDBDateTime(m_pDS->fv("StartTime").get_asString());
      CDateTimeSpan duration;
      duration.SetDateTimeSpan(0, 0, 0, m_pDS->fv("Duration").get_asInteger());
      utc += duration;
    }
    m_pDS->close();
    return utc;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
    return NULL;
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

void CTVDatabase::GetAllChannels(bool freeToAirOnly, VECTVCHANNELS &channels)
{
  try
  {
    channels.erase(channels.begin(), channels.end());
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;

    CStdString SQL=FormatSQL("select * from Channels");
    m_pDS->query( SQL.c_str() );

    while (!m_pDS->eof())
    {
      CFileItemPtr pItem(new CFileItem(m_pDS->fv("Name").get_asString()));
      pItem->SetLabel2(m_pDS->fv("Callsign").get_asString());
      /*pItem->SetIconImage(m_pDS->fv("Logo").get_asString());*/
      pItem->SetProperty("Number", m_pDS->fv("Number").get_asInteger());
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

bool CTVDatabase::GetShowsByChannel(const CStdString &channel, VECTVSHOWS &shows, const CDateTime &start, const CDateTime &end)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    long channelId = GetChannelId(channel);
    if (channelId < 0 )
      return false; ///TODO

    CDateTime     gridStart;
    CDateTime     gridEnd;
    CDateTimeSpan offset;

    gridStart = start;
    offset.SetDateTimeSpan(0, 1, 0, 0); // BST hack
    gridStart -= offset;
    gridEnd = end;

    CStdString SQL=FormatSQL("SELECT bouquets.Name, channels.Name, channels.Number, categories.Name, programmes.Title,"
      "guidedata.ShortDesc, guidedata.LongDesc, guidedata.StartTime, guidedata.Duration FROM guidedata "
      "JOIN bouquets ON bouquets.idBouquet=guidedata.idBouquet "
      "JOIN programmes ON programmes.idProgramme=guidedata.idProgramme "
      "JOIN channels ON channels.idChannel=guidedata.idChannel "
      "JOIN categories ON categories.idCategory=guidedata.idCategory "
      "WHERE guidedata.idChannel = '%u' AND GuideData.StartTime >= '%s' AND GuideData.StartTime <= '%s' "
      "ORDER BY guidedata.StartTime;", channelId, gridStart.GetAsDBDateTime().c_str(), gridEnd.GetAsDBDateTime().c_str());
    m_pDS->query( SQL.c_str() );

    CDateTimeSpan hour;
    hour.SetDateTimeSpan(0, 1, 0, 0); // BST hack

    bool hasdata = false;
    while (!m_pDS->eof())
    {
      hasdata = true;
      CFileItemPtr pItem(new CFileItem(m_pDS->fv("Title").get_asString()));
      pItem->SetProperty("ShortDesc", m_pDS->fv("ShortDesc").get_asString());
      pItem->SetProperty("LongDesc", m_pDS->fv("LongDesc").get_asString());
      /*pItem->SetProperty("Category", m_pDS->fv("LongDesc").get_asString());*/
      pItem->SetProperty("ChannelId", channelId);
      CDateTime utc;
      utc.SetFromDBDateTime(m_pDS->fv("StartTime").get_asString());
      utc += hour;
      pItem->SetProperty("StartTime", utc.GetAsDBDateTime());
      pItem->SetProperty("Duration", m_pDS->fv("Duration").get_asInteger());
      shows.push_back(pItem);
      m_pDS->next();
    }
    m_pDS->close();
    
    if (!hasdata)
      return false;

    // check for a very long previous program
    CDateTime itemStart;
    itemStart.SetFromDBDateTime(shows.begin()->get()->GetProperty("StartTime"));
    itemStart -= hour; // back to UTC

    if (itemStart > gridStart) // we missed a program starting before 'offset'
    {
      CLog::Log(LOGDEBUG, "TEMP: Missed a program");
      CStdString SQL=FormatSQL("SELECT bouquets.Name, channels.Name, channels.Number, categories.Name, programmes.Title,"
        "guidedata.ShortDesc, guidedata.LongDesc, guidedata.StartTime, guidedata.Duration FROM guidedata "
        "JOIN bouquets ON bouquets.idBouquet=guidedata.idBouquet "
        "JOIN programmes ON programmes.idProgramme=guidedata.idProgramme "
        "JOIN channels ON channels.idChannel=guidedata.idChannel "
        "JOIN categories ON categories.idCategory=guidedata.idCategory "
        "WHERE guidedata.idChannel = '%u' AND GuideData.StartTime <= '%s' "
        "ORDER BY guidedata.StartTime DESC;", channelId, gridStart.GetAsDBDateTime().c_str());
      m_pDS->query( SQL.c_str() );
      if (!m_pDS->eof())
      {
        CFileItemPtr pItem(new CFileItem(m_pDS->fv("Title").get_asString()));
        pItem->SetProperty("ShortDesc", m_pDS->fv("ShortDesc").get_asString());
        pItem->SetProperty("LongDesc", m_pDS->fv("LongDesc").get_asString());
        /*pItem->SetProperty("Category", m_pDS->fv("LongDesc").get_asString());*/
        pItem->SetProperty("ChannelId", channelId);
        CDateTime utc;
        utc.SetFromDBDateTime(m_pDS->fv("StartTime").get_asString());
        CDateTimeSpan hour;
        hour.SetDateTimeSpan(0, 1, 0, 0); // BST hack
        utc += hour;
        pItem->SetProperty("StartTime", utc.GetAsDBDateTime());
        pItem->SetProperty("Duration", m_pDS->fv("Duration").get_asInteger());
        shows.insert(shows.begin(), pItem); // insert at start of channel data
      }
      m_pDS->close();
    }
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, channel);
  }
  return false;
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