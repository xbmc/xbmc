//------------------------------
// CFlacTag in 2003 by JMarshall
//------------------------------
#include "VorbisTag.h"

namespace MUSIC_INFO
{

#pragma once

class CFlacTag : public CVorbisTag
{
public:
  CFlacTag(void);
  virtual ~CFlacTag(void);
  virtual bool Read(const CStdString& strFile);

protected:
  XFILE::CFile* m_file;
  void ProcessVorbisComment(const char *pBuffer);
  int ReadFlacHeader(void);    // returns the position after the STREAM_INFO metadata
  int FindFlacHeader(void);    // returns the offset in the file of the fLaC data
  unsigned int ReadUnsigned();  // reads a 32 bit unsigned int
};
}
