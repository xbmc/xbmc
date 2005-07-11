/*!
 \file FileItem.h
 \brief
 */
#pragma once
#include "..\guilib\guilistitem.h"
#include "song.h"
#include "settings.h"
#include "utils/archive.h"

using namespace MUSIC_INFO;

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
  CFileItem(const CShare& share);
  virtual ~CFileItem(void);

  void Clear();
  const CFileItem& operator=(const CFileItem& item);
  virtual void Serialize(CArchive& ar);

  bool IsVideo() const;
  bool IsPicture() const;
  bool IsAudio() const;
  bool IsCUESheet() const;
  bool IsShoutCast() const;
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
  bool IsHD() const;
  bool IsRemote() const;
  bool IsSmb() const;
  bool IsType(const char *ext) const;
  bool IsVirtualDirectoryRoot() const;
  bool IsReadOnly() const;

  void RemoveExtension();
  void CleanFileName();
  void FillInDefaultIcon();
  void SetThumb();
  void SetMusicThumb();
  void SetArtistThumb();

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
class CFileItemList : public ISerializable
{
public:
  CFileItemList();
  virtual ~CFileItemList();
  virtual void Serialize(CArchive& ar);
  CFileItem* operator[] (int iItem);
  const CFileItem* operator[] (int iItem) const;
  void Clear();
  void ClearKeepPointers();
  void Add(CFileItem* pItem);
  void Remove(CFileItem* pItem);
  void Remove(int iItem);
  CFileItem* Get(int iItem);
  const CFileItem* Get(int iItem) const;
  int Size() const;
  bool IsEmpty() const;
  void Append(const CFileItemList& itemlist);
  void Reserve(int iCount);
  void Sort(FILEITEMLISTCOMPARISONFUNC func);
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
private:
  VECFILEITEMS m_items;
  MAPFILEITEMS m_map;
  bool m_fastLookup;
};
