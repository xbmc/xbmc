//------------------------------
// CFlacTag in 2003 by JMarshall
//------------------------------
#include "VOrbisTag.h"

namespace MUSIC_INFO
{

#pragma once

class CFlacTag : public CVorbisTag
{
public:
  CFlacTag(void);
  virtual ~CFlacTag(void);
  virtual bool ReadTag(const CStdString& strFile);

protected:
  CFile* m_file;
  void ProcessVorbisComment(const char *pBuffer);
  int ReadFlacHeader(void);    // returns the position after the STREAM_INFO metadata
  int FindFlacHeader(void);    // returns the offset in the file of the fLaC data

};
};
