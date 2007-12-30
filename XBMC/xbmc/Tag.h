#include "cores/paplayer/replaygain.h"

namespace MUSIC_INFO
{

#pragma once

class CTag
{
public:
  CTag(void) {}
  virtual ~CTag(void) {}
  virtual bool Read(const CStdString& strFile) { m_musicInfoTag.SetURL(strFile); return false; }
  virtual bool Write(const CStdString& strFile) { return false; }

  const CReplayGain &GetReplayGain() const { return m_replayGain; }
  void GetMusicInfoTag(CMusicInfoTag& tag) const { tag=m_musicInfoTag; }
  void SetMusicInfoTag(CMusicInfoTag& tag) { m_musicInfoTag=tag; }

protected:
  CMusicInfoTag m_musicInfoTag;
  CReplayGain m_replayGain;
};
}
