/*!
	\file FileItem.h
	\brief
	*/
#pragma once
#include "guilistitem.h"
#include "stdstring.h"
#include "MusicInfoTag.h"
#include "MusicDatabase.h"
#include <vector>
using namespace std;
using namespace MUSIC_INFO;


/*!
	\brief Represents a file on a share
	\sa VECFILEITEMS, CFileItemList
	*/
class CFileItem :
  public CGUIListItem
{
public:
  CFileItem(void);
  CFileItem(const CFileItem& item);
  CFileItem(const CStdString& strLabel);
	CFileItem(const CSong& song);
	CFileItem(const CAlbum& album);
  virtual ~CFileItem(void);

	
  const CFileItem& operator=(const CFileItem& item);
  CStdString    m_strPath;						///< complete path to item
  bool          m_bIsShareOrDrive;		///< is this a root share/drive
  int			m_iDriveType;			///< If \e m_bIsShareOrDrive is \e true, use to get the share type. Types see: CShare::m_iDriveType
  SYSTEMTIME    m_stTime;							///< file creation date & time
  __int64       m_dwSize;							///< file size (0 for folders)
  float         m_fRating;
  CStdString    m_strDVDLabel;
	CMusicInfoTag m_musicInfoTag;
  int			m_iprogramCount;
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
	\brief Represents a list of files
	\sa VECFILEITEMS, CFileItem
	*/
class CFileItemList
{
public:
	CFileItemList(VECFILEITEMS& items);
	virtual ~CFileItemList();
	VECFILEITEMS& GetList() ;
private:
	VECFILEITEMS& m_items;
};