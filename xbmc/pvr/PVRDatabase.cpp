/*
 *      Copyright (C) 2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "PVRDatabase.h"
#include "dbwrappers/dataset.h"
#include "settings/AdvancedSettings.h"
#include "settings/VideoSettings.h"
#include "utils/log.h"

#include "PVRManager.h"
#include "channels/PVRChannelGroupsContainer.h"
#include "channels/PVRChannelGroupInternal.h"
#include "addons/PVRClient.h"

using namespace std;
using namespace dbiplus;
using namespace PVR;
using namespace ADDON;

#define PVRDB_DEBUGGING 0

bool CPVRDatabase::Open()
{
  return CDatabase::Open(g_advancedSettings.m_databaseTV);
}

bool CPVRDatabase::CreateTables()
{
  bool bReturn(false);

  try
  {
    if (!CDatabase::CreateTables())
      return false;

    BeginTransaction();
    CLog::Log(LOGINFO, "PVR - %s - creating tables", __FUNCTION__);

    CLog::Log(LOGDEBUG, "PVR - %s - creating table 'clients'", __FUNCTION__);
    m_pDS->exec(
        "CREATE TABLE clients ("
          "idClient integer primary key, "
          "sName    varchar(64), "
          "sUid     varchar(32)"
        ")"
    );

    CLog::Log(LOGDEBUG, "PVR - %s - creating table 'channels'", __FUNCTION__);
    m_pDS->exec(
        "CREATE TABLE channels ("
          "idChannel            integer primary key, "
          "iUniqueId            integer, "
          "bIsRadio             bool, "
          "bIsHidden            bool, "
          "bIsUserSetIcon       bool, "
          "bIsLocked            bool, "
          "sIconPath            varchar(255), "
          "sChannelName         varchar(64), "
          "bIsVirtual           bool, "
          "bEPGEnabled          bool, "
          "sEPGScraper          varchar(32), "
          "iLastWatched         integer,"

          // TODO use mapping table
          "iClientId            integer, "
          "iClientChannelNumber integer, "
          "sInputFormat         varchar(32), "
          "sStreamURL           varchar(255), "
          "iEncryptionSystem    integer, "

          "idEpg                integer"
        ")"
    );
    m_pDS->exec("CREATE UNIQUE INDEX idx_channels_iClientId_iUniqueId on channels(iClientId, iUniqueId);");

    // TODO use a mapping table so multiple backends per channel can be implemented
    //    CLog::Log(LOGDEBUG, "PVR - %s - creating table 'map_channels_clients'", __FUNCTION__);
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

    CLog::Log(LOGDEBUG, "PVR - %s - creating table 'channelgroups'", __FUNCTION__);
    m_pDS->exec(
        "CREATE TABLE channelgroups ("
          "idGroup         integer primary key,"
          "bIsRadio        bool, "
          "iGroupType      integer, "
          "sName           varchar(64)"
        ")"
    );
    m_pDS->exec("CREATE INDEX idx_channelgroups_bIsRadio on channelgroups(bIsRadio);");

    CLog::Log(LOGDEBUG, "PVR - %s - creating table 'map_channelgroups_channels'", __FUNCTION__);
    m_pDS->exec(
        "CREATE TABLE map_channelgroups_channels ("
          "idChannel      integer, "
          "idGroup        integer, "
          "iChannelNumber integer"
        ")"
    );
    m_pDS->exec("CREATE UNIQUE INDEX idx_idGroup_idChannel on map_channelgroups_channels(idGroup, idChannel);");

    CLog::Log(LOGDEBUG, "PVR - %s - creating table 'channelsettings'", __FUNCTION__);
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
          "fNoiseReduction      float, "
          "fCustomVerticalShift float, "
          "bCustomNonLinStretch bool, "
          "bPostProcess         bool, "
          "iScalingMethod       integer, "
          "iDeinterlaceMode     integer "
        ")"
    );

    CommitTransaction();
    bReturn = true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "PVR - %s - unable to create PVR database tables (error %i)", __FUNCTION__, (int)GetLastError());
    RollbackTransaction();
    bReturn = false;
  }

  if (bReturn)
  {
    // disable all PVR add-on when started the first time
    ADDON::VECADDONS addons;
    if ((bReturn = CAddonMgr::Get().GetAddons(ADDON_PVRDLL, addons, true)) == false)
      CLog::Log(LOGERROR, "PVR - %s - failed to get add-ons from the add-on manager", __FUNCTION__);
    else
    {
      CAddonDatabase database;
      database.Open();
      for (IVECADDONS it = addons.begin(); it != addons.end(); it++)
        database.DisableAddon(it->get()->ID());
      database.Close();
    }
  }

  return bReturn;
}

bool CPVRDatabase::UpdateOldVersion(int iVersion)
{
  bool bReturn = true;

  BeginTransaction();

  try
  {
    if (iVersion < 11)
    {
      CLog::Log(LOGERROR, "PVR - %s - updating from table versions < 11 not supported. please delete '%s'",
          __FUNCTION__, GetBaseDBName());
      bReturn = false;
    }
    else
    {
      if (iVersion < 12)
        m_pDS->exec("DROP VIEW vw_last_watched;");

      if (iVersion < 13)
        m_pDS->exec("ALTER TABLE channels ADD idEpg integer;");

      if (iVersion < 14)
        m_pDS->exec("ALTER TABLE channelsettings ADD fCustomVerticalShift float;");

      if (iVersion < 15)
      {
        m_pDS->exec("ALTER TABLE channelsettings ADD bCustomNonLinStretch bool;");
        m_pDS->exec("ALTER TABLE channelsettings ADD bPostProcess bool;");
        m_pDS->exec("ALTER TABLE channelsettings ADD iScalingMethod integer;");
      }
      if (iVersion < 16)
      {
        /* sqlite apparently can't delete columns from an existing table, so just leave the extra column alone */
      }
      if (iVersion < 17)
      {
        m_pDS->exec("ALTER TABLE channelsettings ADD iDeinterlaceMode integer");
        m_pDS->exec("UPDATE channelsettings SET iDeinterlaceMode = 2 WHERE iInterlaceMethod NOT IN (0,1)"); // anything other than none: method auto => mode force
        m_pDS->exec("UPDATE channelsettings SET iDeinterlaceMode = 1 WHERE iInterlaceMethod = 1"); // method auto => mode auto
        m_pDS->exec("UPDATE channelsettings SET iDeinterlaceMode = 0, iInterlaceMethod = 1 WHERE iInterlaceMethod = 0"); // method none => mode off, method auto
      }
      if (iVersion < 18)
      {
        m_pDS->exec("DROP INDEX idx_channels_iClientId;");
        m_pDS->exec("DROP INDEX idx_channels_iLastWatched;");
        m_pDS->exec("DROP INDEX idx_channels_bIsRadio;");
        m_pDS->exec("DROP INDEX idx_channels_bIsHidden;");
        m_pDS->exec("DROP INDEX idx_idChannel_idGroup;");
        m_pDS->exec("DROP INDEX idx_idGroup_iChannelNumber;");
        m_pDS->exec("CREATE UNIQUE INDEX idx_channels_iClientId_iUniqueId on channels(iClientId, iUniqueId);");
        m_pDS->exec("CREATE UNIQUE INDEX idx_idGroup_idChannel on map_channelgroups_channels(idGroup, idChannel);");
      }
      if (iVersion < 19)
      {
        // bit of a hack, but we need to keep the version/contents of the non-pvr databases the same to allow clean upgrades
        ADDON::VECADDONS addons;
        if ((bReturn = CAddonMgr::Get().GetAddons(ADDON_PVRDLL, addons, true)) == false)
          CLog::Log(LOGERROR, "PVR - %s - failed to get add-ons from the add-on manager", __FUNCTION__);
        else
        {
          CAddonDatabase database;
          database.Open();
          for (IVECADDONS it = addons.begin(); it != addons.end(); it++)
          {
            if (!database.IsSystemPVRAddonEnabled(it->get()->ID()))
              database.DisableAddon(it->get()->ID());
          }
          database.Close();
        }
      }
      if (iVersion < 20)
        m_pDS->exec("ALTER TABLE channels ADD bIsUserSetIcon bool");

      if (iVersion < 21)
        m_pDS->exec("ALTER TABLE channelgroups ADD iGroupType integer");

      if (iVersion < 22)
        m_pDS->exec("ALTER TABLE channels ADD bIsLocked bool");
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "PVR - %s - error attempting to update the database version!", __FUNCTION__);
    bReturn = false;
  }

  if (bReturn)
    CommitTransaction();
  else
    RollbackTransaction();

  return bReturn;
}

int CPVRDatabase::GetLastChannelId(void)
{
  int iReturn(0);

  CStdString strQuery = FormatSQL("SELECT MAX(idChannel) as iMaxChannel FROM channels");
  if (ResultQuery(strQuery))
  {
    try
    {
      if (!m_pDS->eof())
        iReturn = m_pDS->fv("iMaxChannel").get_asInt();
    }
    catch (...) {}
  }

  return iReturn;
}

/********** Channel methods **********/

bool CPVRDatabase::DeleteChannels(void)
{
  CLog::Log(LOGDEBUG, "PVR - %s - deleting all channels from the database", __FUNCTION__);
  return DeleteValues("channels");
}

bool CPVRDatabase::DeleteClientChannels(const CPVRClient &client)
{
  /* invalid client Id */
  if (client.GetID() <= 0)
  {
    CLog::Log(LOGERROR, "PVR - %s - invalid client id: %i", __FUNCTION__, client.GetID());
    return false;
  }

  CLog::Log(LOGDEBUG, "PVR - %s - deleting all channels from client '%i' from the database", __FUNCTION__, client.GetID());
  CStdString strWhereClause = FormatSQL("iClientId = %u", client.GetID());
  return DeleteValues("channels", strWhereClause);
}

bool CPVRDatabase::Delete(const CPVRChannel &channel)
{
  /* invalid channel */
  if (channel.ChannelID() <= 0)
    return false;

  CLog::Log(LOGDEBUG, "PVR - %s - deleting channel '%s' from the database", __FUNCTION__, channel.ChannelName().c_str());
  CStdString strWhereClause = FormatSQL("idChannel = %u", channel.ChannelID());
  return DeleteValues("channels", strWhereClause);
}

int CPVRDatabase::Get(CPVRChannelGroupInternal &results)
{
  int iReturn(0);

  CStdString strQuery = FormatSQL("SELECT channels.idChannel, channels.iUniqueId, channels.bIsRadio, channels.bIsHidden, channels.bIsUserSetIcon, "
      "channels.sIconPath, channels.sChannelName, channels.bIsVirtual, channels.bEPGEnabled, channels.sEPGScraper, channels.iLastWatched, channels.iClientId, channels.bIsLocked, "
      "channels.iClientChannelNumber, channels.sInputFormat, channels.sInputFormat, channels.sStreamURL, channels.iEncryptionSystem, map_channelgroups_channels.iChannelNumber, channels.idEpg "
      "FROM map_channelgroups_channels "
      "LEFT JOIN channels ON channels.idChannel = map_channelgroups_channels.idChannel "
      "WHERE map_channelgroups_channels.idGroup = %u", results.IsRadio() ? XBMC_INTERNAL_GROUP_RADIO : XBMC_INTERNAL_GROUP_TV);
  if (ResultQuery(strQuery))
  {
    try
    {
      while (!m_pDS->eof())
      {
        CPVRChannelPtr channel = CPVRChannelPtr(new CPVRChannel());

        channel->m_iChannelId              = m_pDS->fv("idChannel").get_asInt();
        channel->m_iUniqueId               = m_pDS->fv("iUniqueId").get_asInt();
        channel->m_bIsRadio                = m_pDS->fv("bIsRadio").get_asBool();
        channel->m_bIsHidden               = m_pDS->fv("bIsHidden").get_asBool();
        channel->m_bIsUserSetIcon          = m_pDS->fv("bIsUserSetIcon").get_asBool();
        channel->m_bIsLocked               = m_pDS->fv("bIsLocked").get_asBool();
        channel->m_strIconPath             = m_pDS->fv("sIconPath").get_asString();
        channel->m_strChannelName          = m_pDS->fv("sChannelName").get_asString();
        channel->m_bIsVirtual              = m_pDS->fv("bIsVirtual").get_asBool();
        channel->m_bEPGEnabled             = m_pDS->fv("bEPGEnabled").get_asBool();
        channel->m_strEPGScraper           = m_pDS->fv("sEPGScraper").get_asString();
        channel->m_iLastWatched            = (time_t) m_pDS->fv("iLastWatched").get_asInt();
        channel->m_iClientId               = m_pDS->fv("iClientId").get_asInt();
        channel->m_iClientChannelNumber    = m_pDS->fv("iClientChannelNumber").get_asInt();
        channel->m_strInputFormat          = m_pDS->fv("sInputFormat").get_asString();
        channel->m_strStreamURL            = m_pDS->fv("sStreamURL").get_asString();
        channel->m_iClientEncryptionSystem = m_pDS->fv("iEncryptionSystem").get_asInt();
        channel->m_iEpgId                  = m_pDS->fv("idEpg").get_asInt();
        channel->UpdateEncryptionName();

#if PVRDB_DEBUGGING
        CLog::Log(LOGDEBUG, "PVR - %s - channel '%s' loaded from the database", __FUNCTION__, channel->m_strChannelName.c_str());
#endif
        PVRChannelGroupMember newMember = { channel, (unsigned int)m_pDS->fv("iChannelNumber").get_asInt() };
        results.m_members.push_back(newMember);

        m_pDS->next();
        ++iReturn;
      }
      m_pDS->close();
    }
    catch (...)
    {
      CLog::Log(LOGERROR, "PVR - %s - couldn't load channels from the database", __FUNCTION__);
    }
  }
  else
  {
    CLog::Log(LOGERROR, "PVR - %s - query failed", __FUNCTION__);
  }

  m_pDS->close();
  return iReturn;
}

bool CPVRDatabase::DeleteChannelSettings()
{
  CLog::Log(LOGDEBUG, "PVR - %s - deleting all channel settings from the database", __FUNCTION__);
  return DeleteValues("channelsettings");
}

bool CPVRDatabase::DeleteChannelSettings(const CPVRChannel &channel)
{
  bool bReturn(false);

  /* invalid channel */
  if (channel.ChannelID() <= 0)
  {
    CLog::Log(LOGERROR, "PVR - %s - invalid channel id: %i", __FUNCTION__, channel.ChannelID());
    return bReturn;
  }

  CStdString strWhereClause = FormatSQL("idChannel = %u", channel.ChannelID());
  return DeleteValues("channelsettings", strWhereClause);
}

bool CPVRDatabase::GetChannelSettings(const CPVRChannel &channel, CVideoSettings &settings)
{
  bool bReturn(false);

  /* invalid channel */
  if (channel.ChannelID() <= 0)
  {
    CLog::Log(LOGERROR, "PVR - %s - invalid channel id: %i", __FUNCTION__, channel.ChannelID());
    return bReturn;
  }

  CStdString strQuery = FormatSQL("SELECT * FROM channelsettings WHERE idChannel = %u;", channel.ChannelID());

  if (ResultQuery(strQuery))
  {
    try
    {
      if (m_pDS->num_rows() > 0)
      {
        settings.m_AudioDelay           = m_pDS->fv("fAudioDelay").get_asFloat();
        settings.m_AudioStream          = m_pDS->fv("iAudioStream").get_asInt();
        settings.m_Brightness           = m_pDS->fv("fBrightness").get_asFloat();
        settings.m_Contrast             = m_pDS->fv("fContrast").get_asFloat();
        settings.m_CustomPixelRatio     = m_pDS->fv("fPixelRatio").get_asFloat();
        settings.m_CustomNonLinStretch  = m_pDS->fv("bCustomNonLinStretch").get_asBool();
        settings.m_NoiseReduction       = m_pDS->fv("fNoiseReduction").get_asFloat();
        settings.m_PostProcess          = m_pDS->fv("bPostProcess").get_asBool();
        settings.m_Sharpness            = m_pDS->fv("fSharpness").get_asFloat();
        settings.m_CustomZoomAmount     = m_pDS->fv("fCustomZoomAmount").get_asFloat();
        settings.m_CustomVerticalShift  = m_pDS->fv("fCustomVerticalShift").get_asFloat();
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
        settings.m_DeinterlaceMode      = (EDEINTERLACEMODE)m_pDS->fv("iDeinterlaceMode").get_asInt();
        settings.m_VolumeAmplification  = m_pDS->fv("fVolumeAmplification").get_asFloat();
        settings.m_OutputToAllSpeakers  = m_pDS->fv("bOutputToAllSpeakers").get_asBool();
        settings.m_ScalingMethod        = (ESCALINGMETHOD)m_pDS->fv("iScalingMethod").get_asInt();

        bReturn = true;
      }

      m_pDS->close();
    }
    catch(...)
    {
      CLog::Log(LOGERROR, "PVR - %s - failed to get channel settings for channel '%s'", __FUNCTION__, channel.ChannelName().c_str());
    }
  }
  else
  {
    CLog::Log(LOGERROR, "PVR - %s - query failed", __FUNCTION__);
  }

  return bReturn;
}

bool CPVRDatabase::PersistChannelSettings(const CPVRChannel &channel, const CVideoSettings &settings)
{
  /* invalid channel */
  if (channel.ChannelID() <= 0)
  {
    CLog::Log(LOGERROR, "PVR - %s - invalid channel id: %i", __FUNCTION__, channel.ChannelID());
    return false;
  }

  CStdString strQuery = FormatSQL(
      "REPLACE INTO channelsettings "
        "(idChannel, iInterlaceMethod, iViewMode, fCustomZoomAmount, fPixelRatio, iAudioStream, iSubtitleStream, fSubtitleDelay, "
         "bSubtitles, fBrightness, fContrast, fGamma, fVolumeAmplification, fAudioDelay, bOutputToAllSpeakers, bCrop, iCropLeft, "
         "iCropRight, iCropTop, iCropBottom, fSharpness, fNoiseReduction, fCustomVerticalShift, bCustomNonLinStretch, bPostProcess, iScalingMethod, iDeinterlaceMode) VALUES "
         "(%i, %i, %i, %f, %f, %i, %i, %f, %i, %f, %f, %f, %f, %f, %i, %i, %i, %i, %i, %i, %f, %f, %f, %i, %i, %i, %i);",
       channel.ChannelID(), settings.m_InterlaceMethod, settings.m_ViewMode, settings.m_CustomZoomAmount, settings.m_CustomPixelRatio,
       settings.m_AudioStream, settings.m_SubtitleStream, settings.m_SubtitleDelay, settings.m_SubtitleOn ? 1 :0,
       settings.m_Brightness, settings.m_Contrast, settings.m_Gamma, settings.m_VolumeAmplification, settings.m_AudioDelay,
       settings.m_OutputToAllSpeakers ? 1 : 0, settings.m_Crop ? 1 : 0, settings.m_CropLeft, settings.m_CropRight, settings.m_CropTop,
       settings.m_CropBottom, settings.m_Sharpness, settings.m_NoiseReduction, settings.m_CustomVerticalShift,
       settings.m_CustomNonLinStretch ? 1 : 0, settings.m_PostProcess ? 1 : 0, settings.m_ScalingMethod, settings.m_DeinterlaceMode);

  return ExecuteQuery(strQuery);
}

/********** Channel group methods **********/

bool CPVRDatabase::RemoveChannelsFromGroup(const CPVRChannelGroup &group)
{
  CStdString strWhereClause = FormatSQL("idGroup = %u", group.GroupID());
  return DeleteValues("map_channelgroups_channels", strWhereClause);
}

bool CPVRDatabase::GetCurrentGroupMembers(const CPVRChannelGroup &group, vector<int> &members)
{
  bool bReturn(false);
  /* invalid group id */
  if (group.GroupID() <= 0)
  {
    CLog::Log(LOGERROR, "PVR - %s - invalid group id: %d", __FUNCTION__, group.GroupID());
    return false;
  }

  CStdString strCurrentMembersQuery = FormatSQL("SELECT idChannel FROM map_channelgroups_channels WHERE idGroup = %u", group.GroupID());
  if (ResultQuery(strCurrentMembersQuery))
  {
    try
    {
      while (!m_pDS->eof())
      {
        members.push_back(m_pDS->fv("idChannel").get_asInt());
        m_pDS->next();
      }
      m_pDS->close();
      bReturn = true;
    }
    catch (...)
    {
      CLog::Log(LOGERROR, "PVR - %s - couldn't load channels from the database", __FUNCTION__);
    }
  }
  else
  {
    CLog::Log(LOGERROR, "PVR - %s - query failed", __FUNCTION__);
  }

  return bReturn;
}

bool CPVRDatabase::DeleteChannelsFromGroup(const CPVRChannelGroup &group)
{
  /* invalid group id */
  if (group.GroupID() <= 0)
  {
    CLog::Log(LOGERROR, "PVR - %s - invalid group id: %d", __FUNCTION__, group.GroupID());
    return false;
  }

  CStdString strWhereClause;
  strWhereClause = FormatSQL("idGroup = %u", group.GroupID());
  return DeleteValues("map_channelgroups_channels", strWhereClause);
}

bool CPVRDatabase::DeleteChannelsFromGroup(const CPVRChannelGroup &group, const vector<int> &channelsToDelete)
{
  bool bDelete(true);
  unsigned int iDeletedChannels(0);
  /* invalid group id */
  if (group.GroupID() <= 0)
  {
    CLog::Log(LOGERROR, "PVR - %s - invalid group id: %d", __FUNCTION__, group.GroupID());
    return false;
  }

  while (iDeletedChannels < channelsToDelete.size())
  {
    CStdString strChannelsToDelete;
    CStdString strWhereClause;

    for (unsigned int iChannelPtr = 0; iChannelPtr + iDeletedChannels < channelsToDelete.size() && iChannelPtr < 50; iChannelPtr++)
      strChannelsToDelete.AppendFormat(", %d", channelsToDelete.at(iDeletedChannels + iChannelPtr));

    if (!strChannelsToDelete.IsEmpty())
    {
      strChannelsToDelete = strChannelsToDelete.Right(strChannelsToDelete.length() - 2);
      strWhereClause = FormatSQL("idGroup = %u AND idChannel IN (%s)", group.GroupID(), strChannelsToDelete.c_str());
      bDelete = DeleteValues("map_channelgroups_channels", strWhereClause) && bDelete;
    }

    iDeletedChannels += 50;
  }

  return bDelete;
}

bool CPVRDatabase::RemoveStaleChannelsFromGroup(const CPVRChannelGroup &group)
{
  bool bDelete(true);
  /* invalid group id */
  if (group.GroupID() <= 0)
  {
    CLog::Log(LOGERROR, "PVR - %s - invalid group id: %d", __FUNCTION__, group.GroupID());
    return false;
  }

  if (!group.IsInternalGroup())
  {
    /* First remove channels that don't exist in the main channels table */
    CStdString strWhereClause = FormatSQL("idChannel IN (SELECT map_channelgroups_channels.idChannel FROM map_channelgroups_channels LEFT JOIN channels on map_channelgroups_channels.idChannel = channels.idChannel WHERE channels.idChannel IS NULL)");
    bDelete = DeleteValues("map_channelgroups_channels", strWhereClause);
  }

  if (group.m_members.size() > 0)
  {
    vector<int> currentMembers;
    if (GetCurrentGroupMembers(group, currentMembers))
    {
      vector<int> channelsToDelete;
      for (unsigned int iChannelPtr = 0; iChannelPtr < currentMembers.size(); iChannelPtr++)
      {
        if (!group.IsGroupMember(currentMembers.at(iChannelPtr)))
          channelsToDelete.push_back(currentMembers.at(iChannelPtr));
      }

      bDelete = DeleteChannelsFromGroup(group, channelsToDelete) && bDelete;
    }
  }
  else
  {
    CStdString strWhereClause = FormatSQL("idGroup = %u", group.GroupID());
    bDelete = DeleteValues("map_channelgroups_channels", strWhereClause) && bDelete;
  }

  return bDelete;
}

bool CPVRDatabase::DeleteChannelGroups(void)
{
  CLog::Log(LOGDEBUG, "PVR - %s - deleting all channel groups from the database", __FUNCTION__);

  return DeleteValues("channelgroups") &&
      DeleteValues("map_channelgroups_channels");
}

bool CPVRDatabase::Delete(const CPVRChannelGroup &group)
{
  /* invalid group id */
  if (group.GroupID() <= 0)
  {
    CLog::Log(LOGERROR, "PVR - %s - invalid group id: %d", __FUNCTION__, group.GroupID());
    return false;
  }

  CStdString strWhereClause = FormatSQL("idGroup = %u AND bIsRadio = %u", group.GroupID(), group.IsRadio());
  return RemoveChannelsFromGroup(group) &&
      DeleteValues("channelgroups", strWhereClause);
}

bool CPVRDatabase::Get(CPVRChannelGroups &results)
{
  bool bReturn = false;
  CStdString strQuery = FormatSQL("SELECT * from channelgroups WHERE bIsRadio = %u", results.IsRadio());

  if (ResultQuery(strQuery))
  {
    try
    {
      while (!m_pDS->eof())
      {
        CPVRChannelGroup data(m_pDS->fv("bIsRadio").get_asBool(), m_pDS->fv("idGroup").get_asInt(), m_pDS->fv("sName").get_asString());
        data.SetGroupType(m_pDS->fv("iGroupType").get_asInt());
        results.Update(data);

        CLog::Log(LOGDEBUG, "PVR - %s - group '%s' loaded from the database", __FUNCTION__, data.GroupName().c_str());
        m_pDS->next();
      }
      m_pDS->close();
      bReturn = true;
    }
    catch (...)
    {
      CLog::Log(LOGERROR, "%s - couldn't load channels from the database", __FUNCTION__);
    }
  }

  return bReturn;
}

int CPVRDatabase::Get(CPVRChannelGroup &group)
{
  int iReturn = -1;

  /* invalid group id */
  if (group.GroupID() < 0)
  {
    CLog::Log(LOGERROR, "PVR - %s - invalid group id: %d", __FUNCTION__, group.GroupID());
    return -1;
  }

  CStdString strQuery = FormatSQL("SELECT idChannel, iChannelNumber FROM map_channelgroups_channels WHERE idGroup = %u ORDER BY iChannelNumber", group.GroupID());
  if (ResultQuery(strQuery))
  {
    iReturn = 0;

    try
    {
      while (!m_pDS->eof())
      {
        int iChannelId = m_pDS->fv("idChannel").get_asInt();
        int iChannelNumber = m_pDS->fv("iChannelNumber").get_asInt();
        CPVRChannelPtr channel = g_PVRChannelGroups->GetGroupAll(group.IsRadio())->GetByChannelID(iChannelId);

        if (channel)
        {
#if PVRDB_DEBUGGING
          CLog::Log(LOGDEBUG, "PVR - %s - channel '%s' loaded from the database", __FUNCTION__, channel->m_strChannelName.c_str());
#endif
          PVRChannelGroupMember newMember = { channel, (unsigned int)iChannelNumber };
          group.m_members.push_back(newMember);
          iReturn++;
        }
        else
        {
          // remove a channel that doesn't exist (anymore) from the table
          DeleteValues("map_channelgroups_channels", FormatSQL("idGroup = %u AND idChannel = %u", group.GroupID(), iChannelId));
        }

        m_pDS->next();
      }
      m_pDS->close();
    }
    catch(...)
    {
      CLog::Log(LOGERROR, "PVR - %s - failed to get channels", __FUNCTION__);
    }
  }

  if (iReturn > 0)
    group.SortByChannelNumber();

  return iReturn;
}

bool CPVRDatabase::PersistChannels(CPVRChannelGroup &group)
{
  bool bReturn(true);
  int iLastChannel(0);

  /* we can only safely get this from a local db */
  if (m_sqlite)
    iLastChannel = GetLastChannelId();

  for (unsigned int iChannelPtr = 0; iChannelPtr < group.m_members.size(); iChannelPtr++)
  {
    PVRChannelGroupMember member = group.m_members.at(iChannelPtr);
    if (member.channel->IsChanged() || member.channel->IsNew())
    {
      if (m_sqlite && member.channel->IsNew())
        member.channel->SetChannelID(++iLastChannel);
      bReturn &= Persist(*member.channel, m_sqlite || !member.channel->IsNew());
    }
  }

  bReturn &= CommitInsertQueries();

  return bReturn;
}

bool CPVRDatabase::PersistGroupMembers(CPVRChannelGroup &group)
{
  bool bReturn = true;
  bool bRemoveChannels = true;
  CStdString strQuery;
  CSingleLock lock(group.m_critSection);

  if (group.m_members.size() > 0)
  {
    for (unsigned int iChannelPtr = 0; iChannelPtr < group.m_members.size(); iChannelPtr++)
    {
      PVRChannelGroupMember member = group.m_members.at(iChannelPtr);

      CStdString strWhereClause = FormatSQL("idChannel = %u AND idGroup = %u AND iChannelNumber = %u",
          member.channel->ChannelID(), group.GroupID(), member.iChannelNumber);

      CStdString strValue = GetSingleValue("map_channelgroups_channels", "idChannel", strWhereClause);
      if (strValue.IsEmpty())
      {
        strQuery = FormatSQL("REPLACE INTO map_channelgroups_channels ("
            "idGroup, idChannel, iChannelNumber) "
            "VALUES (%i, %i, %i);",
            group.GroupID(), member.channel->ChannelID(), member.iChannelNumber);
        QueueInsertQuery(strQuery);
      }
    }
    lock.Leave();

    bReturn = CommitInsertQueries();
    bRemoveChannels = RemoveStaleChannelsFromGroup(group);
  }

  return bReturn && bRemoveChannels;
}

/********** Client methods **********/

bool CPVRDatabase::DeleteClients()
{
  CLog::Log(LOGDEBUG, "PVR - %s - deleting all clients from the database", __FUNCTION__);

  return DeleteValues("clients");
      //TODO && DeleteValues("map_channels_clients");
}

bool CPVRDatabase::Delete(const CPVRClient &client)
{
  /* invalid client uid */
  if (client.ID().IsEmpty())
  {
    CLog::Log(LOGERROR, "PVR - %s - invalid client uid", __FUNCTION__);
    return false;
  }

  CStdString strWhereClause = FormatSQL("sUid = '%s'", client.ID().c_str());
  return DeleteValues("clients", strWhereClause);
}

int CPVRDatabase::GetClientId(const CStdString &strClientUid)
{
  CStdString strWhereClause = FormatSQL("sUid = '%s'", strClientUid.c_str());
  CStdString strValue = GetSingleValue("clients", "idClient", strWhereClause);

  if (strValue.IsEmpty())
    return -1;

  return atol(strValue.c_str());
}

bool CPVRDatabase::Persist(CPVRChannelGroup &group)
{
  bool bReturn(false);
  if (group.GroupName().IsEmpty())
  {
    CLog::Log(LOGERROR, "%s - empty group name", __FUNCTION__);
    return bReturn;
  }

  CStdString strQuery;
  bReturn = true;
  {
    CSingleLock lock(group.m_critSection);

    /* insert a new entry when this is a new group, or replace the existing one otherwise */
    if (group.GroupID() <= 0)
      strQuery = FormatSQL("INSERT INTO channelgroups (bIsRadio, iGroupType, sName) VALUES (%i, %i, '%s')",
          (group.IsRadio() ? 1 :0), group.GroupType(), group.GroupName().c_str());
    else
      strQuery = FormatSQL("REPLACE INTO channelgroups (idGroup, bIsRadio, iGroupType, sName) VALUES (%i, %i, %i, '%s')",
          group.GroupID(), (group.IsRadio() ? 1 :0), group.GroupType(), group.GroupName().c_str());

    bReturn = ExecuteQuery(strQuery);

    /* set the group id if it was <= 0 */
    if (bReturn && group.GroupID() <= 0)
      group.m_iGroupId = (int) m_pDS->lastinsertid();
  }

  /* only persist the channel data for the internal groups */
  if (group.IsInternalGroup())
    bReturn &= PersistChannels(group);

  /* persist the group member entries */
  if (bReturn)
    bReturn = PersistGroupMembers(group);

  return bReturn;
}

int CPVRDatabase::Persist(const AddonPtr client)
{
  int iReturn(-1);

  /* invalid client uid or name */
  if (client->Name().IsEmpty() || client->ID().IsEmpty())
  {
    CLog::Log(LOGERROR, "PVR - %s - invalid client uid or name", __FUNCTION__);
    return iReturn;
  }

  CStdString strQuery = FormatSQL("REPLACE INTO clients (sName, sUid) VALUES ('%s', '%s');",
      client->Name().c_str(), client->ID().c_str());

  if (ExecuteQuery(strQuery))
    iReturn = (int) m_pDS->lastinsertid();

  return iReturn;
}

bool CPVRDatabase::Persist(CPVRChannel &channel, bool bQueueWrite /* = false */)
{
  bool bReturn(false);

  /* invalid channel */
  if (channel.UniqueID() <= 0)
  {
    CLog::Log(LOGERROR, "PVR - %s - invalid channel uid: %d", __FUNCTION__, channel.UniqueID());
    return bReturn;
  }

  CStdString strQuery;
  if (channel.ChannelID() <= 0)
  {
    /* new channel */
    strQuery = FormatSQL("INSERT INTO channels ("
        "iUniqueId, bIsRadio, bIsHidden, bIsUserSetIcon, bIsLocked, "
        "sIconPath, sChannelName, bIsVirtual, bEPGEnabled, sEPGScraper, iLastWatched, iClientId, "
        "iClientChannelNumber, sInputFormat, sStreamURL, iEncryptionSystem, idEpg) "
        "VALUES (%i, %i, %i, %i, %i, '%s', '%s', %i, %i, '%s', %u, %i, %i, '%s', '%s', %i, %i)",
        channel.UniqueID(), (channel.IsRadio() ? 1 :0), (channel.IsHidden() ? 1 : 0), (channel.IsUserSetIcon() ? 1 : 0), (channel.IsLocked() ? 1 : 0),
        channel.IconPath().c_str(), channel.ChannelName().c_str(), (channel.IsVirtual() ? 1 : 0), (channel.EPGEnabled() ? 1 : 0), channel.EPGScraper().c_str(), channel.LastWatched(), channel.ClientID(),
        channel.ClientChannelNumber(), channel.InputFormat().c_str(), channel.StreamURL().c_str(), channel.EncryptionSystem(),
        channel.EpgID());
  }
  else
  {
    /* update channel */
    strQuery = FormatSQL("REPLACE INTO channels ("
        "iUniqueId, bIsRadio, bIsHidden, bIsUserSetIcon, bIsLocked, "
        "sIconPath, sChannelName, bIsVirtual, bEPGEnabled, sEPGScraper, iLastWatched, iClientId, "
        "iClientChannelNumber, sInputFormat, sStreamURL, iEncryptionSystem, idChannel, idEpg) "
        "VALUES (%i, %i, %i, %i, %i, '%s', '%s', %i, %i, '%s', %u, %i, %i, '%s', '%s', %i, %i, %i)",
        channel.UniqueID(), (channel.IsRadio() ? 1 :0), (channel.IsHidden() ? 1 : 0), (channel.IsUserSetIcon() ? 1 : 0), (channel.IsLocked() ? 1 : 0),
        channel.IconPath().c_str(), channel.ChannelName().c_str(), (channel.IsVirtual() ? 1 : 0), (channel.EPGEnabled() ? 1 : 0), channel.EPGScraper().c_str(), channel.LastWatched(), channel.ClientID(),
        channel.ClientChannelNumber(), channel.InputFormat().c_str(), channel.StreamURL().c_str(), channel.EncryptionSystem(), channel.ChannelID(),
        channel.EpgID());
  }

  if (bQueueWrite)
  {
    QueueInsertQuery(strQuery);
    bReturn = true;
  }
  else if (ExecuteQuery(strQuery))
  {
    CSingleLock lock(channel.m_critSection);
    if (channel.m_iChannelId <= 0)
      channel.m_iChannelId = (int)m_pDS->lastinsertid();
    bReturn = true;
  }

  return bReturn;
}
