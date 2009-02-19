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

#include "GUIListItem.h"
#include "utils/Archive.h"
#include "DateTime.h"
#include "SortFileItem.h"
#include "utils/LabelFormatter.h"
#include "GUIPassword.h"
#include "utils/CriticalSection.h"

#include <vector>
#include "boost/shared_ptr.hpp"

namespace MUSIC_INFO
{
  class CMusicInfoTag;
}
class CVideoInfoTag;
class CTVEPGInfoTag;
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
      public CGUIListItem, public ISerializable
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
  CFileItem(const CTVEPGInfoTag& programme);
  CFileItem(const CMediaSource& share);
  virtual ~CFileItem(void);

  void Reset();
  const CFileItem& operator=(const CFileItem& item);
  virtual void Serialize(CArchive& ar);
  virtual bool IsFileItem() const { return true; };

  bool IsVideo() const;
  bool IsPicture() const;
  bool IsLyrics() const;
  bool IsAudio() const;
  bool IsCUESheet() const;
  bool IsShoutCast() const;
  bool IsLastFM() const;
  bool IsInternetStream() const;
  bool IsPlayList() const;
  bool IsSmartPlayList() const;
  bool IsPythonScript() const;
  bool IsXBE() const;
  bool IsPlugin() const;
  bool IsDefaultXBE() const;
  bool IsShortCut() const;
  bool IsNFO() const;
  bool IsDVDImage() const;
  bool IsDVDFile(bool bVobs = true, bool bIfos = true) const;
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
  bool IsRemote() const;
  bool IsSmb() const;
  bool IsDAAP() const;
  bool IsStack() const;
  bool IsMultiPath() const;
  bool IsMusicDb() const;
  bool IsVideoDb() const;
  bool IsTVDb() const;
  bool IsType(const char *ext) const;
  bool IsVirtualDirectoryRoot() const;
  bool IsReadOnly() const;
  bool CanQueue() const;
  void SetCanQueue(bool bYesNo);
  bool IsParentFolder() const;
  bool IsFileFolder() const;
  bool IsMemoryUnit() const;
  bool IsRemovable() const;
  bool IsTuxBox() const;
  bool IsMythTV() const;
  bool IsVTP() const;
  bool IsTV() const;

  void RemoveExtension();
  void CleanString();
  void FillInDefaultIcon();
  void SetMusicThumb(bool alwaysCheckRemote = false);
  void SetFileSizeLabel();
  virtual void SetLabel(const CStdString &strLabel);
  CURL GetAsUrl() const;
  bool IsLabelPreformated() const { return m_bLabelPreformated; }
  void SetLabelPreformated(bool bYesNo) { m_bLabelPreformated=bYesNo; }

  bool HasMusicInfoTag() const
  {
    return m_musicInfoTag != NULL;
  }

  MUSIC_INFO::CMusicInfoTag* GetMusicInfoTag();

  inline const MUSIC_INFO::CMusicInfoTag* GetMusicInfoTag() const
  {
    return m_musicInfoTag;
  }

  bool HasVideoInfoTag() const
  {
    return m_videoInfoTag != NULL;
  }
  
  CVideoInfoTag* GetVideoInfoTag();
  
  inline const CVideoInfoTag* GetVideoInfoTag() const
  {
    return m_videoInfoTag;
  }

  bool HasEPGInfoTag() const
  {
    return m_epgInfoTag != NULL;
  }

  CTVEPGInfoTag* GetEPGInfoTag();

  inline const CTVEPGInfoTag* GetEPGInfoTag() const
  {
    return m_epgInfoTag;
  }

  bool HasPictureInfoTag() const
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
  CStdString GetCachedPictureThumb() const;
  CStdString GetCachedArtistThumb() const;
  CStdString GetCachedProgramThumb() const;
  CStdString GetCachedGameSaveThumb() const;
  CStdString GetCachedProfileThumb() const;
  CStdString GetCachedSeasonThumb() const;
  CStdString GetCachedActorThumb() const;
  CStdString GetCachedFanart() const;
  static CStdString GetCachedThumb(const CStdString &path, const CStdString& strPath2, bool split=false);

  // Sets the video thumb (cached first, else caches user thumb)
  void SetVideoThumb();
  CStdString CacheFanart(bool probe=false) const;

  // Sets the cached thumb for the item if it exists
  void SetCachedVideoThumb();
  void SetCachedPictureThumb();
  void SetCachedArtistThumb();
  void SetCachedProgramThumb();
  void SetCachedGameSavesThumb();
  void SetCachedMusicThumb();
  void SetCachedSeasonThumb();

  // Gets the .tbn file associated with this item
  CStdString GetTBNFile() const;
  // Gets the folder image associated with this item (defaults to folder.jpg)
  CStdString GetFolderThumb(const CStdString &folderJPG = "folder.jpg") const;

  // Gets the user thumb, if it exists
  CStdString GetUserVideoThumb() const;
  CStdString GetUserMusicThumb(bool alwaysCheckRemote = false) const;

  // Caches the user thumb and assigns it to the item
  void SetUserVideoThumb();
  void SetUserProgramThumb();
  void SetUserMusicThumb(bool alwaysCheckRemote = false);

  // finds a matching local trailer file
  CStdString FindTrailer() const;

  virtual bool LoadMusicTag();

  /* returns the content type of this item if known. will lookup for http streams */  
  const CStdString& GetContentType() const; 

  /* sets the contenttype if known beforehand */
  void              SetContentType(const CStdString& content) { m_contenttype = content; } ;  

  /* general extra info about the contents of the item, not for display */
  void SetExtraInfo(const CStdString& info) { m_extrainfo = info; };
  const CStdString& GetExtraInfo() const { return m_extrainfo; };

  bool IsSamePath(const CFileItem *item) const;
private:
  // Gets the previously cached thumb file (with existence checks)
  CStdString GetPreviouslyCachedMusicThumb() const;

public:
  CStdString m_strPath;            ///< complete path to item
  bool m_bIsShareOrDrive;    ///< is this a root share/drive
  int m_iDriveType;     ///< If \e m_bIsShareOrDrive is \e true, use to get the share type. Types see: CMediaSource::m_iDriveType
  CDateTime m_dateTime;             ///< file creation date & time
  __int64 m_dwSize;             ///< file size (0 for folders)
  CStdString m_strDVDLabel;
  CStdString m_strTitle;
  int m_iprogramCount;
  int m_idepth;
  long m_lStartOffset;
  long m_lEndOffset;
  LockType m_iLockMode;
  CStdString m_strLockCode;
  int m_iHasLock; // 0 - no lock 1 - lock, but unlocked 2 - locked
  int m_iBadPwdCount;
private:

  bool m_bIsParentFolder;
  bool m_bCanQueue;
  bool m_bLabelPreformated;
  CStdString m_contenttype;
  CStdString m_extrainfo;
  MUSIC_INFO::CMusicInfoTag* m_musicInfoTag;
  CVideoInfoTag* m_videoInfoTag;
  CTVEPGInfoTag* m_epgInfoTag;
  CPictureInfoTag* m_pictureInfoTag;
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
  virtual void Serialize(CArchive& ar);
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
  void Stack();
  SORT_ORDER GetSortOrder() const { return m_sortOrder; }
  SORT_METHOD GetSortMethod() const { return m_sortMethod; }
  bool Load();
  bool Save();
  void SetCacheToDisc(CACHE_TYPE cacheToDisc) { m_cacheToDisc = cacheToDisc; }
  bool CacheToDiscAlways() const { return m_cacheToDisc == CACHE_ALWAYS; }
  bool CacheToDiscIfSlow() const { return m_cacheToDisc == CACHE_IF_SLOW; }
  void RemoveDiscCache() const;
  bool AlwaysCache() const;

  void SetCachedVideoThumbs();
  void SetCachedProgramThumbs();
  void SetCachedGameSavesThumbs();
  void SetCachedMusicThumbs();
  void SetProgramThumbs();
  void SetGameSavesThumbs();

  void Swap(unsigned int item1, unsigned int item2);

  void UpdateItem(const CFileItem *item);

  void AddSortMethod(SORT_METHOD method, int buttonLabel, const LABEL_MASKS &labelMasks);
  bool HasSortDetails() const { return m_sortDetails.size() != 0; };
  const std::vector<SORT_METHOD_DETAILS> &GetSortDetails() const { return m_sortDetails; };
  bool GetReplaceListing() const { return m_replaceListing; };
  void SetReplaceListing(bool replace);
  void SetContent(const CStdString &content) { m_content = content; };
  const CStdString &GetContent() const { return m_content; };

  void ClearSortState();
private:
  void Sort(FILEITEMLISTCOMPARISONFUNC func);
  void FillSortFields(FILEITEMFILLFUNC func);
  CStdString GetDiscCacheFile() const;

  VECFILEITEMS m_items;
  MAPFILEITEMS m_map;
  bool m_fastLookup;
  SORT_METHOD m_sortMethod;
  SORT_ORDER m_sortOrder;
  CACHE_TYPE m_cacheToDisc;
  bool m_replaceListing;
  CStdString m_content;

  std::vector<SORT_METHOD_DETAILS> m_sortDetails;

  CCriticalSection m_lock;
};
