/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "PictureDatabase.h"
#include "system.h"
#include "utils/LegacyPathTranslation.h"
#include "utils/log.h"
#include "utils/TimeUtils.h"
#include "interfaces/AnnouncementManager.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"
#include "settings/AdvancedSettings.h"
#include "Application.h"





#include "network/Network.h"
#include "threads/SystemClock.h"
#include "system.h"
#include "network/cddb.h"
#include "filesystem/DirectoryCache.h"
#include "filesystem/MusicDatabaseDirectory/DirectoryNode.h"
#include "filesystem/MusicDatabaseDirectory/QueryParams.h"
#include "filesystem/MusicDatabaseDirectory.h"
#include "filesystem/SpecialProtocol.h"
#include "GUIInfoManager.h"
#include "music/tags/MusicInfoTag.h"
#include "addons/AddonManager.h"
#include "addons/Scraper.h"
#include "addons/Addon.h"
#include "utils/URIUtils.h"
#include "guilib/GUIWindowManager.h"
#include "dialogs/GUIDialogOK.h"
#include "dialogs/GUIDialogProgress.h"
#include "dialogs/GUIDialogYesNo.h"
#include "dialogs/GUIDialogSelect.h"
#include "filesystem/File.h"
#include "profiles/ProfilesManager.h"
#include "settings/AdvancedSettings.h"
#include "FileItem.h"
#include "Application.h"
#include "storage/MediaManager.h"
#include "settings/MediaSettings.h"
#include "settings/Settings.h"
#include "utils/StringUtils.h"
#include "guilib/LocalizeStrings.h"
#include "utils/LegacyPathTranslation.h"
#include "utils/log.h"
#include "utils/TimeUtils.h"
#include "TextureCache.h"
#include "addons/AddonInstaller.h"
#include "utils/AutoPtrHandle.h"
#include "interfaces/AnnouncementManager.h"
#include "dbwrappers/dataset.h"
#include "utils/XMLUtils.h"
#include "URL.h"
#include "playlists/SmartPlayList.h"


using namespace std;
using namespace AUTOPTR;
using namespace XFILE;
using namespace MUSICDATABASEDIRECTORY;
using ADDON::AddonPtr;


static void AnnounceRemove(const std::string& content, int id)
{
    CVariant data;
    data["type"] = content;
    data["id"] = id;
    ANNOUNCEMENT::CAnnouncementManager::Announce(ANNOUNCEMENT::AudioLibrary, "xbmc", "OnRemove", data);
}

static void AnnounceUpdate(const std::string& content, int id)
{
    CVariant data;
    data["type"] = content;
    data["id"] = id;
    ANNOUNCEMENT::CAnnouncementManager::Announce(ANNOUNCEMENT::AudioLibrary, "xbmc", "OnUpdate", data);
}

CPictureDatabase::CPictureDatabase(void)
{
}

CPictureDatabase::~CPictureDatabase(void)
{
    EmptyCache();
}

void CPictureDatabase::EmptyCache()
{
//    m_albumCache.erase(m_albumCache.begin(), m_albumCache.end());
//    m_thumbCache.erase(m_thumbCache.begin(), m_thumbCache.end());
}

bool CPictureDatabase::Open()
{
    return CDatabase::Open(g_advancedSettings.m_databasePicture);
}

bool CPictureDatabase::CommitTransaction()
{
    if (CDatabase::CommitTransaction())
    { // number of items in the db has likely changed, so reset the infomanager cache
        //g_infoManager.SetLibraryBool(LIBRARY_HAS_MUSIC, GetSongsCount() > 0);
        return true;
    }
    return false;
}

int CPictureDatabase::GetMinVersion() const
{
    return 1;
}


int CPictureDatabase::Cleanup(CGUIDialogProgress *pDlgProgress)
{
    if (NULL == m_pDB.get()) return ERROR_DATABASE;
    if (NULL == m_pDS.get()) return ERROR_DATABASE;
    
    int ret = ERROR_OK;
    unsigned int time = XbmcThreads::SystemClockMillis();
    CLog::Log(LOGNOTICE, "%s: Starting musicdatabase cleanup ..", __FUNCTION__);
    ANNOUNCEMENT::CAnnouncementManager::Announce(ANNOUNCEMENT::AudioLibrary, "xbmc", "OnCleanStarted");
    
    // first cleanup any songs with invalid paths
    if (pDlgProgress)
    {
        pDlgProgress->SetHeading(700);
        pDlgProgress->SetLine(0, "");
        pDlgProgress->SetLine(1, 318);
        pDlgProgress->SetLine(2, 330);
        pDlgProgress->SetPercentage(0);
        pDlgProgress->StartModal();
        pDlgProgress->ShowProgressBar(true);
    }
    if (!CommitTransaction())
    {
        ret = ERROR_WRITING_CHANGES;
        goto error;
    }
    // and compress the database
    if (pDlgProgress)
    {
        pDlgProgress->SetLine(1, 331);
        pDlgProgress->SetPercentage(100);
        pDlgProgress->Progress();
    }
    time = XbmcThreads::SystemClockMillis() - time;
    CLog::Log(LOGNOTICE, "%s: Cleaning musicdatabase done. Operation took %s", __FUNCTION__, StringUtils::SecondsToTimeString(time / 1000).c_str());
    ANNOUNCEMENT::CAnnouncementManager::Announce(ANNOUNCEMENT::AudioLibrary, "xbmc", "OnCleanFinished");
    
    if (!Compress(false))
    {
        return ERROR_COMPRESSING;
    }
    return ERROR_OK;
    
error:
    RollbackTransaction();
    ANNOUNCEMENT::CAnnouncementManager::Announce(ANNOUNCEMENT::AudioLibrary, "xbmc", "OnCleanFinished");
    return ret;
}


bool CPictureDatabase::CreateTables()
{
    BeginTransaction();
    try
    {
        CDatabase::CreateTables();

        
        CLog::Log(LOGINFO, "create artist pictures");
        m_pDS->exec("CREATE TABLE pictures ( idPicture integer primary key, strName text, strPath text, strThumbnail text)\n");
        CLog::Log(LOGINFO, "create album table");
        m_pDS->exec("CREATE TABLE album ( idAlbum integer primary key, strAlbum text)\n");
        CLog::Log(LOGINFO, "create album_picture table");
        m_pDS->exec("CREATE TABLE album_picture ( idPicture integer, idAlbum integer )\n");
                 
    }
    catch (...)
    {
        CLog::Log(LOGERROR, "%s unable to create tables:%i", __FUNCTION__, (int)GetLastError());
        RollbackTransaction();
        return false;
    }
    CommitTransaction();
    return true;
}




void CPictureDatabase::Clean()
{
    // If we are scanning for music info in the background,
    // other writing access to the database is prohibited.
    if (g_application.IsMusicScanning())
    {
        CGUIDialogOK::ShowAndGetInput(189, 14057, 0, 0);
        return;
    }
    
    if (CGUIDialogYesNo::ShowAndGetInput(313, 333, 0, 0))
    {
        CGUIDialogProgress* dlgProgress = (CGUIDialogProgress*)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
        if (dlgProgress)
        {
            CPictureDatabase musicdatabase;
            if (musicdatabase.Open())
            {
                int iReturnString = musicdatabase.Cleanup(dlgProgress);
                musicdatabase.Close();
                
                if (iReturnString != ERROR_OK)
                {
                    CGUIDialogOK::ShowAndGetInput(313, iReturnString, 0, 0);
                }
            }
            dlgProgress->Close();
        }
    }
}
