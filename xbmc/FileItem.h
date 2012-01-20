/*!
 \file FileItem.h
 \brief
 */
#pragma once

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

#include "guilib/GUIListItem.h"
#include "utils/Archive.h"
#include "utils/ISerializable.h"
#include "XBDateTime.h"
#include "SortFileItem.h"
#include "utils/LabelFormatter.h"
#include "GUIPassword.h"
#include "threads/CriticalSection.h"

#include <vector>
#include "boost/shared_ptr.hpp"

namespace MUSIC_INFO
{
  class CMusicInfoTag;
}
class CVideoInfoTag;
namespace EPG
{
  class CEpgInfoTag;
}
namespace PVR
{
  class CPVRChannel;
  class CPVRRecording;
  class CPVRTimerInfoTag;
}
class CPictureInfoTag;

class CAlbum;
class CArtist;
class CSong;
class CGenre;

class CURL;

/* special startoffset used to indicate that we wish to resume */
#define STARTOFFSET_RESUME (-1)

class CMediaSource;

/*!
  \brief Represents a file on a share
  \sa CFileItemList
  */
class CFileItem :
  public CGUIListItem, public IArchivable, public ISerializable
{
public:
  CFileItem(void);
  CFileItem(const CFileItem& item);
  CFileItem(const CGUIListItem& item);
  CFileItem(const CStdString& strLabel);
  CFileItem(const CStdString& strPath, bool bIsFolder);
  CFileItem(const CSong& song);
  CFileItem(const CStdString &path, const CAlbum& album);
  CFileItem(const CArtist& artist);
  CFileItem(const CGenre& genre);
  CFileItem(const CVideoInfoTag& movie);
  CFileItem(const EPG::CEpgInfoTag& tag);
  CFileItem(const PVR::CPVRChannel& channel);
  CFileItem(const PVR::CPVRRecording& record);
  CFileItem(const PVR::CPVRTimerInfoTag& timer);
  CFileItem(const CMediaSource& share);
  virtual ~CFileItem(void);
  virtual CGUIListItem *Clone() const { return new CFileItem(*this); };

  const CStdString &GetPath() const { return m_strPath; };
  void SetPath(const CStdString &path) { m_strPath = path; };

  void Reset();
  const CFileItem& operator=(const CFileItem& item);
  virtual void Archive(CArchive& ar);
  virtual void Serialize(CVariant& value);
  virtual bool IsFileItem() const { return true; };

  bool Exists(bool bUseCache = true) const;
  bool IsVideo() const;
  bool IsDiscStub() const;
  bool IsPicture() const;
  bool IsLyrics() const;
  bool IsAudio() const;
  bool IsKaraoke() const;
  bool IsCUESheet() const;
  bool IsLastFM() const;
  bool IsInternetStream(const bool bStrictCheck = false) const;
  bool IsPlayList() const;
  bool IsSmartPlayList() const;
  bool IsPythonScript() const;
  bool IsXBE() const;
  bool IsPlugin() const;
  bool IsScript() const;
  bool IsAddonsPath() const;
  bool IsSourcesPath() const;
  bool IsShortCut() const;
  bool IsNFO() const;
  bool IsDVDImage() const;
  bool IsOpticalMediaFile() const;
  bool IsDVDFile(bool bVobs = true, bool bIfos = true) const;
  bool IsBDFile() const;
  bool IsRAR() const;
  bool IsZIP() const;
  bool IsCBZ() const;
  bool IsCBR() const;
  bool IsISO9660() const;
  bool IsCDDA() const;
  bool IsDVD() const;
  bool IsOnDVD() const;
  bool IsOnLAN() const;
  bool IsHD() const;
  bool IsNfs() const;  
  bool IsAfp() const;    
  bool IsRemote() const;
  bool IsSmb() const;
  bool IsURL() const;
  bool IsDAAP() const;
  bool IsStack() const;
  bool IsMultiPath() const;
  bool IsMusicDb() const;
  bool IsVideoDb() const;
  bool IsEPG() const;
  bool IsPVRChannel() const;
  bool IsPVRRecording() const;
  bool IsPVRTimer() const;
  bool IsType(const char *ext) const;
  bool IsVirtualDirectoryRoot() const;
  bool IsReadOnly() const;
  bool CanQueue() const;
  void SetCanQueue(bool bYesNo);
  bool IsParentFolder() const;
  bool IsFileFolder() const;
  bool IsRemovable() const;
  bool IsTuxBox() const;
  bool IsMythTV() const;
  bool IsHDHomeRun() const;
  bool IsSlingbox() const;
  bool IsVTP() const;
  bool IsPVR() const;
  bool IsLiveTV() const;
  bool IsRSS() const;

  void RemoveExtension();
  void CleanString();
  void FillInDefaultIcon();
  void SetMusicThumb(bool alwaysCheckRemote = false);
  void SetFileSizeLabel();
  virtual void SetLabel(const CStdString &strLabel);
  virtual void SetLabel2(const CStdString &strLabel);
  CURL GetAsUrl() const;
  int GetVideoContentType() const; /* return VIDEODB_CONTENT_TYPE, but don't want to include videodb in this header */
  bool IsLabelPreformated() const { return m_bLabelPreformated; }
  void SetLabelPreformated(bool bYesNo) { m_bLabelPreformated=bYesNo; }
  bool SortsOnTop() const { return m_specialSort == SORT_ON_TOP; }
  bool SortsOnBottom() const { return m_specialSort == SORT_ON_BOTTOM; }
  void SetSpecialSort(SPECIAL_SORT sort) { m_specialSort = sort; }

  inline bool HasMusicInfoTag() const
  {
    return m_musicInfoTag != NULL;
  }

  MUSIC_INFO::CMusicInfoTag* GetMusicInfoTag();

  inline const MUSIC_INFO::CMusicInfoTag* GetMusicInfoTag() const
  {
    return m_musicInfoTag;
  }

  inline bool HasVideoInfoTag() const
  {
    return m_videoInfoTag != NULL;
  }

  CVideoInfoTag* GetVideoInfoTag();

  inline const CVideoInfoTag* GetVideoInfoTag() const
  {
    return m_videoInfoTag;
  }

  inline bool HasEPGInfoTag() const
  {
    return m_epgInfoTag != NULL;
  }

  EPG::CEpgInfoTag* GetEPGInfoTag();

  inline const EPG::CEpgInfoTag* GetEPGInfoTag() const
  {
    return m_epgInfoTag;
  }

  inline bool HasPVRChannelInfoTag() const
  {
    return m_pvrChannelInfoTag != NULL;
  }

  PVR::CPVRChannel* GetPVRChannelInfoTag();

  inline const PVR::CPVRChannel* GetPVRChannelInfoTag() const
  {
    return m_pvrChannelInfoTag;
  }

  inline bool HasPVRRecordingInfoTag() const
  {
    return m_pvrRecordingInfoTag != NULL;
  }

  PVR::CPVRRecording* GetPVRRecordingInfoTag();

  inline const PVR::CPVRRecording* GetPVRRecordingInfoTag() const
  {
    return m_pvrRecordingInfoTag;
  }

  inline bool HasPVRTimerInfoTag() const
  {
    return m_pvrTimerInfoTag != NULL;
  }

  PVR::CPVRTimerInfoTag* GetPVRTimerInfoTag();

  inline const PVR::CPVRTimerInfoTag* GetPVRTimerInfoTag() const
  {
    return m_pvrTimerInfoTag;
  }

  inline bool HasPictureInfoTag() const
  {
    return m_pictureInfoTag != NULL;
  }

  inline const CPictureInfoTag* GetPictureInfoTag() const
  {
    return m_pictureInfoTag;
  }

  CPictureInfoTag* GetPictureInfoTag();

  // Gets the cached thumb filename (no existence checks)
  CStdString GetCachedVideoThumb() const;
  CStdString GetCachedEpisodeThumb() const;
  CStdString GetCachedArtistThumb() const;
  CStdString GetCachedSeasonThumb() const;
  CStdString GetCachedActorThumb() const;
  /*!
   \brief Get the cached fanart path for this item if it exists
   \return path to the cached fanart for this item, or empty if none exists
   \sa CacheLocalFanart, GetLocalFanart
   */
  CStdString GetCachedFanart() const;
  static CStdString GetCachedThumb(const CStdString &path, const CStdString& strPath2, bool split=false);

  // Sets the video thumb (cached first, else caches user thumb)
  void SetVideoThumb();
  /*!
   \brief Cache a copy of the local fanart for this item if we don't already have an image cached
   \return true if we already have cached fanart or if the caching was successful, false if no image is cached.
   \sa GetLocalFanart, GetCachedFanart
   */
  bool CacheLocalFanart() const;
  /*!
   \brief Get the local fanart for this item if it exists
   \return path to the local fanart for this item, or empty if none exists
   \sa CacheLocalFanart, GetCachedFanart
   */
  CStdString GetLocalFanart() const;

  // Sets the cached thumb for the item if it exists
  void SetCachedVideoThumb();
  void SetCachedArtistThumb();
  void SetCachedMusicThumb();
  void SetCachedSeasonThumb();

  // Gets the .tbn file associated with this item
  CStdString GetTBNFile() const;
  // Gets the folder image associated with this item (defaults to folder.jpg)
  CStdString GetFolderThumb(const CStdString &folderJPG = "folder.jpg") const;
  // Gets the correct movie title
  CStdString GetMovieName(bool bUseFolderNames = false) const;

  /*! \brief Find the base movie path (eg the folder if using "use foldernames for lookups")
   Takes care of VIDEO_TS, BDMV, and rar:// listings
   \param useFolderNames whether we're using foldernames for lookups
   \return the base movie folder
   */
  CStdString GetBaseMoviePath(bool useFolderNames) const;

#ifdef UNIT_TESTING
  static bool testGetBaseMoviePath();
#endif

  // Gets the user thumb, if it exists
  CStdString GetUserVideoThumb() const;
  CStdString GetUserMusicThumb(bool alwaysCheckRemote = false) const;

  // Caches the user thumb and assigns it to the item
  void SetUserVideoThumb();
  void SetUserMusicThumb(bool alwaysCheckRemote = false);

  /*! \brief Get the path where we expect local metadata to reside.
   For a folder, this is just the existing path (eg tvshow folder)
   For a file, this is the parent path, with exceptions made for VIDEO_TS and BDMV files

   Three cases are handled:

     /foo/bar/movie_name/file_name          -> /foo/bar/movie_name/
     /foo/bar/movie_name/VIDEO_TS/file_name -> /foo/bar/movie_name/
     /foo/bar/movie_name/BDMV/file_name     -> /foo/bar/movie_name/

     \sa URIUtils::GetParentPath
   */
  CStdString GetLocalMetadataPath() const;

  // finds a matching local trailer file
  CStdString FindTrailer() const;

  virtual bool LoadMusicTag();

  /* returns the content type of this item if known. will lookup for http streams */
  const CStdString& GetMimeType(bool lookup = true) const;

  /* sets the mime-type if known beforehand */
  void SetMimeType(const CStdString& mimetype) { m_mimetype = mimetype; } ;

  /* general extra info about the contents of the item, not for display */
  void SetExtraInfo(const CStdString& info) { m_extrainfo = info; };
  const CStdString& GetExtraInfo() const { return m_extrainfo; };

  /*! \brief Update an item with information from another item
   We take metadata information from the given item and supplement the current item
   with that info.  If tags exist in the new item we use the entire tag information.
   Properties are appended, and labels, thumbnail and icon are updated if non-empty
   in the given item.
   \param item the item used to supplement information
   */
  void UpdateInfo(const CFileItem &item);

  bool IsSamePath(const CFileItem *item) const;

  bool IsAlbum() const;
private:
  // Gets the previously cached thumb file (with existence checks)
  CStdString GetPreviouslyCachedMusicThumb() const;

public:
  bool m_bIsShareOrDrive;    ///< is this a root share/drive
  int m_iDriveType;     ///< If \e m_bIsShareOrDrive is \e true, use to get the share type. Types see: CMediaSource::m_iDriveType
  CDateTime m_dateTime;             ///< file creation date & time
  int64_t m_dwSize;             ///< file size (0 for folders)
  CStdString m_strDVDLabel;
  CStdString m_strTitle;
  int m_iprogramCount;
  int m_idepth;
  int m_lStartOffset;
  int m_lEndOffset;
  LockType m_iLockMode;
  CStdString m_strLockCode;
  int m_iHasLock; // 0 - no lock 1 - lock, but unlocked 2 - locked
  int m_iBadPwdCount;

private:
  CStdString m_strPath;            ///< complete path to item

  SPECIAL_SORT m_specialSort;
  bool m_bIsParentFolder;
  bool m_bCanQueue;
  bool m_bLabelPreformated;
  CStdString m_mimetype;
  CStdString m_extrainfo;
  MUSIC_INFO::CMusicInfoTag* m_musicInfoTag;
  CVideoInfoTag* m_videoInfoTag;
  EPG::CEpgInfoTag* m_epgInfoTag;
  PVR::CPVRChannel* m_pvrChannelInfoTag;
  PVR::CPVRRecording* m_pvrRecordingInfoTag;
  PVR::CPVRTimerInfoTag * m_pvrTimerInfoTag;
  CPictureInfoTag* m_pictureInfoTag;
  bool m_bIsAlbum;
};

/*!
  \brief A shared pointer to CFileItem
  \sa CFileItem
  */
typedef boost::shared_ptr<CFileItem> CFileItemPtr;

/*!
  \brief A vector of pointer to CFileItem
  \sa CFileItem
  */
typedef std::vector< CFileItemPtr > VECFILEITEMS;

/*!
  \brief Iterator for VECFILEITEMS
  \sa CFileItemList
  */
typedef std::vector< CFileItemPtr >::iterator IVECFILEITEMS;

/*!
  \brief A map of pointers to CFileItem
  \sa CFileItem
  */
typedef std::map<CStdString, CFileItemPtr > MAPFILEITEMS;

/*!
  \brief Iterator for MAPFILEITEMS
  \sa MAPFILEITEMS
  */
typedef std::map<CStdString, CFileItemPtr >::iterator IMAPFILEITEMS;

/*!
  \brief Pair for MAPFILEITEMS
  \sa MAPFILEITEMS
  */
typedef std::pair<CStdString, CFileItemPtr > MAPFILEITEMSPAIR;

typedef bool (*FILEITEMLISTCOMPARISONFUNC) (const CFileItemPtr &pItem1, const CFileItemPtr &pItem2);
typedef void (*FILEITEMFILLFUNC) (CFileItemPtr &item);

/*!
  \brief Represents a list of files
  \sa CFileItemList, CFileItem
  */
class CFileItemList : public CFileItem
{
public:
  enum CACHE_TYPE { CACHE_NEVER = 0, CACHE_IF_SLOW, CACHE_ALWAYS };

  CFileItemList();
  CFileItemList(const CStdString& strPath);
  virtual ~CFileItemList();
  virtual void Archive(CArchive& ar);
  CFileItemPtr operator[] (int iItem);
  const CFileItemPtr operator[] (int iItem) const;
  CFileItemPtr operator[] (const CStdString& strPath);
  const CFileItemPtr operator[] (const CStdString& strPath) const;
  void Clear();
  void ClearItems();
  void Add(const CFileItemPtr &pItem);
  void AddFront(const CFileItemPtr &pItem, int itemPosition);
  void Remove(CFileItem* pItem);
  void Remove(int iItem);
  CFileItemPtr Get(int iItem);
  const CFileItemPtr Get(int iItem) const;
  CFileItemPtr Get(const CStdString& strPath);
  const CFileItemPtr Get(const CStdString& strPath) const;
  int Size() const;
  bool IsEmpty() const;
  void Append(const CFileItemList& itemlist);
  void Assign(const CFileItemList& itemlist, bool append = false);
  bool Copy  (const CFileItemList& item);
  void Reserve(int iCount);
  void Sort(SORT_METHOD sortMethod, SORT_ORDER sortOrder);
  void Randomize();
  void SetMusicThumbs();
  void FillInDefaultIcons();
  int GetFolderCount() const;
  int GetFileCount() const;
  int GetSelectedCount() const;
  int GetObjectCount() const;
  void FilterCueItems();
  void RemoveExtensions();
  void SetFastLookup(bool fastLookup);
  bool Contains(const CStdString& fileName) const;
  bool GetFastLookup() const { return m_fastLookup; };

  /*! \brief stack a CFileItemList
   By default we stack all items (files and folders) in a CFileItemList
   \param stackFiles whether to stack all items or just collapse folders (defaults to true)
   \sa StackFiles,StackFolders
   */
  void Stack(bool stackFiles = true);

  SORT_ORDER GetSortOrder() const { return m_sortOrder; }
  SORT_METHOD GetSortMethod() const { return m_sortMethod; }
  /*! \brief load a CFileItemList out of the cache

   The file list may be cached based on which window we're viewing in, as different
   windows will be listing different portions of the same URL (eg viewing music files
   versus viewing video files)
   
   \param windowID id of the window that's loading this list (defaults to 0)
   \return true if we loaded from the cache, false otherwise.
   \sa Save,RemoveDiscCache
   */
  bool Load(int windowID = 0);

  /*! \brief save a CFileItemList to the cache
   
   The file list may be cached based on which window we're viewing in, as different
   windows will be listing different portions of the same URL (eg viewing music files
   versus viewing video files)
   
   \param windowID id of the window that's saving this list (defaults to 0)
   \return true if successful, false otherwise.
   \sa Load,RemoveDiscCache
   */
  bool Save(int windowID = 0);
  void SetCacheToDisc(CACHE_TYPE cacheToDisc) { m_cacheToDisc = cacheToDisc; }
  bool CacheToDiscAlways() const { return m_cacheToDisc == CACHE_ALWAYS; }
  bool CacheToDiscIfSlow() const { return m_cacheToDisc == CACHE_IF_SLOW; }
  /*! \brief remove a previously cached CFileItemList from the cache
   
   The file list may be cached based on which window we're viewing in, as different
   windows will be listing different portions of the same URL (eg viewing music files
   versus viewing video files)
   
   \param windowID id of the window whose cache we which to remove (defaults to 0)
   \sa Save,Load
   */
  void RemoveDiscCache(int windowID = 0) const;
  bool AlwaysCache() const;

  void SetCachedVideoThumbs();
  void SetCachedMusicThumbs();

  void Swap(unsigned int item1, unsigned int item2);

  /*! \brief Update an item in the item list
   \param item the new item, which we match based on path to an existing item in the list
   \return true if the item exists in the list (and was thus updated), false otherwise.
   */
  bool UpdateItem(const CFileItem *item);

  void AddSortMethod(SORT_METHOD method, int buttonLabel, const LABEL_MASKS &labelMasks);
  bool HasSortDetails() const { return m_sortDetails.size() != 0; };
  const std::vector<SORT_METHOD_DETAILS> &GetSortDetails() const { return m_sortDetails; };

  /*! \brief Specify whether this list should be sorted with folders separate from files
   By default we sort with folders listed (and sorted separately) except for those sort modes
   which should be explicitly sorted with folders interleaved with files (eg SORT_METHOD_FILES).
   With this set the folder state will be ignored, allowing folders and files to sort interleaved.
   \param sort whether to ignore the folder state.
   */
  void SetSortIgnoreFolders(bool sort) { m_sortIgnoreFolders = sort; };
  bool GetReplaceListing() const { return m_replaceListing; };
  void SetReplaceListing(bool replace);
  void SetContent(const CStdString &content) { m_content = content; };
  const CStdString &GetContent() const { return m_content; };

  void ClearSortState();
private:
  void Sort(FILEITEMLISTCOMPARISONFUNC func);
  void FillSortFields(FILEITEMFILLFUNC func);
  CStdString GetDiscCacheFile(int windowID) const;

  /*!
   \brief stack files in a CFileItemList
   \sa Stack
   */
  void StackFiles();

  /*!
   \brief stack folders in a CFileItemList
   \sa Stack
   */
  void StackFolders();

  VECFILEITEMS m_items;
  MAPFILEITEMS m_map;
  bool m_fastLookup;
  SORT_METHOD m_sortMethod;
  SORT_ORDER m_sortOrder;
  bool m_sortIgnoreFolders;
  CACHE_TYPE m_cacheToDisc;
  bool m_replaceListing;
  CStdString m_content;

  std::vector<SORT_METHOD_DETAILS> m_sortDetails;

  CCriticalSection m_lock;
};
