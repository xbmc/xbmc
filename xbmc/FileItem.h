/*!
	\file FileItem.h
	\brief
	*/
#pragma once
#include "guilistitem.h"
#include "stdstring.h"
#include "MusicInfoTag.h"
#include "MusicDatabase.h"
#include "PlayList.h"
#include <vector>
#include "utils/archive.h"
using namespace std;
using namespace MUSIC_INFO;
using namespace PLAYLIST;

/*!
	\brief Represents a file on a share
	\sa VECFILEITEMS, CFileItemList
	*/
class CFileItem :
	public CGUIListItem, public ISerializable
{
public:
  CFileItem(void);
  CFileItem(const CFileItem& item);
  CFileItem(const CStdString& strLabel);
	CFileItem(const CSong& song);
	CFileItem(const CAlbum& album);
	CFileItem(const CPlayList::CPlayListItem &item);
  virtual ~CFileItem(void);

	void Clear();
  const CFileItem& operator=(const CFileItem& item);
	virtual void Serialize(CArchive& ar);

  CStdString    m_strPath;						///< complete path to item
  bool          m_bIsShareOrDrive;		///< is this a root share/drive
  int			m_iDriveType;			///< If \e m_bIsShareOrDrive is \e true, use to get the share type. Types see: CShare::m_iDriveType
  SYSTEMTIME    m_stTime;							///< file creation date & time
  __int64       m_dwSize;							///< file size (0 for folders)
  float         m_fRating;
  CStdString    m_strDVDLabel;
	CMusicInfoTag m_musicInfoTag;
  int			m_iprogramCount;
	int			m_idepth;
	long		m_lStartOffset;
	long		m_lEndOffset;
  int           m_iLockMode;
  CStdStringW    m_strLockCode;
  int           m_iBadPwdCount;
};

/*!
	\brief A vector of pointer to CFileItem
	\sa CFileItem
	*/
typedef vector<CFileItem*> VECFILEITEMS;

/*!
	\brief Iterator for VECFILEITEMS
	\sa VECFILEITEMS
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

/*!
	\brief Represents a list of files
	\sa VECFILEITEMS, CFileItem
	*/
class CFileItemList : public ISerializable
{
public:
	CFileItemList(VECFILEITEMS& items);
	virtual ~CFileItemList();
	VECFILEITEMS& GetList() ;
	void Clear();
	virtual void Serialize(CArchive& ar);
private:
	VECFILEITEMS& m_items;
};