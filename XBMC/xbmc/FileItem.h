#pragma once
#include "guilistitem.h"
#include "stdstring.h"
#include "MusicInfoTag.h"
#include <vector>
using namespace std;
using namespace MUSIC_INFO;

class CFileItem :
  public CGUIListItem
{
public:
  CFileItem(void);
  CFileItem(const CStdString& strLabel);
  virtual ~CFileItem(void);

  CStdString    m_strPath;						// complete path to item
  bool          m_bIsFolder;					// is item a folder or a file
  bool          m_bIsShareOrDrive;		// is this a root share/drive
  SYSTEMTIME    m_stTime;							// file creation date & time
  DWORD         m_dwSize;							// file size (0 for folders)
	CMusicInfoTag m_musicInfoTag;
};

typedef vector<CFileItem*> VECFILEITEMS;
typedef vector<CFileItem*>::iterator IVECFILEITEMS;