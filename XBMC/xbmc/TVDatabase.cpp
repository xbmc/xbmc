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

#include "TVDatabase.h"
#include "utils/GUIInfoManager.h"
#include "utils/log.h"

using namespace std;

using namespace dbiplus;

#define TV_DATABASE_VERSION 4
#define TV_DATABASE_OLD_VERSION 3
#define TV_DATABASE_NAME "MyTV1.db"

CTVDatabase::CTVDatabase(void)
{
  m_preV2version      = TV_DATABASE_OLD_VERSION;
  m_version           = TV_DATABASE_VERSION;
  m_strDatabaseFile   = TV_DATABASE_NAME;
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
    m_pDS->exec("CREATE TABLE Clients (idClient integer primary key, Name text, GUID text)\n");

    CLog::Log(LOGINFO, "TV: Creating Last Channel table");
    m_pDS->exec("CREATE TABLE LastChannel (idClient integer, idChannel integer primary key, Number integer, Name text)\n");

    CLog::Log(LOGINFO, "TV: Creating Channels table");
    m_pDS->exec("CREATE TABLE Channels (idClient integer, idChannel integer primary key,"
                "XBMCNumber integer, Name text, ClientName text, ClientNumber integer,"
                "UniqueId integer, IconPath text, GroupID integer, encrypted bool,"
                "radio bool, hide bool, strFileNameAndPath text)\n");

    CLog::Log(LOGINFO, "TV: Creating GuideData table");
    m_pDS->exec("CREATE TABLE GuideData (idClient integer, idBouquet integer, idChannel integer, strChannel text, "
                "idProgramme integer, strTitle text, strOriginalTitle text, strPlotOutline text, "
                "strPlot text, strGenre text, StartTime datetime, EndTime datetime, strExtra text, "
                "strFileNameAndPath text, commFree bool, isRecording bool, "
                "GenreType integer, GenreSubType integer, firstAired datetime, "
                "repeat bool, AutoSwitch integer, idUniqueBroadcast integer primary key)\n");
    m_pDS->exec("CREATE UNIQUE INDEX idx_UniqueBroadcast on GuideData(idProgramme, idChannel, idBouquet, StartTime desc)\n"); /// pointless?

    CLog::Log(LOGINFO, "TV: Creating ChannelSettings table");
    m_pDS->exec("CREATE TABLE ChannelSettings ( idClient integer, idChannel integer primary key, Deinterlace bool, "
                "ViewMode integer, ZoomAmount float, PixelRatio float, AudioStream integer, SubtitleStream integer, "
                "SubtitleDelay float, SubtitlesOn bool, Brightness integer, Contrast integer, Gamma integer, "
                "VolumeAmplification float, AudioDelay float, OutputToAllSpeakers bool, ResumeTime integer, Crop bool, CropLeft integer, "
                "CropRight integer, CropTop integer, CropBottom integer)\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_ChannelSettings ON ChannelSettings (idChannel)\n");

    CLog::Log(LOGINFO, "TV: Creating Groups table");
    m_pDS->exec("CREATE TABLE Groups (idGroup integer primary key, idClient integer, Name text)\n");
    m_pDS->exec("CREATE UNIQUE INDEX ix_grouplinkClient ON Groups (idGroup, idClient)\n");

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
  {
    // number of items in the db has likely changed, so reset the infomanager cache
    g_infoManager.ResetPersistentCache(); /// what is this
    return true;
  }

  return false;
}

int CTVDatabase::GetLastChannel(DWORD clientID)
{
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    CStdString SQL=FormatSQL("select * from LastChannel WHERE LastChannel.idClient = '%u'", clientID);

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

bool CTVDatabase::UpdateLastChannel(DWORD clientID, unsigned int channelID, CStdString m_strChannel)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    if (channelID < 0)   // no match found, update required
    {
      return false;
    }

    CStdString SQL;

    SQL=FormatSQL("select * from LastChannel WHERE LastChannel.idChannel = '%u' AND LastChannel.idClient = '%u'", channelID, clientID);
    m_pDS->query(SQL.c_str());

    if (m_pDS->num_rows() > 0)
    {
      m_pDS->close();
      // update the item
      CStdString SQL=FormatSQL("update LastChannel set idClient=%i,Number=%i,Name='%s' where idChannel=%i",
                               clientID, channelID, m_strChannel.c_str(), channelID);

      m_pDS->exec(SQL.c_str());
      return true;
    }
    else   // add the items
    {
      m_pDS->close();
      SQL=FormatSQL("insert into LastChannel ( idClient,idChannel,Number,Name)"
                    " values ('%i','%i','%i','%s')\n",
                    clientID, channelID, channelID, m_strChannel.c_str());
      m_pDS->exec(SQL.c_str());
      return true;
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%i) failed", __FUNCTION__, channelID);
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
      clientId = (long)sqlite3_last_insert_rowid(m_pDB->getHandle());

      CLog::Log(LOGNOTICE, "TVDatabase: Added new PVR Client ID '%i' with Name '%s' and GUID '%s'", clientId, client.c_str(), guid.c_str());
    }

    return clientId;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (Name: %s, GUID: %s) failed", __FUNCTION__, client.c_str(), guid.c_str());
  }

  return -1;
}

long CTVDatabase::AddDBChannel(const cPVRChannelInfoTag &info)
{
  try
  {
    if (info.ClientID() < 0) return -1;
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    long channelId = info.m_iIdChannel;

    if (channelId < 0)
    {
      CStdString SQL = FormatSQL("insert into Channels (idClient, idChannel, XBMCNumber, Name, ClientName,"
                                 "ClientNumber, UniqueId, IconPath, GroupID, encrypted, radio, hide, strFileNameAndPath) "
                                 "values ('%i', NULL, '%i', '%s', '%s', '%i', '%i', '%s', '%i', '%i', '%i', '%i', '%s')\n",
                                 info.ClientID(), info.Number(), info.Name().c_str(), info.ClientName().c_str(),
                                 info.ClientNumber(), info.UniqueID(), info.m_IconPath.c_str(), info.m_iGroupID,
                                 info.m_encrypted, info.m_radio, info.m_hide, info.m_strFileNameAndPath.c_str());

      m_pDS->exec(SQL.c_str());
      channelId = (long)sqlite3_last_insert_rowid(m_pDB->getHandle());
    }

    return channelId;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, info.m_strChannel.c_str());
  }

  return -1;
}

bool CTVDatabase::RemoveAllChannels(DWORD clientID)
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

    if (channelId < 0)   // no match found, update required
    {
      return false;
    }

    CStdString SQL;

    SQL=FormatSQL("select * from Channels WHERE Channels.idChannel = '%u'", channelId);
    m_pDS->query(SQL.c_str());

    if (m_pDS->num_rows() > 0)
    {
      m_pDS->close();
      // update the item
      SQL = FormatSQL("update Channels set idClient=%i,XBMCNumber=%i,Name='%s',ClientName='%s',"
                      "ClientNumber=%i,UniqueId=%i,IconPath='%s',GroupID=%i,encrypted=%i,radio=%i,"
                      "hide=%i,strFileNameAndPath='%s' where idChannel=%i",
                      info.ClientID(), info.Number(), info.Name().c_str(), info.ClientName().c_str(),
                      info.ClientNumber(), info.UniqueID(), info.m_IconPath.c_str(), info.m_iGroupID,
                      info.m_encrypted, info.m_radio, info.m_hide, info.m_strFileNameAndPath.c_str(),
                      channelId);

      m_pDS->exec(SQL.c_str());
      return channelId;
    }
    else   // add the items
    {
      m_pDS->close();
      SQL = FormatSQL("insert into Channels (idClient, idChannel, XBMCNumber, Name, ClientName,"
                      "ClientNumber, UniqueId, IconPath, GroupID, encrypted, radio, hide, strFileNameAndPath) "
                      "values ('%i', NULL, '%i', '%s', '%s', '%i', '%i', '%s', '%i', '%i', '%i', '%i', '%s')\n",
                      info.ClientID(), info.Number(), info.Name().c_str(), info.ClientName().c_str(),
                      info.ClientNumber(), info.UniqueID(), info.m_IconPath.c_str(), info.m_iGroupID,
                      info.m_encrypted, info.m_radio, info.m_hide, info.m_strFileNameAndPath.c_str());

      m_pDS->exec(SQL.c_str());
      channelId = (long)sqlite3_last_insert_rowid(m_pDB->getHandle());
      return channelId;
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, info.m_strChannel.c_str());
    return false;
  }
}

bool CTVDatabase::HasChannel(DWORD clientID, const cPVRChannelInfoTag &info)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString SQL=FormatSQL("select * from Channels WHERE Channels.Name = '%s' AND Channels.ClientNumber = '%i' AND Channels.idClient = '%u'", info.m_strChannel.c_str(), info.m_iClientNum, clientID);

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

    CStdString SQL=FormatSQL("select * from Channels WHERE Channels.radio=%u ORDER BY Channels.XBMCNumber", radio);

    m_pDS->query(SQL.c_str());

    while (!m_pDS->eof())
    {
      cPVRChannelInfoTag broadcast;

      broadcast.m_clientID            = m_pDS->fv("idClient").get_asInt();
      broadcast.m_iIdChannel          = m_pDS->fv("idChannel").get_asInt();
      broadcast.m_iChannelNum         = m_pDS->fv("XBMCNumber").get_asInt();
      broadcast.m_strChannel          = m_pDS->fv("Name").get_asString();
      broadcast.m_strClientName       = m_pDS->fv("ClientName").get_asString();
      broadcast.m_iClientNum          = m_pDS->fv("ClientNumber").get_asInt();
      broadcast.m_iIdUnique           = m_pDS->fv("UniqueId").get_asInt();
      broadcast.m_IconPath            = m_pDS->fv("IconPath").get_asString();
      broadcast.m_iGroupID            = m_pDS->fv("GroupID").get_asInt();
      broadcast.m_encrypted           = m_pDS->fv("encrypted").get_asBool();
      broadcast.m_radio               = m_pDS->fv("radio").get_asBool();
      broadcast.m_hide                = m_pDS->fv("hide").get_asBool();
      broadcast.m_strFileNameAndPath  = m_pDS->fv("strFileNameAndPath").get_asString();

      results.push_back(broadcast);
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

bool CTVDatabase::GetChannelSettings(DWORD clientID, unsigned int channelID, CVideoSettings &settings)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;
    if (channelID < 0) return false;

    CStdString SQL=FormatSQL("select * from ChannelSettings where idChannel like '%u'", channelID);

    m_pDS->query(SQL.c_str());

    if (m_pDS->num_rows() > 0)
    {
      // get the channel settings info
      settings.m_AudioDelay           = m_pDS->fv("AudioDelay").get_asFloat();
      settings.m_AudioStream          = m_pDS->fv("AudioStream").get_asInt();
      settings.m_Brightness           = m_pDS->fv("Brightness").get_asInt();
      settings.m_Contrast             = m_pDS->fv("Contrast").get_asInt();
      settings.m_CustomPixelRatio     = m_pDS->fv("PixelRatio").get_asFloat();
      settings.m_CustomZoomAmount     = m_pDS->fv("ZoomAmount").get_asFloat();
      settings.m_Gamma                = m_pDS->fv("Gamma").get_asInt();
      settings.m_SubtitleDelay        = m_pDS->fv("SubtitleDelay").get_asFloat();
      settings.m_SubtitleOn           = m_pDS->fv("SubtitlesOn").get_asBool();
      settings.m_SubtitleStream       = m_pDS->fv("SubtitleStream").get_asInt();
      settings.m_ViewMode             = m_pDS->fv("ViewMode").get_asInt();
      settings.m_ResumeTime           = m_pDS->fv("ResumeTime").get_asInt();
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

bool CTVDatabase::SetChannelSettings(DWORD clientID, unsigned int channelID, const CVideoSettings &settings)
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

    CStdString SQL;

    SQL.Format("select * from ChannelSettings where idChannel=%i", channelID);
    m_pDS->query(SQL.c_str());

    if (m_pDS->num_rows() > 0)
    {
      m_pDS->close();
      // update the item
      SQL=FormatSQL("update ChannelSettings set Deinterlace=%i,ViewMode=%i,ZoomAmount=%f,PixelRatio=%f,"
                    "AudioStream=%i,SubtitleStream=%i,SubtitleDelay=%f,SubtitlesOn=%i,Brightness=%i,Contrast=%i,Gamma=%i,"
                    "VolumeAmplification=%f,AudioDelay=%f,OutputToAllSpeakers=%i,",
                    settings.m_InterlaceMethod, settings.m_ViewMode,
                    settings.m_CustomZoomAmount, settings.m_CustomPixelRatio, settings.m_AudioStream, settings.m_SubtitleStream, settings.m_SubtitleDelay,
                    settings.m_SubtitleOn, settings.m_Brightness, settings.m_Contrast, settings.m_Gamma, settings.m_VolumeAmplification, settings.m_AudioDelay,
                    settings.m_OutputToAllSpeakers);
      CStdString SQL2;
      SQL2=FormatSQL("ResumeTime=%i,Crop=%i,CropLeft=%i,CropRight=%i,CropTop=%i,CropBottom=%i where idChannel=%i\n", settings.m_ResumeTime,
                     settings.m_Crop, settings.m_CropLeft, settings.m_CropRight, settings.m_CropTop, settings.m_CropBottom, channelID);
      SQL += SQL2;
      m_pDS->exec(SQL.c_str());
      return true;
    }
    else
    {
      // add the items
      m_pDS->close();
      SQL=FormatSQL("insert into ChannelSettings ( idChannel,Deinterlace,ViewMode,ZoomAmount,PixelRatio,"
                    "AudioStream,SubtitleStream,SubtitleDelay,SubtitlesOn,Brightness,Contrast,Gamma,"
                    "VolumeAmplification,AudioDelay,OutputToAllSpeakers,ResumeTime,Crop,CropLeft,CropRight,CropTop,CropBottom)"
                    " values (%i,%i,%i,%i,%i,%i,%f,%f,%i,%i,%f,%i,%i,%i,%i,%f,%f,",
                    channelID, settings.m_InterlaceMethod, settings.m_ViewMode,
                    settings.m_CustomZoomAmount, settings.m_CustomPixelRatio, settings.m_AudioStream, settings.m_SubtitleStream, settings.m_SubtitleDelay,
                    settings.m_SubtitleOn, settings.m_Brightness, settings.m_Contrast, settings.m_Gamma, settings.m_VolumeAmplification, settings.m_AudioDelay);
      CStdString SQL2;
      SQL2=FormatSQL("%i,%i,%i,%i,%i,%i,%i)\n", settings.m_OutputToAllSpeakers, settings.m_ResumeTime, settings.m_Crop, settings.m_CropLeft, settings.m_CropRight,
                     settings.m_CropTop, settings.m_CropBottom);
      SQL += SQL2;
      m_pDS->exec(SQL.c_str());
      return true;
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (ID=%i) failed", __FUNCTION__, channelID);
    return false;
  }
}

long CTVDatabase::AddGroup(const CStdString &groupname)
{
  try
  {
    if (groupname == "") return -1;
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    long groupId;
    groupId = GetGroupId(groupname);
    if (groupId < 0)
    {
      CStdString SQL=FormatSQL("insert into Groups (idClient, idGroup, Name) values (NULL, NULL, '%s')", groupname.c_str());
      m_pDS->exec(SQL.c_str());
      groupId = (long)sqlite3_last_insert_rowid(m_pDB->getHandle());
    }

    return groupId;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, groupname.c_str());
  }
  return -1;
}

bool CTVDatabase::DeleteGroup(unsigned int groupID)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    if (groupID < 0)   // no match found, update required
    {
      return false;
    }

    CStdString strSQL=FormatSQL("delete from Groups WHERE Groups.idGroup = '%u'", groupID);

    m_pDS->exec(strSQL.c_str());
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%i) failed", __FUNCTION__, groupID);
    return false;
  }
}

bool CTVDatabase::RenameGroup(unsigned int GroupId, const CStdString &newname)
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

    SQL=FormatSQL("select * from Groups WHERE Groups.idGroup = '%u'", GroupId);
    m_pDS->query(SQL.c_str());

    if (m_pDS->num_rows() > 0)
    {
      m_pDS->close();
      // update the item
      CStdString SQL = FormatSQL("update Groups set Name='%s' WHERE idGroup=%i", newname.c_str(), GroupId);
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

bool CTVDatabase::GetGroupList(cPVRChannelGroups &results)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    CStdString SQL=FormatSQL("select * from Groups ORDER BY Groups.Name");

    m_pDS->query(SQL.c_str());

    while (!m_pDS->eof())
    {
      cPVRChannelGroup data;

      data.SetGroupID(m_pDS->fv("idGroup").get_asInt());
      data.SetGroupName(m_pDS->fv("Name").get_asString());

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

long CTVDatabase::GetGroupId(const CStdString &groupname)
{
  CStdString SQL;

  try
  {
    long lGroupId=-1;
    if (NULL == m_pDB.get()) return lGroupId;
    if (NULL == m_pDS.get()) return lGroupId;

    SQL=FormatSQL("select idGroup from Groups where Name like '%s'", groupname.c_str());
    m_pDS->query(SQL.c_str());
    if (!m_pDS->eof())
      lGroupId = m_pDS->fv("Groups.idGroup").get_asInt();

    m_pDS->close();
    return lGroupId;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s unable to get GroupId (%s)", __FUNCTION__, SQL.c_str());
  }
  return -1;
}
