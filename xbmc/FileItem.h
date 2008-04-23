/*!
 \file FileItem.h
 \brief
 */
#pragma once
#include "GUIListItem.h"
#include "utils/Archive.h"
#include "DateTime.h"
#include "SortFileItem.h"
#include "utils/LabelFormatter.h"
#include "GUIPassword.h"
#include "utils/CriticalSection.h"

#include <vector>

namespace MUSIC_INFO
{
  class CMusicInfoTag;
}
class CVideoInfoTag;
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
  CFileItem(const CMediaSource& share);
  virtual ~CFileItem(void);

  void Reset();
  const CFileItem& operator=(const CFileItem& item);
  virtual void Serialize(CArchive& ar);
  virtual bool IsFileItem() const { return true; };

  bool IsVideo() const;
  bool IsPicture() const;
  bool IsAudio() const;
  bool IsCUESheet() const;
  bool IsShoutCast() const;
  bool IsLastFM() const;
  bool IsInternetStream() const;
  bool IsPlayList() const;
  bool IsSmartPlayList() const;
  bool IsPythonScript() const;
  bool IsXBE() const;
  bool IsPluginFolder() const;
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

  void RemoveExtension();
  void CleanFileName();
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
  CStdString GetCachedPictureThumb() const;
  CStdString GetCachedArtistThumb() const;
  CStdString GetCachedProgramThumb() const;
  CStdString GetCachedGameSaveThumb() const;
  CStdString GetCachedProfileThumb() const;
  CStdString GetCachedSeasonThumb() const;
  CStdString GetCachedActorThumb() const;
  CStdString GetCachedVideoFanart() const;
  static CStdString GetCachedVideoFanart(const CStdString &path);

  // Sets the video thumb (cached first, else caches user thumb)
  void SetVideoThumb();
  void CacheVideoFanart() const;

  // Sets the cached thumb for the item if it exists
  void SetCachedVideoThumb();
  void SetCachedPictureThumb();
  void SetCachedArtistThumb();
  void SetCachedProgramThumb();
  void SetCachedGameSavesThumb();
  void SetCachedMusicThumb();
  void SetCachedSeasonThumb();

  // Gets the user thumb, if it exists
  CStdString GetUserVideoThumb() const;
  CStdString GetUserMusicThumb(bool alwaysCheckRemote = false) const;

  // Caches the user thumb and assigns it to the item
  void SetUserVideoThumb();
  void SetUserProgramThumb();
  void SetUserMusicThumb(bool alwaysCheckRemote = false);

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
  // Gets the .tbn file associated with this item
  CStdString GetTBNFile() const;
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
  CStdString GetFolderThumb(const CStdString &folderJPG = "folder.jpg") const;

  bool m_bIsParentFolder;
  bool m_bCanQueue;
  bool m_bLabelPreformated;
  CStdString m_contenttype;
  CStdString m_extrainfo;
  MUSIC_INFO::CMusicInfoTag* m_musicInfoTag;
  CVideoInfoTag* m_videoInfoTag;
  CPictureInfoTag* m_pictureInfoTag;
};

/*!
  \brief A vector of pointer to CFileItem
  \sa CFileItem
  */
typedef std::vector<CFileItem*> VECFILEITEMS;

/*!
  \brief Iterator for VECFILEITEMS
  \sa CFileItemList
  */
typedef std::vector<CFileItem*>::iterator IVECFILEITEMS;

/*!
  \brief A map of pointers to CFileItem
  \sa CFileItem
  */
typedef std::map<CStdString, CFileItem*> MAPFILEITEMS;

/*!
  \brief Iterator for MAPFILEITEMS
  \sa MAPFILEITEMS
  */
typedef std::map<CStdString, CFileItem*>::iterator IMAPFILEITEMS;

/*!
  \brief Pair for MAPFILEITEMS
  \sa MAPFILEITEMS
  */
typedef std::pair<CStdString, CFileItem*> MAPFILEITEMSPAIR;

typedef bool (*FILEITEMLISTCOMPARISONFUNC) (CFileItem* pItem1, CFileItem* pItem2);
/*!
  \brief Represents a list of files
  \sa CFileItemList, CFileItem
  */
class CFileItemList : public CFileItem
{
public:
  CFileItemList();
  CFileItemList(const CStdString& strPath);
  virtual ~CFileItemList();
  virtual void Serialize(CArchive& ar);
  CFileItem* operator[] (int iItem);
  const CFileItem* operator[] (int iItem) const;
  CFileItem* operator[] (const CStdString& strPath);
  const CFileItem* operator[] (const CStdString& strPath) const;
  void Clear();
  void ClearKeepPointer();
  void Add(CFileItem* pItem);
  void AddFront(CFileItem* pItem, int itemPosition);
  void Remove(CFileItem* pItem);
  void Remove(int iItem);
  CFileItem* Get(int iItem);
  const CFileItem* Get(int iItem) const;
  CFileItem* Get(const CStdString& strPath);
  const CFileItem* Get(const CStdString& strPath) const;
  int Size() const;
  bool IsEmpty() const;
  void Append(const CFileItemList& itemlist);
  void AppendPointer(const CFileItemList& itemlist);
  void AssignPointer(const CFileItemList& itemlist, bool append = false);
  void Reserve(int iCount);
  void Sort(SORT_METHOD sortMethod, SORT_ORDER sortOrder);
  void Randomize();
  void SetMusicThumbs();
  void FillInDefaultIcons();
  int GetFolderCount() const;
  int GetFileCount() const;
  int GetSelectedCount() const;
  void FilterCueItems();
  void RemoveExtensions();
  void CleanFileNames();
  void SetFastLookup(bool fastLookup);
  bool Contains(const CStdString& fileName) const;
  bool GetFastLookup() const { return m_fastLookup; };
  void Stack();
  SORT_ORDER GetSortOrder() const { return m_sortOrder; }
  SORT_METHOD GetSortMethod() const { return m_sortMethod; }
  bool Load();
  bool Save();
  void SetCacheToDisc(bool bYesNo) { m_bCacheToDisc=bYesNo; }
  bool GetCacheToDisc() const { return m_bCacheToDisc; }
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
  CStdString GetDiscCacheFile() const;

  VECFILEITEMS m_items;
  MAPFILEITEMS m_map;
  bool m_fastLookup;
  SORT_METHOD m_sortMethod;
  SORT_ORDER m_sortOrder;
  bool m_bCacheToDisc;
  bool m_replaceListing;
  CStdString m_content;

  std::vector<SORT_METHOD_DETAILS> m_sortDetails;

  CCriticalSection m_lock;
};
