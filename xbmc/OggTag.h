//------------------------------
// COggTag in 2003 by Bobbin007
//------------------------------
#include "VorbisTag.h"
#include "cores/paplayer/replaygain.h"
#include "cores/paplayer/dllvorbisfile.h"

namespace MUSIC_INFO
{

#pragma once

class COggTag : public CVorbisTag
{
public:
  COggTag(void);
  virtual ~COggTag(void);
  virtual bool Read(const CStdString& strFile);
          int  GetStreamCount(const CStdString& strFile);
protected:
  DllVorbisfile m_dll;
};
};
