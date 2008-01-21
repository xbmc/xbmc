#pragma once

#include "ImusicInfoTagLoader.h"

using namespace MUSIC_INFO;

namespace MUSIC_INFO
{

class CMusicInfoTagLoaderMP4: public IMusicInfoTagLoader
{
public:
  CMusicInfoTagLoaderMP4(void);
  virtual ~CMusicInfoTagLoaderMP4();

  virtual bool Load(const CStdString& strFileName, CMusicInfoTag& tag);

private:
  unsigned int ReadUnsignedInt( const char* pData );
  void ParseTag( unsigned int metaKey, const char* pMetaData, int metaSize, CMusicInfoTag& tag);
  int GetILSTOffset( const char* pBuffer, int bufferSize );
  int ParseAtom( __int64 startOffset, __int64 stopOffset, CMusicInfoTag& tag );

  unsigned int m_thumbSize;
  BYTE *m_thumbData;
  bool m_isCompilation;

  XFILE::CFile m_file;
};
}
