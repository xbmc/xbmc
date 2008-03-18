#include "Tag.h"

namespace MUSIC_INFO
{

#pragma once

class CVorbisTag : public CTag
{
public:
  CVorbisTag(void);
  virtual ~CVorbisTag(void);
  int ParseTagEntry(CStdString& strTagEntry);

private:
  void SplitEntry(const CStdString& strTagEntry, CStdString& strTagType, CStdString& strTagValue);
};
}
