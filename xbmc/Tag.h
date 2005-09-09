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
  virtual bool Write() { return false; }

  const CReplayGain &GetReplayGain() { return m_replayGain; };
  void GetMusicInfoTag(CMusicInfoTag& tag) { tag=m_musicInfoTag; };
  virtual void SetMusicInfoTag(CMusicInfoTag& tag) {}

protected:
  CMusicInfoTag m_musicInfoTag;
  CReplayGain m_replayGain;
};
};
