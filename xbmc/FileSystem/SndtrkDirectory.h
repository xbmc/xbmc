#pragma once

#include "IDirectory.h"

class CSoundtrack
{
public:

  //VOID    GetSoundtrackName( WCHAR* strName ) { wcscpy( strName, m_strName ); }
  //UINT    GetSongCount() { return m_uSongCount; }

  WCHAR strName[42];
  UINT uSongCount;
  UINT uSoundtrackId;
};

typedef map<UINT, CSoundtrack> SOUNDTRACK;
typedef map<UINT, CSoundtrack>::iterator ISOUNDTRACK;
typedef pair<UINT, CSoundtrack> SOUNDTRACK_PAIR;

namespace DIRECTORY
{

class CSndtrkDirectory :
      public IDirectory
{
public:
  CSndtrkDirectory(void);
  virtual ~CSndtrkDirectory(void);
  virtual bool GetDirectory(const CStdString& strPath, CFileItemList &items);
  bool IsAlone(const CStdString& strPath);
  bool FindTrackName(const CStdString& strPath, char* NameOfSong);
};
};
