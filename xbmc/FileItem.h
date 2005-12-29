/*!
 \file FileItem.h
 \brief
 */
#pragma once
#include "..\guilib\guilistitem.h"
#include "song.h"
#include "utils/archive.h"

using namespace MUSIC_INFO;

typedef enum {
  SORT_METHOD_NONE=0,
  SORT_METHOD_LABEL,
  SORT_METHOD_LABEL_IGNORE_THE,
  SORT_METHOD_DATE,
  SORT_METHOD_SIZE,
  SORT_METHOD_FILE,
  SORT_METHOD_DRIVE_TYPE,
  SORT_METHOD_TRACKNUM,
  SORT_METHOD_DURATION,
  SORT_METHOD_TITLE,
  SORT_METHOD_TITLE_IGNORE_THE,
  SORT_METHOD_ARTIST,
  SORT_METHOD_ARTIST_IGNORE_THE,
  SORT_METHOD_ALBUM,
  SORT_METHOD_ALBUM_IGNORE_THE,
  SORT_METHOD_GENRE,
  SORT_METHOD_VIDEO_YEAR,
  SORT_METHOD_VIDEO_RATING,
  SORT_METHOD_PROGRAM_COUNT,

  SORT_METHOD_MAX
} SORT_METHOD;

typedef enum {
  SORT_ORDER_NONE=0,
  SORT_ORDER_ASC,
  SORT_ORDER_DESC
} SORT_ORDER;

class CShare;

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
  CFileItem(const CStdString& strLabel);
  CFileItem(const CStdString& strPath, bool bIsFolder);
  CFileItem(const CSong& song);
  CFileItem(const CAlbum& album);
  CFileItem(const CArtist& artist);
  CFileItem(const CGenre& genre);
  CFileItem(const CShare& share);
  virtual ~CFileItem(void);

  void Reset();
  const CFileItem& operator=(const CFileItem& item);
  virtual void Serialize(CArchive& ar);

  bool IsVideo() const;
  bool IsPicture() const;
  bool IsAudio() const;
  bool IsCUESheet() const;
  bool IsShoutCast() const;
  bool IsLastFM() const;
  bool IsInternetStream() const;
  bool IsPlayList() const;
  bool IsPythonScript() const;
  bool IsXBE() const;
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
  bool IsHD() const;
  bool IsRemote() const;
  bool IsSmb() const;
  bool IsStack() const;
  bool IsMusicDb() const;
  bool IsType(const char *ext) const;
  bool IsVirtualDirectoryRoot() const;
  bool IsReadOnly() const;
  bool CanQueue() const;
  void SetCanQueue(bool bYesNo);
  bool IsParentFolder() const;

  void RemoveExtension();
  void CleanFileName();
  void FillInDefaultIcon();
  void SetThumb();
  void SetMusicThumb();
  void SetArtistThumb();
  void SetFileSizeLabel();
  virtual void SetLabel(const CStdString &strLabel);
  CURL GetAsUrl() const;

public:
  CStdString m_strPath;            ///< complete path to item
  bool m_bIsShareOrDrive;    ///< is this a root share/drive
  int m_iDriveType;     ///< If \e m_bIsShareOrDrive is \e true, use to get the share type. Types see: CShare::m_iDriveType
  SYSTEMTIME m_stTime;             ///< file creation date & time
  __int64 m_dwSize;             ///< file size (0 for folders)
  float m_fRating;
  CStdString m_strDVDLabel;
  CMusicInfoTag m_musicInfoTag;
  int m_iprogramCount;
  int m_idepth;
  long m_lStartOffset;
  long m_lEndOffset;
  int m_iLockMode;
  CStdString m_strLockCode;
  int m_iBadPwdCount;
private:
  bool m_bIsParentFolder;
  bool m_bCanQueue;
};

/*!
  \brief A vector of pointer to CFileItem
  \sa CFileItem
  */
typedef vector<CFileItem*> VECFILEITEMS;

/*!
  \brief Iterator for VECFILEITEMS
  \sa CFileItemList
  */
typedef vector<CFileItem*>::iterator IVECFILEITEMS;

/*!
  \brief A map of pointers to CFileItem
  \sa CFileItem
  */
typedef map<CStdString, CFileItem*> MAPFILEITEMS;

/*!
  \brief Iterator for MAPFILEITEMS
  \sa MAPFILEITEMS
  */
typedef map<CStdString, CFileItem*>::iterator IMAPFILEITEMS;

/*!
  \brief Pair for MAPFILEITEMS
  \sa MAPFILEITEMS
  */
typedef pair<CStdString, CFileItem*> MAPFILEITEMSPAIR;

typedef bool (*FILEITEMLISTCOMPARISONFUNC) (CFileItem* pItem1, CFileItem* pItem2);
/*!
  \brief Represents a list of files
  \sa CFileItemList, CFileItem
  */
class CFileItemList : public CFileItem
{
public:
  CFileItemList();
  virtual ~CFileItemList();
  virtual void Serialize(CArchive& ar);
  CFileItem* operator[] (int iItem);
  const CFileItem* operator[] (int iItem) const;
  void Clear();
  void ClearKeepPointer();
  void Add(CFileItem* pItem);
  void Remove(CFileItem* pItem);
  void Remove(int iItem);
  CFileItem* Get(int iItem);
  const CFileItem* Get(int iItem) const;
  int Size() const;
  bool IsEmpty() const;
  void Append(const CFileItemList& itemlist);
  void AppendPointer(const CFileItemList& itemlist);
  void Reserve(int iCount);
  void Sort(FILEITEMLISTCOMPARISONFUNC func);
  void Sort(SORT_METHOD sortMethod, SORT_ORDER sortOrder);
  void Randomize();
  void SetThumbs();
  void SetMusicThumbs();
  void FillInDefaultIcons();
  int GetFolderCount() const;
  int GetFileCount() const;
  void FilterCueItems();
  void RemoveExtensions();
  void CleanFileNames();
  bool HasFileNoCase(CStdString& path);
  void SetFastLookup(bool fastLookup);
  bool Contains(CStdString& fileName);
  bool GetFastLookup() { return m_fastLookup; };
  void Stack();
  SORT_ORDER GetSortOrder() { return m_sortOrder; }
  SORT_METHOD GetSortMethod() { return m_sortMethod; }
  bool Load();
  bool Save();
private:
  VECFILEITEMS m_items;
  MAPFILEITEMS m_map;
  bool m_fastLookup;
  SORT_METHOD m_sortMethod;
  SORT_ORDER m_sortOrder;
};
