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

#include "threads/SystemClock.h"
#include "PictureInfoScanner.h"
//#include "picture/tags/PictureInfoTagLoaderFactory.h"
#include "PictureAlbumInfo.h"
#include "PictureInfoScraper.h"
#include "filesystem/PictureDatabaseDirectory.h"
#include "filesystem/PictureDatabaseDirectory/DirectoryNode.h"
#include "Util.h"
#include "utils/md5.h"
#include "GUIInfoManager.h"
#include "utils/Variant.h"
#include "NfoFile.h"
//#include "picture/tags/PictureInfoTag.h"
#include "guilib/GUIWindowManager.h"
#include "dialogs/GUIDialogExtendedProgressBar.h"
#include "dialogs/GUIDialogProgress.h"
#include "dialogs/GUIDialogSelect.h"
#include "guilib/GUIKeyboardFactory.h"
#include "filesystem/File.h"
#include "filesystem/Directory.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "FileItem.h"
#include "guilib/LocalizeStrings.h"
#include "utils/StringUtils.h"
#include "utils/TimeUtils.h"
#include "utils/log.h"
#include "utils/URIUtils.h"
#include "TextureCache.h"
#include "pictures/Picture.h"
#include "pictures/PictureThumbLoader.h"
#include "interfaces/AnnouncementManager.h"
#include "GUIUserMessages.h"
#include "addons/AddonManager.h"
#include "addons/Scraper.h"

#include <algorithm>

using namespace std;
using namespace PICTURE_INFO;
using namespace XFILE;
using namespace PICTUREDATABASEDIRECTORY;
using namespace PICTURE_GRABBER;
using namespace ADDON;

CPictureInfoScanner::CPictureInfoScanner() : CThread("PictureInfoScanner"), m_fileCountReader(this, "PictureFileCounter")
{
    m_bRunning = false;
    m_showDialog = false;
    m_handle = NULL;
    m_bCanInterrupt = false;
    m_currentItem=0;
    m_itemCount=0;
    m_flags = 0;
}

CPictureInfoScanner::~CPictureInfoScanner()
{
}

void CPictureInfoScanner::Process()
{
    ANNOUNCEMENT::CAnnouncementManager::Announce(ANNOUNCEMENT::AudioLibrary, "xbmc", "OnScanStarted");
    try
    {
        unsigned int tick = XbmcThreads::SystemClockMillis();
        
        m_pictureDatabase.Open();
        
        if (m_showDialog && !CSettings::Get().GetBool("picturelibrary.backgroundupdate"))
        {
            CGUIDialogExtendedProgressBar* dialog =
            (CGUIDialogExtendedProgressBar*)g_windowManager.GetWindow(WINDOW_DIALOG_EXT_PROGRESS);
            m_handle = dialog->GetHandle(g_localizeStrings.Get(314));
        }
        
        m_bCanInterrupt = true;
        
        if (m_scanType == 0) // load info from files
        {
            CLog::Log(LOGDEBUG, "%s - Starting scan", __FUNCTION__);
            
            if (m_handle)
                m_handle->SetTitle(g_localizeStrings.Get(505));
            
            // Reset progress vars
            m_currentItem=0;
            m_itemCount=-1;
            
            // Create the thread to count all files to be scanned
            SetPriority( GetMinPriority() );
            if (m_handle)
                m_fileCountReader.Create();
            
            // Database operations should not be canceled
            // using Interupt() while scanning as it could
            // result in unexpected behaviour.
            m_bCanInterrupt = false;
            m_needsCleanup = false;
            
            bool commit = false;
            bool cancelled = false;
            for (std::set<std::string>::const_iterator it = m_pathsToScan.begin(); it != m_pathsToScan.end(); it++)
            {
                /*
                 * A copy of the directory path is used because the path supplied is
                 * immediately removed from the m_pathsToScan set in DoScan(). If the
                 * reference points to the entry in the set a null reference error
                 * occurs.
                 */
                if (!DoScan(*it))
                    cancelled = true;
                commit = !cancelled;
            }
            
            if (commit)
            {
                g_infoManager.ResetLibraryBools();
                
                if (m_needsCleanup)
                {
                    if (m_handle)
                    {
                        m_handle->SetTitle(g_localizeStrings.Get(700));
                        m_handle->SetText("");
                    }
                    
                    m_pictureDatabase.CleanupOrphanedItems();
                    
                    if (m_handle)
                        m_handle->SetTitle(g_localizeStrings.Get(331));
                    
                    m_pictureDatabase.Compress(false);
                }
            }
            
            m_fileCountReader.StopThread();
            
            m_pictureDatabase.EmptyCache();
            
            tick = XbmcThreads::SystemClockMillis() - tick;
            CLog::Log(LOGNOTICE, "My Picture: Scanning for picture info using worker thread, operation took %s", StringUtils::SecondsToTimeString(tick / 1000).c_str());
        }
        if (m_scanType == 1) // load album info
        {
            for (std::set<std::string>::const_iterator it = m_pathsToScan.begin(); it != m_pathsToScan.end(); ++it)
            {
                CQueryParams params;
                CDirectoryNode::GetDatabaseInfo(*it, params);
                if (m_pictureDatabase.HasPictureAlbumInfo(params.GetFaceId())) // should this be here?
                    continue;
                /*
                CPictureAlbum album;
                m_pictureDatabase.GetPictureAlbumInfo(params.GetAlbumId(), album, &album.pictures);
                if (m_handle)
                {
                    float percentage = (float) std::distance(it, m_pathsToScan.end()) / m_pathsToScan.size();
                    m_handle->SetText(StringUtils::Join(album.face, g_advancedSettings.m_pictureItemSeparator) + " - " + album.strAlbum);
                    m_handle->SetPercentage(percentage);
                }
                
                CPictureAlbumInfo albumInfo;
                UpdateDatabaseAlbumInfo(*it, albumInfo, false);
                */
                if (m_bStop)
                    break;
            }
        }
        if (m_scanType == 2) // load face info
        {
            for (std::set<std::string>::const_iterator it = m_pathsToScan.begin(); it != m_pathsToScan.end(); ++it)
            {
                CQueryParams params;
                CDirectoryNode::GetDatabaseInfo(*it, params);
                if (m_pictureDatabase.HasFaceInfo(params.GetFaceId())) // should this be here?
                    continue;
                
                CFace face;
                m_pictureDatabase.GetFaceInfo(params.GetFaceId(), face);
                m_pictureDatabase.GetFacePath(params.GetFaceId(), face.strPath);
                
                if (m_handle)
                {
                    float percentage = (float) (std::distance(m_pathsToScan.begin(), it) / m_pathsToScan.size()) * 100;
                    m_handle->SetText(face.strFace);
                    m_handle->SetPercentage(percentage);
                }
                
                CPictureFaceInfo faceInfo;
                UpdateDatabaseFaceInfo(*it, faceInfo, false);
                
                if (m_bStop)
                    break;
            }
        }
        
    }
    catch (...)
    {
        CLog::Log(LOGERROR, "PictureInfoScanner: Exception while scanning.");
    }
    m_pictureDatabase.Close();
    CLog::Log(LOGDEBUG, "%s - Finished scan", __FUNCTION__);
    
    m_bRunning = false;
    ANNOUNCEMENT::CAnnouncementManager::Announce(ANNOUNCEMENT::AudioLibrary, "xbmc", "OnScanFinished");
    
    // we need to clear the picturedb cache and update any active lists
//    CUtil::DeletePictureDatabaseDirectoryCache();
    CGUIMessage msg(GUI_MSG_SCAN_FINISHED, 0, 0, 0);
    g_windowManager.SendThreadMessage(msg);
    
    if (m_handle)
        m_handle->MarkFinished();
    m_handle = NULL;
}

void CPictureInfoScanner::Start(const CStdString& strDirectory, int flags)
{
    m_fileCountReader.StopThread();
    StopThread();
    m_pathsToScan.clear();
    m_flags = flags;
    
    if (strDirectory.IsEmpty())
    { // scan all paths in the database.  We do this by scanning all paths in the db, and crossing them off the list as
        // we go.
        m_pictureDatabase.Open();
        m_pictureDatabase.GetPaths(m_pathsToScan);
        m_pictureDatabase.Close();
    }
    else
        m_pathsToScan.insert(strDirectory);
    
    m_scanType = 0;
    Create();
    m_bRunning = true;
}

void CPictureInfoScanner::FetchAlbumInfo(const CStdString& strDirectory,
                                       bool refresh)
{
    m_fileCountReader.StopThread();
    StopThread();
    m_pathsToScan.clear();
    
    CFileItemList items;
    if (strDirectory.IsEmpty())
    {
        m_pictureDatabase.Open();
        m_pictureDatabase.GetPictureAlbumsNav("picturedb://albums/", items);
        m_pictureDatabase.Close();
    }
    else
    {
        if (URIUtils::HasSlashAtEnd(strDirectory)) // directory
            CDirectory::GetDirectory(strDirectory,items);
        else
        {
            CFileItemPtr item(new CFileItem(strDirectory,false));
            items.Add(item);
        }
    }
    
    m_pictureDatabase.Open();
    for (int i=0;i<items.Size();++i)
    {
        if (CPictureDatabaseDirectory::IsAllItem(items[i]->GetPath()) || items[i]->IsParentFolder())
            continue;
        
        m_pathsToScan.insert(items[i]->GetPath());
        if (refresh)
        {
            //m_pictureDatabase.DeletePictureAlbumInfo(items[i]->GetPictureInfoTag()->GetDatabaseId());
        }
    }
    m_pictureDatabase.Close();
    
    m_scanType = 1;
    Create();
    m_bRunning = true;
}

void CPictureInfoScanner::FetchFaceInfo(const CStdString& strDirectory,
                                        bool refresh)
{
    m_fileCountReader.StopThread();
    StopThread();
    m_pathsToScan.clear();
    CFileItemList items;
    
    if (strDirectory.IsEmpty())
    {
        m_pictureDatabase.Open();
        m_pictureDatabase.GetFacesNav("picturedb://faces/", items, false, -1);
        m_pictureDatabase.Close();
    }
    else
    {
        if (URIUtils::HasSlashAtEnd(strDirectory)) // directory
            CDirectory::GetDirectory(strDirectory,items);
        else
        {
            CFileItemPtr newItem(new CFileItem(strDirectory,false));
            items.Add(newItem);
        }
    }
    
    m_pictureDatabase.Open();
    for (int i=0;i<items.Size();++i)
    {
        if (CPictureDatabaseDirectory::IsAllItem(items[i]->GetPath()) || items[i]->IsParentFolder())
            continue;
        
        m_pathsToScan.insert(items[i]->GetPath());
        if (refresh)
        {
            //m_pictureDatabase.DeleteFaceInfo(items[i]->GetPictureInfoTag()->GetDatabaseId());
        }
    }
    m_pictureDatabase.Close();
    
    m_scanType = 2;
    Create();
    m_bRunning = true;
}

bool CPictureInfoScanner::IsScanning()
{
    return m_bRunning;
}

void CPictureInfoScanner::Stop()
{
    if (m_bCanInterrupt)
        m_pictureDatabase.Interupt();
    
    StopThread(false);
}

static void OnDirectoryScanned(const CStdString& strDirectory)
{
    CGUIMessage msg(GUI_MSG_DIRECTORY_SCANNED, 0, 0, 0);
    msg.SetStringParam(strDirectory);
    g_windowManager.SendThreadMessage(msg);
}

static CStdString Prettify(const CStdString& strDirectory)
{
    CURL url(strDirectory);
    CStdString strStrippedPath = url.GetWithoutUserDetails();
    CURL::Decode(strStrippedPath);
    
    return strStrippedPath;
}

bool CPictureInfoScanner::DoScan(const CStdString& strDirectory)
{
    if (m_handle)
        m_handle->SetText(Prettify(strDirectory));
    
    // Discard all excluded files defined by m_pictureExcludeRegExps
    CStdStringArray regexps = g_advancedSettings.m_audioExcludeFromScanRegExps;
    if (CUtil::ExcludeFileOrFolder(strDirectory, regexps))
        return true;
    
    // load subfolder
    CFileItemList items;
    CDirectory::GetDirectory(strDirectory, items, g_advancedSettings.m_pictureExtensions + "|.jpg|.tbn|.lrc|.cdg");
    
    // sort and get the path hash.  Note that we don't filter .cue sheet items here as we want
    // to detect changes in the .cue sheet as well.  The .cue sheet items only need filtering
    // if we have a changed hash.
    items.Sort(SORT_METHOD_LABEL, SortOrderAscending);
    CStdString hash;
    GetPathHash(items, hash);

    // check whether we need to rescan or not
    CStdString dbHash;
    if ((m_flags & SCAN_RESCAN) || !m_pictureDatabase.GetPathHash(strDirectory, dbHash) || dbHash != hash)
    { // path has changed - rescan
        if (dbHash.IsEmpty())
            CLog::Log(LOGDEBUG, "%s Scanning dir '%s' as not in the database", __FUNCTION__, strDirectory.c_str());
        else
            CLog::Log(LOGDEBUG, "%s Rescanning dir '%s' due to change", __FUNCTION__, strDirectory.c_str());
        
        // filter items in the sub dir (for .cue sheet support)
        items.FilterCueItems();
        items.Sort(SORT_METHOD_LABEL, SortOrderAscending);
        
        // and then scan in the new information
        if (RetrievePictureInfo(strDirectory, items) > 0)
        {
            if (m_handle)
                OnDirectoryScanned(strDirectory);
        }
        
        // save information about this folder
        m_pictureDatabase.SetPathHash(strDirectory, hash);
    }
    else
    { // path is the same - no need to rescan
        CLog::Log(LOGDEBUG, "%s Skipping dir '%s' due to no change", __FUNCTION__, strDirectory.c_str());
        m_currentItem += CountFiles(items, false);  // false for non-recursive
        
        // updated the dialog with our progress
        if (m_handle)
        {
            if (m_itemCount>0)
                m_handle->SetPercentage(m_currentItem/(float)m_itemCount*100);
            OnDirectoryScanned(strDirectory);
        }
    }
    
    // now scan the subfolders
    for (int i = 0; i < items.Size(); ++i)
    {
        CFileItemPtr pItem = items[i];
        
        if (m_bStop)
            break;
        // if we have a directory item (non-playlist) we then recurse into that folder
        if (pItem->m_bIsFolder && !pItem->IsParentFolder() && !pItem->IsPlayList())
        {
            CStdString strPath=pItem->GetPath();
            if (!DoScan(strPath))
            {
                m_bStop = true;
            }
        }
    }

    return !m_bStop;
}

INFO_RET CPictureInfoScanner::ScanTags(const CFileItemList& items, CFileItemList& scannedItems)
{
    CStdStringArray regexps = g_advancedSettings.m_audioExcludeFromScanRegExps;
    
    for (int i = 0; i < items.Size(); ++i)
    {
        if (m_bStop)
            return INFO_CANCELLED;
        
        CFileItemPtr pItem = items[i];
        
        if (CUtil::ExcludeFileOrFolder(pItem->GetPath(), regexps))
            continue;
        
        if (pItem->m_bIsFolder || pItem->IsPlayList() || pItem->IsPicture() || pItem->IsLyrics())
            continue;
        
        m_currentItem++;
        /*
        CPictureInfoTag& tag = *pItem->GetPictureInfoTag();
        if (!tag.Loaded())
        {
            auto_ptr<IPictureInfoTagLoader> pLoader (CPictureInfoTagLoaderFactory::CreateLoader(pItem->GetPath()));
            if (NULL != pLoader.get())
                pLoader->Load(pItem->GetPath(), tag);
        }
        
        if (m_handle && m_itemCount>0)
            m_handle->SetPercentage(m_currentItem/(float)m_itemCount*100);
        
        if (!tag.Loaded())
        {
            CLog::Log(LOGDEBUG, "%s - No tag found for: %s", __FUNCTION__, pItem->GetPath().c_str());
            continue;
        }
         */
        scannedItems.Add(pItem);
    }
    return INFO_ADDED;
}

void CPictureInfoScanner::FileItemsToAlbums(CFileItemList& items, VECPICTUREALBUMS& albums, MAPPICTURES* picturesMap /* = NULL */)
{
    for (int i = 0; i < items.Size(); ++i)
    {
        CFileItemPtr pItem = items[i];
        CPictureInfoTag& tag = *pItem->GetPictureInfoTag();
        CPicture picture(*pItem);
        
        // keep the db-only fields intact on rescan...
        if (picturesMap != NULL)
        {
            MAPPICTURES::iterator it = picturesMap->find(pItem->GetPath());
            if (it != picturesMap->end())
            {
                picture.iTimesPlayed = it->second.iTimesPlayed;
                picture.lastPlayed = it->second.lastPlayed;
                picture.iKaraokeNumber = it->second.iKaraokeNumber;
                if (picture.rating == '0')    picture.rating = it->second.rating;
                if (picture.strThumb.empty()) picture.strThumb = it->second.strThumb;
            }
        }
        /*
        if (!tag.GetPictureBrainzFaceID().empty())
        {
            for (vector<string>::const_iterator it = tag.GetPictureBrainzFaceID().begin(); it != tag.GetPictureBrainzFaceID().end(); ++it)
            {
                CStdString strJoinPhrase = (it == --tag.GetPictureBrainzFaceID().end() ? "" : g_advancedSettings.m_pictureItemSeparator);
                CFaceCredit mbface(tag.GetFace().empty() ? "" : tag.GetFace()[0], *it, strJoinPhrase);
                picture.faceCredits.push_back(mbface);
            }
            picture.face = tag.GetFace();
        }
        else
        {
            for (vector<string>::const_iterator it = tag.GetFace().begin(); it != tag.GetFace().end(); ++it)
            {
                CStdString strJoinPhrase = (it == --tag.GetFace().end() ? "" : g_advancedSettings.m_pictureItemSeparator);
                CFaceCredit nonmbface(*it, strJoinPhrase);
                picture.faceCredits.push_back(nonmbface);
            }
            picture.face = tag.GetFace();
        }
        
        bool found = false;
        for (VECPICTUREALBUMS::iterator it = albums.begin(); it != albums.end(); ++it)
        {
            if (it->strAlbum == tag.GetAlbum() && it->strPictureBrainzAlbumID == tag.GetPictureBrainzAlbumID())
            {
                it->pictures.push_back(picture);
                found = true;
            }
        }
        if (!found)
        {
            CPictureAlbum album(*pItem.get());
            if (!tag.GetPictureBrainzAlbumFaceID().empty())
            {
                for (vector<string>::const_iterator it = tag.GetPictureBrainzAlbumFaceID().begin(); it != tag.GetPictureBrainzAlbumFaceID().end(); ++it)
                {
                    // Picard always stored the display face string in the first face slot, no need to split it
                    CStdString strJoinPhrase = (it == --tag.GetPictureBrainzAlbumFaceID().end() ? "" : g_advancedSettings.m_pictureItemSeparator);
                    CFaceCredit mbface(tag.GetAlbumFace().empty() ? "" : tag.GetAlbumFace()[0], *it, strJoinPhrase);
                    album.faceCredits.push_back(mbface);
                }
                album.face = tag.GetAlbumFace();
            }
            else
            {
                for (vector<string>::const_iterator it = tag.GetAlbumFace().begin(); it != tag.GetAlbumFace().end(); ++it)
                {
                    CStdString strJoinPhrase = (it == --tag.GetAlbumFace().end() ? "" : g_advancedSettings.m_pictureItemSeparator);
                    CFaceCredit nonmbface(*it, strJoinPhrase);
                    album.faceCredits.push_back(nonmbface);
                }
                album.face = tag.GetAlbumFace();
            }
            album.pictures.push_back(picture);
            albums.push_back(album);
        }
         */
    }
}

int CPictureInfoScanner::RetrievePictureInfo(const CStdString& strDirectory, CFileItemList& items)
{
    MAPPICTURES picturesMap;
    
    // get all information for all files in current directory from database, and remove them
    if (m_pictureDatabase.RemovePicturesFromPath(strDirectory, picturesMap))
        m_needsCleanup = true;
    
    CFileItemList scannedItems;
    if (ScanTags(items, scannedItems) == INFO_CANCELLED)
        return 0;
    
    VECPICTUREALBUMS albums;
    FileItemsToAlbums(scannedItems, albums, &picturesMap);
    if (!(m_flags & SCAN_ONLINE))
        FixupAlbums(albums);
    FindArtForAlbums(albums, items.GetPath());
    
    int numAdded = 0;
    ADDON::AddonPtr addon;
    ADDON::ScraperPtr albumScraper;
    ADDON::ScraperPtr faceScraper;
    if(ADDON::CAddonMgr::Get().GetDefault(ADDON::ADDON_SCRAPER_ALBUMS, addon))
        albumScraper = boost::dynamic_pointer_cast<ADDON::CScraper>(addon);
    
    if(ADDON::CAddonMgr::Get().GetDefault(ADDON::ADDON_SCRAPER_ARTISTS, addon))
        faceScraper = boost::dynamic_pointer_cast<ADDON::CScraper>(addon);
    
    // Add each album
    for (VECPICTUREALBUMS::iterator album = albums.begin(); album != albums.end(); ++album)
    {
        if (m_bStop)
            break;
        
        album->strPath = strDirectory;
        m_pictureDatabase.BeginTransaction();
        
        // Check if the album has already been downloaded or failed
        map<CPictureAlbum, CPictureAlbum>::iterator cachedAlbum = m_albumCache.find(*album);
        if (cachedAlbum == m_albumCache.end())
        {
            // No - download the information
            CPictureAlbumInfo albumInfo;
            INFO_RET albumDownloadStatus = INFO_NOT_FOUND;
            if ((m_flags & SCAN_ONLINE) && albumScraper)
                albumDownloadStatus = DownloadAlbumInfo(*album, albumScraper, albumInfo);
            /*
            if (albumDownloadStatus == INFO_ADDED || albumDownloadStatus == INFO_HAVE_ALREADY)
            {
                CPictureAlbum &downloadedAlbum = albumInfo.GetAlbum();
                downloadedAlbum.idAlbum = m_pictureDatabase.AddPictureAlbum(downloadedAlbum.strAlbum,
                                                                   downloadedAlbum.strPictureBrainzAlbumID,
                                                                   downloadedAlbum.GetFaceString(),
                                                                   downloadedAlbum.GetLocationString(),
                                                                   downloadedAlbum.iYear,
                                                                   downloadedAlbum.bCompilation);
                m_pictureDatabase.SetPictureAlbumInfo(downloadedAlbum.idAlbum,
                                             downloadedAlbum,
                                             downloadedAlbum.pictures);
                m_pictureDatabase.SetArtForItem(downloadedAlbum.idAlbum,
                                              "album", album->art);
                GetAlbumArtwork(downloadedAlbum.idAlbum, downloadedAlbum);
                m_albumCache.insert(make_pair(*album, albumInfo.GetAlbum()));
            }
            else if (albumDownloadStatus == INFO_CANCELLED)
                break;
            else
            {
                // No download info, fallback to already gathered (eg. local) information/art (if any)
                album->idAlbum = m_pictureDatabase.AddAlbum(album->strAlbum,
                                                          album->strPictureBrainzAlbumID,
                                                          album->GetFaceString(),
                                                          album->GetLocationString(),
                                                          album->iYear,
                                                          album->bCompilation);
                if (!album->art.empty())
                    m_pictureDatabase.SetArtForItem(album->idAlbum,
                                                  "album", album->art);
                m_albumCache.insert(make_pair(*album, *album));
            }
            
            // Update the cache pointer with our newly created info
            cachedAlbum = m_albumCache.find(*album);
             */
        }
        
        if (m_bStop)
            break;
        
        // Add the album faces
        for (VECFACECREDITS::iterator faceCredit = cachedAlbum->second.faceCredits.begin(); faceCredit != cachedAlbum->second.faceCredits.end(); ++faceCredit)
        {
            if (m_bStop)
                break;
            /*
            // Check if the face has already been downloaded or failed
            map<CFaceCredit, CFace>::iterator cachedFace = m_faceCache.find(*faceCredit);
            if (cachedFace == m_faceCache.end())
            {
                CFace faceTmp;
                faceTmp.strFace = faceCredit->GetFace();
                faceTmp.strPictureBrainzFaceID = faceCredit->GetPictureBrainzFaceID();
                URIUtils::GetParentPath(album->strPath, faceTmp.strPath);
                
                // No - download the information
                CPictureFaceInfo faceInfo;
                INFO_RET faceDownloadStatus = INFO_NOT_FOUND;
                if ((m_flags & SCAN_ONLINE) && faceScraper)
                    faceDownloadStatus = DownloadFaceInfo(faceTmp, faceScraper, faceInfo);
                
                if (faceDownloadStatus == INFO_ADDED || faceDownloadStatus == INFO_HAVE_ALREADY)
                {
                    CFace &downloadedFace = faceInfo.GetFace();
                    downloadedFace.idFace = m_pictureDatabase.AddFace(downloadedFace.strFace,
                                                                          downloadedFace.strPictureBrainzFaceID);
                    m_pictureDatabase.SetFaceInfo(downloadedFace.idFace,
                                                  downloadedFace);
                    
                    URIUtils::GetParentPath(album->strPath, downloadedFace.strPath);
                    map<string, string> artwork = GetFaceArtwork(downloadedFace);
                    // check thumb stuff
                    m_pictureDatabase.SetArtForItem(downloadedFace.idFace, "face", artwork);
                    m_faceCache.insert(make_pair(*faceCredit, downloadedFace));
                }
                else if (faceDownloadStatus == INFO_CANCELLED)
                    break;
                else
                {
                    // Cache the lookup failure so we don't retry
                    faceTmp.idFace = m_pictureDatabase.AddFace(faceCredit->GetFace(), faceCredit->GetPictureBrainzFaceID());
                    m_faceCache.insert(make_pair(*faceCredit, faceTmp));
                }
                
                // Update the cache pointer with our newly created info
                cachedFace = m_faceCache.find(*faceCredit);
            }
            
            m_pictureDatabase.AddAlbumFace(cachedFace->second.idFace,
                                           cachedAlbum->second.idAlbum,
                                           faceCredit->GetJoinPhrase(),
                                           faceCredit == album->faceCredits.begin() ? false : true,
                                           std::distance(cachedAlbum->second.faceCredits.begin(), faceCredit));
             */
        }
        
        if (m_bStop)
            break;
        
        for (VECPICTURES::iterator picture = album->pictures.begin(); picture != album->pictures.end(); ++picture)
        {
            picture->idAlbum = cachedAlbum->second.idAlbum;
            picture->idPicture = m_pictureDatabase.AddPicture(cachedAlbum->second.idAlbum,
                                                   picture->strTitle, picture->strPictureBrainzTrackID,
                                                   picture->strFileName, picture->strComment,
                                                   picture->strThumb,
                                                   picture->face, picture->location,
                                                   picture->iTrack, picture->iDuration, picture->iYear,
                                                   picture->iTimesPlayed, picture->iStartOffset,
                                                   picture->iEndOffset,
                                                   picture->lastPlayed,
                                                   picture->rating,
                                                   picture->iKaraokeNumber);
            for (VECFACECREDITS::iterator faceCredit = picture->faceCredits.begin(); faceCredit != picture->faceCredits.end(); ++faceCredit)
            {
                if (m_bStop)
                    break;
                /*
                // Check if the face has already been downloaded or failed
                map<CFaceCredit, CFace>::iterator cachedFace = m_faceCache.find(*faceCredit);
                if (cachedFace == m_faceCache.end())
                {
                    CFace faceTmp;
                    faceTmp.strFace = faceCredit->GetFace();
                    faceTmp.strPictureBrainzFaceID = faceCredit->GetPictureBrainzFaceID();
                    URIUtils::GetParentPath(album->strPath, faceTmp.strPath); // FIXME
                    
                    // No - download the information
                    CPictureFaceInfo faceInfo;
                    INFO_RET faceDownloadStatus = INFO_NOT_FOUND;
                    if ((m_flags & SCAN_ONLINE) && faceScraper)
                        faceDownloadStatus = DownloadFaceInfo(faceTmp, faceScraper, faceInfo);
                    
                    if (faceDownloadStatus == INFO_ADDED || faceDownloadStatus == INFO_HAVE_ALREADY)
                    {
                        CFace &downloadedFace = faceInfo.GetFace();
                        downloadedFace.idFace = m_pictureDatabase.AddFace(downloadedFace.strFace,
                                                                              downloadedFace.strPictureBrainzFaceID);
                        m_pictureDatabase.SetFaceInfo(downloadedFace.idFace,
                                                      downloadedFace);
                        // check thumb stuff
                        URIUtils::GetParentPath(album->strPath, downloadedFace.strPath);
                        map<string, string> artwork = GetFaceArtwork(downloadedFace);
                        m_pictureDatabase.SetArtForItem(downloadedFace.idFace, "face", artwork);
                        m_faceCache.insert(make_pair(*faceCredit, downloadedFace));
                    }
                    else if (faceDownloadStatus == INFO_CANCELLED)
                        break;
                    else
                    {
                        // Cache the lookup failure so we don't retry
                        faceTmp.idFace = m_pictureDatabase.AddFace(faceCredit->GetFace(), faceCredit->GetPictureBrainzFaceID());
                        m_faceCache.insert(make_pair(*faceCredit, faceTmp));
                    }
                    
                    // Update the cache pointer with our newly created info
                    cachedFace = m_faceCache.find(*faceCredit);
                }
                
                m_pictureDatabase.AddPictureFace(cachedFace->second.idFace,
                                              picture->idPicture,
                                              g_advancedSettings.m_pictureItemSeparator, // we don't have picture face breakdowns from scrapers, yet
                                              faceCredit == picture->faceCredits.begin() ? false : true,
                                              std::distance(picture->faceCredits.begin(), faceCredit));
                 */
            }
        }
        
        if (m_bStop)
            break;
        
        // Commit the album to the DB
        m_pictureDatabase.CommitTransaction();
        numAdded += album->pictures.size();
    }
    
    if (m_bStop)
        m_pictureDatabase.RollbackTransaction();
    
    if (m_handle)
        m_handle->SetTitle(g_localizeStrings.Get(505));
    
    return numAdded;
}

static bool SortPicturesByTrack(const CPicture& picture, const CPicture& picture2)
{
    return picture.iTrack < picture2.iTrack;
}

void CPictureInfoScanner::FixupAlbums(VECPICTUREALBUMS &albums)
{
    /*
     Step 2: Split into unique albums based on album name and album face
     In the case where the album face is unknown, we use the primary face
     (i.e. first face from each picture).
     */
    for (VECPICTUREALBUMS::iterator album = albums.begin(); album != albums.end(); ++album)
    {
        /*
         * If we have a valid PictureBrainz tag for the album, assume everything
         * is okay with our tags, as Picard should set everything up correctly.
         */
//        if (!album->strPictureBrainzAlbumID.IsEmpty())
//            continue;
        
        VECPICTURES &pictures = album->pictures;
        // sort the pictures by tracknumber to identify duplicate track numbers
        sort(pictures.begin(), pictures.end(), SortPicturesByTrack);
        
        // map the pictures to their primary faces
        bool tracksOverlap = false;
        bool hasAlbumFace = false;
        bool isCompilation = true;
        
        map<string, vector<CPicture *> > faces;
        for (VECPICTURES::iterator picture = pictures.begin(); picture != pictures.end(); ++picture)
        {
            // test for picture overlap
            if (picture != pictures.begin() && picture->iTrack == (picture - 1)->iTrack)
                tracksOverlap = true;
            
            if (!picture->bCompilation)
                isCompilation = false;
            
            // get primary face
            string primary;
            if (!picture->albumFace.empty())
            {
                primary = picture->albumFace[0];
                hasAlbumFace = true;
            }
            else if (!picture->face.empty())
                primary = picture->face[0];
            
            // add to the face map
            faces[primary].push_back(&(*picture));
        }
        
        /*
         We have a compilation if
         1. album name is non-empty AND
         2a. no tracks overlap OR
         2b. all tracks are marked as part of compilation AND
         3a. a unique primary face is specified as "various" or "various faces" OR
         3b. we have at least two primary faces and no album face specified.
         */
        bool compilation = !album->strAlbum.empty() && (isCompilation || !tracksOverlap); // 1+2b+2a
        if (faces.size() == 1)
        {
            string face = faces.begin()->first; StringUtils::ToLower(face);
            if (!StringUtils::EqualsNoCase(face, "various") &&
                !StringUtils::EqualsNoCase(face, "various faces")) // 3a
                compilation = false;
        }
        else if (hasAlbumFace) // 3b
            compilation = false;
        
        if (compilation)
        {
            CLog::Log(LOGDEBUG, "Album '%s' is a compilation as there's no overlapping tracks and %s", album->strAlbum.c_str(), hasAlbumFace ? "the album face is 'Various'" : "there is more than one unique face");
            faces.clear();
            std::string various = g_localizeStrings.Get(340); // Various Faces
            vector<string> va; va.push_back(various);
            for (VECPICTURES::iterator picture = pictures.begin(); picture != pictures.end(); ++picture)
            {
                picture->albumFace = va;
                faces[various].push_back(&(*picture));
            }
        }
        
        /*
         Step 3: Find the common albumface for each picture and assign
         albumface to those tracks that don't have it set.
         */
        for (map<string, vector<CPicture *> >::iterator j = faces.begin(); j != faces.end(); ++j)
        {
            // find the common face for these pictures
            vector<CPicture *> &facePictures = j->second;
            vector<string> common = facePictures.front()->albumFace.empty() ? facePictures.front()->face : facePictures.front()->albumFace;
            for (vector<CPicture *>::iterator k = facePictures.begin() + 1; k != facePictures.end(); ++k)
            {
                unsigned int match = 0;
                vector<string> &compare = (*k)->albumFace.empty() ? (*k)->face : (*k)->albumFace;
                for (; match < common.size() && match < compare.size(); match++)
                {
                    if (compare[match] != common[match])
                        break;
                }
                common.erase(common.begin() + match, common.end());
            }
            
            /*
             Step 4: Assign the album face for each picture that doesn't have it set
             and add to the album vector
             */
            album->faceCredits.clear();
            for (vector<string>::iterator it = common.begin(); it != common.end(); ++it)
            {
                CStdString strJoinPhrase = (it == --common.end() ? "" : g_advancedSettings.m_pictureItemSeparator);
                CFaceCredit faceCredit(*it, strJoinPhrase);
                album->faceCredits.push_back(faceCredit);
            }
            album->bCompilation = compilation;
            for (vector<CPicture *>::iterator k = facePictures.begin(); k != facePictures.end(); ++k)
            {
                if ((*k)->albumFace.empty())
                    (*k)->albumFace = common;
                // TODO: in future we may wish to union up the locations, for now we assume they're the same
                if (album->location.empty())
                    album->location = (*k)->location;
                //       in addition, we may want to use year as discriminating for albums
                if (album->iYear == 0)
                    album->iYear = (*k)->iYear;
            }
        }
    }
}

void CPictureInfoScanner::FindArtForAlbums(VECPICTUREALBUMS &albums, const CStdString &path)
{
    /*
     If there's a single album in the folder, then art can be taken from
     the folder art.
     */
    std::string albumArt;
    if (albums.size() == 1)
    {
        CFileItem album(path, true);
//        albumArt = album.GetUserPictureThumb(true);
        if (!albumArt.empty())
            albums[0].art["thumb"] = albumArt;
    }
    for (VECPICTUREALBUMS::iterator i = albums.begin(); i != albums.end(); ++i)
    {
        CPictureAlbum &album = *i;
        
        if (albums.size() != 1)
            albumArt = "";
        
        /*
         Find art that is common across these items
         If we find a single art image we treat it as the album art
         and discard picture art else we use first as album art and
         keep everything as picture art.
         */
        bool singleArt = true;
        CPicture *art = NULL;
        for (VECPICTURES::iterator k = album.pictures.begin(); k != album.pictures.end(); ++k)
        {
            CPicture &picture = *k;
            if (picture.HasArt())
            {
                if (art && !art->ArtMatches(picture))
                {
                    singleArt = false;
                    break;
                }
                if (!art)
                    art = &picture;
            }
        }
        
        /*
         assign the first art found to the album - better than no art at all
         */
        
        if (art && albumArt.empty())
        {
            if (!art->strThumb.empty())
                albumArt = art->strThumb;
            else
                albumArt = CTextureCache::GetWrappedImageURL(art->strFileName, "picture");
        }
        
        if (!albumArt.empty())
            album.art["thumb"] = albumArt;
        
        if (singleArt)
        { //if singleArt then we can clear the artwork for all pictures
            for (VECPICTURES::iterator k = album.pictures.begin(); k != album.pictures.end(); ++k)
                k->strThumb.clear();
        }
        else
        { // more than one piece of art was found for these pictures, so cache per picture
            for (VECPICTURES::iterator k = album.pictures.begin(); k != album.pictures.end(); ++k)
            {
                //if (k->strThumb.empty() && !k->embeddedArt.empty())
                //    k->strThumb = CTextureCache::GetWrappedImageURL(k->strFileName, "picture");
            }
        }
    }
    if (albums.size() == 1 && !albumArt.empty())
    {
        // assign to folder thumb as well
        CFileItem albumItem(path, true);
        CPictureThumbLoader::SetCachedImage(albumItem, "thumb", albumArt);
    }
}

int CPictureInfoScanner::GetPathHash(const CFileItemList &items, CStdString &hash)
{
    // Create a hash based on the filenames, filesize and filedate.  Also count the number of files
    if (0 == items.Size()) return 0;
    XBMC::XBMC_MD5 md5state;
    int count = 0;
    for (int i = 0; i < items.Size(); ++i)
    {
        const CFileItemPtr pItem = items[i];
        md5state.append(pItem->GetPath());
        md5state.append((unsigned char *)&pItem->m_dwSize, sizeof(pItem->m_dwSize));
        FILETIME time = pItem->m_dateTime;
        md5state.append((unsigned char *)&time, sizeof(FILETIME));
        if (pItem->IsAudio() && !pItem->IsPlayList() && !pItem->IsNFO())
            count++;
    }
    md5state.getDigest(hash);
    return count;
}

INFO_RET CPictureInfoScanner::UpdateDatabaseAlbumInfo(const CStdString& strPath, CPictureAlbumInfo& albumInfo, bool bAllowSelection, CGUIDialogProgress* pDialog /* = NULL */)
{
    m_pictureDatabase.Open();
    CQueryParams params;
    CDirectoryNode::GetDatabaseInfo(strPath, params);
    
//    if (params.GetAlbumId() == -1)
//        return INFO_ERROR;
    
    CPictureAlbum album;
//    m_pictureDatabase.GetPictureAlbumInfo(params.GetAlbumId(), album, &album.pictures);
    
    // find album info
    ADDON::ScraperPtr scraper;
    bool result = m_pictureDatabase.GetScraperForPath(strPath, scraper, ADDON::ADDON_SCRAPER_ALBUMS);
    
    m_pictureDatabase.Close();
    
    if (!result || !scraper)
        return INFO_ERROR;
    
loop:
    CLog::Log(LOGDEBUG, "%s downloading info for: %s", __FUNCTION__, album.strAlbum.c_str());
    INFO_RET albumDownloadStatus = DownloadAlbumInfo(album, scraper, albumInfo, pDialog);
    if (albumDownloadStatus == INFO_NOT_FOUND)
    {
        if (pDialog && bAllowSelection)
        {
            if (!CGUIKeyboardFactory::ShowAndGetInput(album.strAlbum, g_localizeStrings.Get(16011), false))
                return INFO_CANCELLED;
            
            CStdString strTempFace(StringUtils::Join(album.face, g_advancedSettings.m_pictureItemSeparator));
            if (!CGUIKeyboardFactory::ShowAndGetInput(strTempFace, g_localizeStrings.Get(16025), false))
                return INFO_CANCELLED;
            
            album.face = StringUtils::Split(strTempFace, g_advancedSettings.m_pictureItemSeparator);
            goto loop;
        }
    }
    else if (albumDownloadStatus == INFO_ADDED)
    {
        m_pictureDatabase.Open();
//        m_pictureDatabase.SetPictureAlbumInfo(params.GetAlbumId(), albumInfo.GetAlbum(), albumInfo.GetAlbum().pictures);
//        GetAlbumArtwork(params.GetAlbumId(), albumInfo.GetAlbum());
        albumInfo.SetLoaded(true);
        m_pictureDatabase.Close();
    }
    return albumDownloadStatus;
}

INFO_RET CPictureInfoScanner::UpdateDatabaseFaceInfo(const CStdString& strPath, CPictureFaceInfo& faceInfo, bool bAllowSelection, CGUIDialogProgress* pDialog /* = NULL */)
{
    m_pictureDatabase.Open();
    CQueryParams params;
    CDirectoryNode::GetDatabaseInfo(strPath, params);
    
    if (params.GetFaceId() == -1)
        return INFO_ERROR;
    
    CFace face;
    m_pictureDatabase.GetFaceInfo(params.GetFaceId(), face);
    
    // find album info
    ADDON::ScraperPtr scraper;
    if (!m_pictureDatabase.GetScraperForPath(strPath, scraper, ADDON::ADDON_SCRAPER_ARTISTS) || !scraper)
        return INFO_ERROR;
    m_pictureDatabase.Close();
    
loop:
    CLog::Log(LOGDEBUG, "%s downloading info for: %s", __FUNCTION__, face.strFace.c_str());
    INFO_RET faceDownloadStatus = DownloadFaceInfo(face, scraper, faceInfo, pDialog);
    if (faceDownloadStatus == INFO_NOT_FOUND)
    {
        if (pDialog && bAllowSelection)
        {
            if (!CGUIKeyboardFactory::ShowAndGetInput(face.strFace, g_localizeStrings.Get(16025), false))
                return INFO_CANCELLED;
            goto loop;
        }
    }
    else if (faceDownloadStatus == INFO_ADDED)
    {
        m_pictureDatabase.Open();
        m_pictureDatabase.SetFaceInfo(params.GetFaceId(), faceInfo.GetFace());
        m_pictureDatabase.GetFacePath(params.GetFaceId(), face.strPath);
        map<string, string> artwork = GetFaceArtwork(face);
        m_pictureDatabase.SetArtForItem(params.GetFaceId(), "face", artwork);
        faceInfo.SetLoaded();
        m_pictureDatabase.Close();
    }
    return faceDownloadStatus;
}

#define THRESHOLD .95f

INFO_RET CPictureInfoScanner::DownloadAlbumInfo(const CPictureAlbum& album, ADDON::ScraperPtr& info, CPictureAlbumInfo& albumInfo, CGUIDialogProgress* pDialog)
{
    if (m_handle)
    {
        m_handle->SetTitle(StringUtils::Format(g_localizeStrings.Get(20321), info->Name().c_str()));
        m_handle->SetText(StringUtils::Join(album.face, g_advancedSettings.m_pictureItemSeparator) + " - " + album.strAlbum);
    }
    
    // clear our scraper cache
    info->ClearCache();
    
    CPictureInfoScraper scraper(info);
    
    bool bPictureBrainz = false;
    /*
    if (!album.strPictureBrainzAlbumID.empty())
    {
        CScraperUrl pictureBrainzURL;
        if (ResolvePictureBrainz(album.strPictureBrainzAlbumID, info, scraper, pictureBrainzURL))
        {
            CPictureAlbumInfo albumNfo("nfo", pictureBrainzURL);
            scraper.GetAlbums().clear();
            scraper.GetAlbums().push_back(albumNfo);
            bPictureBrainz = true;
        }
    }
     */
    
    // handle nfo files
    CStdString strNfo = URIUtils::AddFileToFolder(album.strPath, "album.nfo");
    CNfoFile::NFOResult result = CNfoFile::NO_NFO;
    CNfoFile nfoReader;
    if (XFILE::CFile::Exists(strNfo))
    {
        CLog::Log(LOGDEBUG,"Found matching nfo file: %s", strNfo.c_str());
        result = nfoReader.Create(strNfo, info, -1, album.strPath);
        if (result == CNfoFile::FULL_NFO)
        {
            CLog::Log(LOGDEBUG, "%s Got details from nfo", __FUNCTION__);
            nfoReader.GetDetails(albumInfo.GetAlbum());
            return INFO_ADDED;
        }
        else if (result == CNfoFile::URL_NFO || result == CNfoFile::COMBINED_NFO)
        {
            CScraperUrl scrUrl(nfoReader.ScraperUrl());
            CPictureAlbumInfo albumNfo("nfo",scrUrl);
            info = nfoReader.GetScraperInfo();
            CLog::Log(LOGDEBUG,"-- nfo-scraper: %s",info->Name().c_str());
            CLog::Log(LOGDEBUG,"-- nfo url: %s", scrUrl.m_url[0].m_url.c_str());
            scraper.SetScraperInfo(info);
            scraper.GetAlbums().clear();
            scraper.GetAlbums().push_back(albumNfo);
        }
        else
            CLog::Log(LOGERROR,"Unable to find an url in nfo file: %s", strNfo.c_str());
    }
    
    if (!scraper.CheckValidOrFallback(CSettings::Get().GetString("picturelibrary.albumsscraper")))
    { // the current scraper is invalid, as is the default - bail
        CLog::Log(LOGERROR, "%s - current and default scrapers are invalid.  Pick another one", __FUNCTION__);
        return INFO_ERROR;
    }
    
    if (!scraper.GetAlbumCount())
    {
        scraper.FindAlbumInfo(album.strAlbum, StringUtils::Join(album.face, g_advancedSettings.m_pictureItemSeparator));
        
        while (!scraper.Completed())
        {
            if (m_bStop)
            {
                scraper.Cancel();
                return INFO_CANCELLED;
            }
            Sleep(1);
        }
    }
    
    CGUIDialogSelect *pDlg = NULL;
    int iSelectedAlbum=0;
    if (result == CNfoFile::NO_NFO && !bPictureBrainz)
    {
        iSelectedAlbum = -1; // set negative so that we can detect a failure
        if (scraper.Succeeded() && scraper.GetAlbumCount() >= 1)
        {
            double bestRelevance = 0;
            double minRelevance = THRESHOLD;
            if (scraper.GetAlbumCount() > 1) // score the matches
            {
                //show dialog with all albums found
                if (pDialog)
                {
                    pDlg = (CGUIDialogSelect*)g_windowManager.GetWindow(WINDOW_DIALOG_SELECT);
                    pDlg->SetHeading(g_localizeStrings.Get(181).c_str());
                    pDlg->Reset();
                    pDlg->EnableButton(true, 413); // manual
                }
                
                for (int i = 0; i < scraper.GetAlbumCount(); ++i)
                {
                    CPictureAlbumInfo& info = scraper.GetAlbum(i);
                    double relevance = info.GetRelevance();
                    if (relevance < 0)
                        relevance = CUtil::AlbumRelevance(info.GetAlbum().strAlbum, album.strAlbum, StringUtils::Join(info.GetAlbum().face, g_advancedSettings.m_pictureItemSeparator), StringUtils::Join(album.face, g_advancedSettings.m_pictureItemSeparator));
                    
                    // if we're doing auto-selection (ie querying all albums at once, then allow 95->100% for perfect matches)
                    // otherwise, perfect matches only
                    if (relevance >= max(minRelevance, bestRelevance))
                    { // we auto-select the best of these
                        bestRelevance = relevance;
                        iSelectedAlbum = i;
                    }
                    if (pDialog)
                    {
                        // set the label to [relevance]  album - face
                        CStdString strTemp;
                        strTemp.Format("[%0.2f]  %s", relevance, info.GetTitle2());
                        CFileItem item(strTemp);
                        item.m_idepth = i; // use this to hold the index of the album in the scraper
                        pDlg->Add(&item);
                    }
                    if (relevance > .99f) // we're so close, no reason to search further
                        break;
                }
                
                if (pDialog && bestRelevance < THRESHOLD)
                {
                    pDlg->Sort(false);
                    pDlg->DoModal();
                    
                    // and wait till user selects one
                    if (pDlg->GetSelectedLabel() < 0)
                    { // none chosen
                        if (!pDlg->IsButtonPressed())
                            return INFO_CANCELLED;
                        
                        // manual button pressed
                        CStdString strNewAlbum = album.strAlbum;
                        if (!CGUIKeyboardFactory::ShowAndGetInput(strNewAlbum, g_localizeStrings.Get(16011), false)) return INFO_CANCELLED;
                        if (strNewAlbum == "") return INFO_CANCELLED;
                        
                        CStdString strNewFace = StringUtils::Join(album.face, g_advancedSettings.m_pictureItemSeparator);
                        if (!CGUIKeyboardFactory::ShowAndGetInput(strNewFace, g_localizeStrings.Get(16025), false)) return INFO_CANCELLED;
                        
                        pDialog->SetLine(0, strNewAlbum);
                        pDialog->SetLine(1, strNewFace);
                        pDialog->Progress();
                        
                        CPictureAlbum newAlbum = album;
                        newAlbum.strAlbum = strNewAlbum;
                        newAlbum.face = StringUtils::Split(strNewFace, g_advancedSettings.m_pictureItemSeparator);
                        
                        return DownloadAlbumInfo(newAlbum, info, albumInfo, pDialog);
                    }
                    iSelectedAlbum = pDlg->GetSelectedItem()->m_idepth;
                }
            }
            else
            {
                CPictureAlbumInfo& info = scraper.GetAlbum(0);
                double relevance = info.GetRelevance();
                if (relevance < 0)
                    relevance = CUtil::AlbumRelevance(info.GetAlbum().strAlbum,
                                                      album.strAlbum,
                                                      StringUtils::Join(info.GetAlbum().face, g_advancedSettings.m_pictureItemSeparator),
                                                      StringUtils::Join(album.face, g_advancedSettings.m_pictureItemSeparator));
                if (relevance < THRESHOLD)
                    return INFO_NOT_FOUND;
                
                iSelectedAlbum = 0;
            }
        }
        
        if (iSelectedAlbum < 0)
            return INFO_NOT_FOUND;
        
    }
    
    scraper.LoadAlbumInfo(iSelectedAlbum);
    while (!scraper.Completed())
    {
        if (m_bStop)
        {
            scraper.Cancel();
            return INFO_CANCELLED;
        }
        Sleep(1);
    }
    
    if (!scraper.Succeeded())
        return INFO_ERROR;
    
    albumInfo = scraper.GetAlbum(iSelectedAlbum);
    
    if (result == CNfoFile::COMBINED_NFO)
        nfoReader.GetDetails(albumInfo.GetAlbum(), NULL, true);
    
    return INFO_ADDED;
}

void CPictureInfoScanner::GetAlbumArtwork(long id, const CPictureAlbum &album)
{
    if (album.thumbURL.m_url.size())
    {
        if (m_pictureDatabase.GetArtForItem(id, "album", "thumb").empty())
        {
            string thumb = CScraperUrl::GetThumbURL(album.thumbURL.GetFirstThumb());
            if (!thumb.empty())
            {
                CTextureCache::Get().BackgroundCacheImage(thumb);
                m_pictureDatabase.SetArtForItem(id, "album", "thumb", thumb);
            }
        }
    }
}

INFO_RET CPictureInfoScanner::DownloadFaceInfo(const CFace& face, ADDON::ScraperPtr& info, PICTURE_GRABBER::CPictureFaceInfo& faceInfo, CGUIDialogProgress* pDialog)
{
    if (m_handle)
    {
        m_handle->SetTitle(StringUtils::Format(g_localizeStrings.Get(20320), info->Name().c_str()));
        m_handle->SetText(face.strFace);
    }
    
    // clear our scraper cache
    info->ClearCache();
    
    CPictureInfoScraper scraper(info);
    bool bPictureBrainz = false;
    /*
    if (!face.strPictureBrainzFaceID.empty())
    {
        CScraperUrl pictureBrainzURL;
        if (ResolvePictureBrainz(face.strPictureBrainzFaceID, info, scraper, pictureBrainzURL))
        {
            CPictureFaceInfo faceNfo("nfo", pictureBrainzURL);
            scraper.GetFaces().clear();
            scraper.GetFaces().push_back(faceNfo);
            bPictureBrainz = true;
        }
    }
    */
    // handle nfo files
    CStdString strNfo = URIUtils::AddFileToFolder(face.strPath, "face.nfo");
    CNfoFile::NFOResult result=CNfoFile::NO_NFO;
    CNfoFile nfoReader;
    if (XFILE::CFile::Exists(strNfo))
    {
        CLog::Log(LOGDEBUG,"Found matching nfo file: %s", strNfo.c_str());
        result = nfoReader.Create(strNfo, info);
        if (result == CNfoFile::FULL_NFO)
        {
            CLog::Log(LOGDEBUG, "%s Got details from nfo", __FUNCTION__);
            nfoReader.GetDetails(faceInfo.GetFace());
            return INFO_ADDED;
        }
        else if (result == CNfoFile::URL_NFO || result == CNfoFile::COMBINED_NFO)
        {
            CScraperUrl scrUrl(nfoReader.ScraperUrl());
            CPictureFaceInfo faceNfo("nfo",scrUrl);
            info = nfoReader.GetScraperInfo();
            CLog::Log(LOGDEBUG,"-- nfo-scraper: %s",info->Name().c_str());
            CLog::Log(LOGDEBUG,"-- nfo url: %s", scrUrl.m_url[0].m_url.c_str());
            scraper.SetScraperInfo(info);
            scraper.GetFaces().push_back(faceNfo);
        }
        else
            CLog::Log(LOGERROR,"Unable to find an url in nfo file: %s", strNfo.c_str());
    }
    
    if (!scraper.GetFaceCount())
    {
        scraper.FindFaceInfo(face.strFace);
        
        while (!scraper.Completed())
        {
            if (m_bStop)
            {
                scraper.Cancel();
                return INFO_CANCELLED;
            }
            Sleep(1);
        }
    }
    
    int iSelectedFace = 0;
    if (result == CNfoFile::NO_NFO && !bPictureBrainz)
    {
        if (scraper.GetFaceCount() >= 1)
        {
            // now load the first match
            if (pDialog && scraper.GetFaceCount() > 1)
            {
                // if we found more then 1 album, let user choose one
                CGUIDialogSelect *pDlg = (CGUIDialogSelect*)g_windowManager.GetWindow(WINDOW_DIALOG_SELECT);
                if (pDlg)
                {
                    pDlg->SetHeading(g_localizeStrings.Get(21890));
                    pDlg->Reset();
                    pDlg->EnableButton(true, 413); // manual
                    
                    for (int i = 0; i < scraper.GetFaceCount(); ++i)
                    {
                        /*
                        // set the label to face
                        CFileItem item(scraper.GetFace(i).GetFace());
                        CStdString strTemp=scraper.GetFace(i).GetFace().strFace;
                        if (!scraper.GetFace(i).GetFace().strBorn.IsEmpty())
                            strTemp += " ("+scraper.GetFace(i).GetFace().strBorn+")";
                        if (!scraper.GetFace(i).GetFace().location.empty())
                        {
                            CStdString locations = StringUtils::Join(scraper.GetFace(i).GetFace().location, g_advancedSettings.m_pictureItemSeparator);
                            if (!locations.empty())
                                strTemp.Format("[%s] %s", locations.c_str(), strTemp.c_str());
                        }
                        item.SetLabel(strTemp);
                        item.m_idepth = i; // use this to hold the index of the album in the scraper
                        pDlg->Add(&item);
                         */
                    }
                    pDlg->DoModal();
                    
                    // and wait till user selects one
                    if (pDlg->GetSelectedLabel() < 0)
                    { // none chosen
                        if (!pDlg->IsButtonPressed())
                            return INFO_CANCELLED;
                        
                        // manual button pressed
                        CStdString strNewFace = face.strFace;
                        if (!CGUIKeyboardFactory::ShowAndGetInput(strNewFace, g_localizeStrings.Get(16025), false)) return INFO_CANCELLED;
                        
                        if (pDialog)
                        {
                            pDialog->SetLine(0, strNewFace);
                            pDialog->Progress();
                        }
                        
                        CFace newFace;
                        newFace.strFace = strNewFace;
                        return DownloadFaceInfo(newFace, info, faceInfo, pDialog);
                    }
                    iSelectedFace = pDlg->GetSelectedItem()->m_idepth;
                }
            }
        }
        else
            return INFO_NOT_FOUND;
    }
    
    scraper.LoadFaceInfo(iSelectedFace, face.strFace);
    while (!scraper.Completed())
    {
        if (m_bStop)
        {
            scraper.Cancel();
            return INFO_CANCELLED;
        }
        Sleep(1);
    }
    
    if (!scraper.Succeeded())
        return INFO_ERROR;
    
    faceInfo = scraper.GetFace(iSelectedFace);
    
    if (result == CNfoFile::COMBINED_NFO)
        nfoReader.GetDetails(faceInfo.GetFace(), NULL, true);
    
    return INFO_ADDED;
}

bool CPictureInfoScanner::ResolvePictureBrainz(const CStdString strPictureBrainzID, ScraperPtr &preferredScraper, CPictureInfoScraper &pictureInfoScraper, CScraperUrl &pictureBrainzURL)
{
    // We have a PictureBrainz ID
    // Get a scraper that can resolve it to a PictureBrainz URL & force our
    // search directly to the specific album.
    bool bPictureBrainz = false;
    ADDON::TYPE type = ScraperTypeFromContent(preferredScraper->Content());
    
    CFileItemList items;
    ADDON::AddonPtr addon;
    ADDON::ScraperPtr defaultScraper;
    if (ADDON::CAddonMgr::Get().GetDefault(type, addon))
        defaultScraper = boost::dynamic_pointer_cast<CScraper>(addon);
    
    vector<ScraperPtr> vecScrapers;
    
    // add selected scraper - first proirity
    if (preferredScraper)
        vecScrapers.push_back(preferredScraper);
    
    // Add all scrapers except selected and default
    VECADDONS addons;
    CAddonMgr::Get().GetAddons(type, addons);
    
    for (unsigned i = 0; i < addons.size(); ++i)
    {
        ScraperPtr scraper = boost::dynamic_pointer_cast<CScraper>(addons[i]);
        
        // skip if scraper requires settings and there's nothing set yet
        if (!scraper || (scraper->RequiresSettings() && !scraper->HasUserSettings()))
            continue;
        
        if((!preferredScraper || preferredScraper->ID() != scraper->ID()) && (!defaultScraper || defaultScraper->ID() != scraper->ID()) )
            vecScrapers.push_back(scraper);
    }
    
    // add default scraper - not user selectable so it's last priority
    if(defaultScraper && 
       (!preferredScraper || preferredScraper->ID() != defaultScraper->ID()) &&
       (!defaultScraper->RequiresSettings() || defaultScraper->HasUserSettings()))
        vecScrapers.push_back(defaultScraper);
    
    for (unsigned int i=0; i < vecScrapers.size(); ++i)
    {
        if (vecScrapers[i]->Type() != type)
            continue;
        
        vecScrapers[i]->ClearCache();
        try
        {
            pictureBrainzURL = vecScrapers[i]->ResolveIDToUrl(strPictureBrainzID);
        }
        catch (const ADDON::CScraperError &sce)
        {
            if (!sce.FAborted())
                continue;
        }
        if (!pictureBrainzURL.m_url.empty())
        {
            Sleep(2000); // PictureBrainz rate-limits queries to 1 p.s - once we hit the rate-limiter
            // they start serving up the 'you hit the rate-limiter' page fast - meaning
            // we will never get below the rate-limit threshold again in a specific run. 
            // This helps us to avoidthe rate-limiter as far as possible.
            CLog::Log(LOGDEBUG,"-- nfo-scraper: %s",vecScrapers[i]->Name().c_str());
            CLog::Log(LOGDEBUG,"-- nfo url: %s", pictureBrainzURL.m_url[0].m_url.c_str());
            pictureInfoScraper.SetScraperInfo(vecScrapers[i]);
            bPictureBrainz = true;
            break;
        }
    }
    
    return bPictureBrainz;
}

map<string, string> CPictureInfoScanner::GetFaceArtwork(const CFace& face)
{
    map<string, string> artwork;
    
    // check thumb
    CStdString strFolder;
    CStdString thumb;
    if (!face.strPath.IsEmpty())
    {
        strFolder = face.strPath;
        for (int i = 0; i < 3 && thumb.IsEmpty(); ++i)
        {
            CFileItem item(strFolder, true);
            //thumb = item.GetUserPictureThumb(true);
            strFolder = URIUtils::GetParentPath(strFolder);
        }
    }
    if (thumb.IsEmpty())
        thumb = CScraperUrl::GetThumbURL(face.thumbURL.GetFirstThumb());
    if (!thumb.IsEmpty())
    {
        CTextureCache::Get().BackgroundCacheImage(thumb);
        artwork.insert(make_pair("thumb", thumb));
    }
    
    // check fanart
    CStdString fanart;
    if (!face.strPath.IsEmpty())
    {
        strFolder = face.strPath;
        for (int i = 0; i < 3 && fanart.IsEmpty(); ++i)
        {
            CFileItem item(strFolder, true);
            fanart = item.GetLocalFanart();
            strFolder = URIUtils::GetParentPath(strFolder);
        }
    }
    if (fanart.IsEmpty())
        fanart = face.fanart.GetImageURL();
    if (!fanart.IsEmpty())
    {
        CTextureCache::Get().BackgroundCacheImage(fanart);
        artwork.insert(make_pair("fanart", fanart));
    }
    
    return artwork;
}

// This function is the Run() function of the IRunnable
// CFileCountReader and runs in a separate thread.
void CPictureInfoScanner::Run()
{
    int count = 0;
    for (set<std::string>::iterator it = m_pathsToScan.begin(); it != m_pathsToScan.end() && !m_bStop; ++it)
    {
        count+=CountFilesRecursively(*it);
    }
    m_itemCount = count;
}

// Recurse through all folders we scan and count files
int CPictureInfoScanner::CountFilesRecursively(const CStdString& strPath)
{
    // load subfolder
    CFileItemList items;
    CDirectory::GetDirectory(strPath, items, g_advancedSettings.m_pictureExtensions, DIR_FLAG_NO_FILE_DIRS);
    
    if (m_bStop)
        return 0;
    
    // true for recursive counting
    int count = CountFiles(items, true);
    return count;
}
 
int CPictureInfoScanner::CountFiles(const CFileItemList &items, bool recursive)
{
    int count = 0;
    for (int i=0; i<items.Size(); ++i)
    {
        const CFileItemPtr pItem=items[i];
        
        if (recursive && pItem->m_bIsFolder)
            count+=CountFilesRecursively(pItem->GetPath());
        else if (pItem->IsAudio() && !pItem->IsPlayList() && !pItem->IsNFO())
            count++;
    }
    return count;
}
