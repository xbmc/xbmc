#include "cores/paplayer/replaygain.h"

namespace MUSIC_INFO
{

#pragma once

class CVorbisTag
{
public:
  CVorbisTag(void);
  virtual ~CVorbisTag(void);
  virtual bool ReadTag(const CStdString& strFile)=0;
  const CReplayGain &GetReplayGain() { return m_replayGain; };
  void GetMusicInfoTag(CMusicInfoTag& tag) { tag=m_musicInfoTag; };

  int ParseTagEntry(CStdString& strTagEntry);
protected:
  CMusicInfoTag m_musicInfoTag;
  CReplayGain m_replayGain;

private:
  void SplitEntry(const CStdString& strTagEntry, CStdString& strTagType, CStdString& strTagValue);
};
};
