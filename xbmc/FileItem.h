/*!
 \file FileItem.h
 \brief
 */
#pragma once

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

#include "guilib/GUIListItem.h"
#include "utils/Archive.h"
#include "utils/ISerializable.h"
#include "utils/ISortable.h"
#include "XBDateTime.h"
#include "utils/SortUtils.h"
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

enum EFileFolderType {
  EFILEFOLDER_TYPE_ALWAYS     = 1<<0,
  EFILEFOLDER_TYPE_ONCLICK    = 1<<1,
  EFILEFOLDER_TYPE_ONBROWSE   = 1<<2,

  EFILEFOLDER_MASK_ALL        = 0xff,
  EFILEFOLDER_MASK_ONCLICK    = EFILEFOLDER_TYPE_ALWAYS
                              | EFILEFOLDER_TYPE_ONCLICK,
  EFILEFOLDER_MASK_ONBROWSE   = EFILEFOLDER_TYPE_ALWAYS
                              | EFILEFOLDER_TYPE_ONCLICK
                              | EFILEFOLDER_TYPE_ONBROWSE,
};

/*!
  \brief Represents a file on a share
  \sa CFileItemList
  */
class CFileItem :
  public CGUIListItem, public IArchivable, public ISerializable, public ISortable
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
  CFileItem(const MUSIC_INFO::CMusicInfoTag& music);
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
  virtual void Serialize(CVariant& value) const;
  virtual void ToSortable(SortItem &sortable);
  virtual bool IsFileItem() const { return true; };

  bool Exists(bool bUseCache = true) const;
  
  /*!
   \brief Check whether an item is a video item. Note that this returns true for
    anything with a video info tag, so that may include eg. folders.
   \return true if item is video, false otherwise. 
   */
  bool IsVideo() const;

  bool IsDiscStub() const;

  /*!
   \brief Check whether an item is a picture item. Note that this returns true for
    anything with a picture info tag, so that may include eg. folders.
   \return true if item is picture, false otherwise. 
   */
  bool IsPicture() const;
  bool IsLyrics() const;

  /*!
   \brief Check whether an item is an audio item. Note that this returns true for
    anything with a music info tag, so that may include eg. folders.
   \return true if item is audio, false otherwise. 
   */
  bool IsAudio() const;

  bool IsKaraoke() const;
  bool IsCUESheet() const;
  bool IsInternetStream(const bool bStrictCheck = false) const;
  bool IsPlayList() const;
  bool IsSmartPlayList() const;
  bool IsPythonScript() const;
  bool IsPlugin() const;
  bool IsScript() const;
  bool IsAddonsPath() const;
  bool IsSourcesPath() const;
  bool IsNFO() const;
  bool IsDVDImage() const;
  bool IsOpticalMediaFile() const;
  bool IsDVDFile(bool bVobs = true, bool bIfos = true) const;
  bool IsBDFile() const;
  bool IsRAR() const;
  bool IsAPK() const;
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
  bool IsFileFolder(EFileFolderType types = EFILEFOLDER_MASK_ALL) const;
  bool IsRemovable() const;
  bool IsTuxBox() const;
  bool IsMythTV() const;
  bool IsHDHomeRun() const;
  bool IsSlingbox() const;
  bool IsVTP() const;
  bool IsPVR() const;
  bool IsLiveTV() const;
  bool IsRSS() const;
  bool IsAndroidApp() const;

  void RemoveExtension();
  void CleanString();
  void FillInDefaultIcon();
  void SetFileSizeLabel();
  virtual void SetLabel(const CStdString &strLabel);
  CURL GetAsUrl() const;
  int GetVideoContentType() const; /* return VIDEODB_CONTENT_TYPE, but don't want to include videodb in this header */
  bool IsLabelPreformated() const { return m_bLabelPreformated; }
  void SetLabelPreformated(bool bYesNo) { m_bLabelPreformated=bYesNo; }
  bool SortsOnTop() const { return m_specialSort == SortSpecialOnTop; }
  bool SortsOnBottom() const { return m_specialSort == SortSpecialOnBottom; }
  void SetSpecialSort(SortSpecial sort) { m_specialSort = sort; }

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

  /*!
   \brief Get the local fanart for this item if it exists
   \return path to the local fanart for this item, or empty if none exists
   \sa GetFolderThumb, GetTBNFile
   */
  CStdString GetLocalFanart() const;

  /*! \brief Assemble the filename of a particular piece of local artwork for an item.
             No file existence check is typically performed.
   \param artFile the art file to search for.
   \param useFolder whether to look in the folder for the art file. Defaults to false.
   \return the path to the local artwork.
   \sa FindLocalArt
   */
  CStdString GetLocalArt(const std::string &artFile, bool useFolder = false) const;

  /*! \brief Assemble the filename of a particular piece of local artwork for an item,
             and check for file existence.
   \param artFile the art file to search for.
   \param useFolder whether to look in the folder for the art file. Defaults to false.
   \return the path to the local artwork if it exists, empty otherwise.
   \sa GetLocalArt
   */
  CStdString FindLocalArt(const std::string &artFile, bool useFolder) const;

  // Gets the .tbn file associated with this item
  CStdString GetTBNFile() const;
  // Gets the folder image associated with this item (defaults to folder.jpg)
  CStdString GetFolderThumb(const CStdString &folderJPG = "folder.jpg") const;
  // Gets the correct movie title
  CStdString GetMovieName(bool bUseFolderNames = false) const;

  /*! \brief Find the base movie path (i.e. the item the user expects us to use to lookup the movie)
   For folder items, with "use foldernames for lookups" it returns the folder.
   Regardless of settings, for VIDEO_TS/BDMV it returns the parent of the VIDEO_TS/BDMV folder (if present)

   \param useFolderNames whether we're using foldernames for lookups
   \return the base movie folder
   */
  CStdString GetBaseMoviePath(bool useFolderNames) const;

  // Gets the user thumb, if it exists
  CStdString GetUserMusicThumb(bool alwaysCheckRemote = false, bool fallbackToFolder = false) const;

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
   \param replaceLabels whether to replace labels (defaults to true)
   */
  void UpdateInfo(const CFileItem &item, bool replaceLabels = true);

  bool IsSamePath(const CFileItem *item) const;

  bool IsAlbum() const;

  /*! \brief Sets details using the information from the CVideoInfoTag object
   Sets the videoinfotag and uses its information to set the label and path.
   \param video video details to use and set
   */
  void SetFromVideoInfoTag(const CVideoInfoTag &video);
  /*! \brief Sets details using the information from the CAlbum object
   Sets the album in the music info tag and uses its information to set the
   label and album-specific properties.
   \param album album details to use and set
   */
  void SetFromAlbum(const CAlbum &album);
  /*! \brief Sets details using the information from the CSong object
   Sets the song in the music info tag and uses its information to set the
   label, path, song-specific properties and artwork.
   \param song song details to use and set
   */
  void SetFromSong(const CSong &song);

  bool m_bIsShareOrDrive;    ///< is this a root share/drive
  int m_iDriveType;     ///< If \e m_bIsShareOrDrive is \e true, use to get the share type. Types see: CMediaSource::m_iDriveType
  CDateTime m_dateTime;             ///< file creation date & time
  int64_t m_dwSize;             ///< file size (0 for folders)
  CStdString m_strDVDLabel;
  CStdString m_strTitle;
  int m_iprogramCount;
  int m_idepth;
  int m_lStartOffset;
  int m_lStartPartNumber;
  int m_lEndOffset;
  LockType m_iLockMode;
  CStdString m_strLockCode;
  int m_iHasLock; // 0 - no lock 1 - lock, but unlocked 2 - locked
  int m_iBadPwdCount;

private:
  CStdString m_strPath;            ///< complete path to item

  SortSpecial m_specialSort;
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
  CFileItemList(const CFileItemList& rhs);
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
  void RemoveRange(int iRangeBegin, int iRangeEnd);
  CFileItemPtr Get(int iItem);
  const CFileItemPtr Get(int iItem) const;
  const VECFILEITEMS GetList() const { return m_items; }
  CFileItemPtr Get(const CStdString& strPath);
  const CFileItemPtr Get(const CStdString& strPath) const;
  int Size() const;
  bool IsEmpty() const;
  void Append(const CFileItemList& itemlist);
  void Assign(const CFileItemList& itemlist, bool append = false);
  bool Copy  (const CFileItemList& item, bool copyItems = true);
  void Reserve(int iCount);
  void Sort(SORT_METHOD sortMethod, SortOrder sortOrder);
  /* \brief Sorts the items based on the given sorting options

  In contrast to Sort (see above) this does not change the internal
  state by storing the sorting method and order used and therefore
  will always execute the sorting even if the list of items has
  already been sorted with the same options before.
  */
  void Sort(SortDescription sortDescription);
  void Randomize();
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

  SortOrder GetSortOrder() const { return m_sortOrder; }
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

    //void Move(int position, int move);
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
  CStdString GetDiscFileCache(int windowID) const;

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
  SortOrder m_sortOrder;
  bool m_sortIgnoreFolders;
  CACHE_TYPE m_cacheToDisc;
  bool m_replaceListing;
  CStdString m_content;

  std::vector<SORT_METHOD_DETAILS> m_sortDetails;

  CCriticalSection m_lock;
};
