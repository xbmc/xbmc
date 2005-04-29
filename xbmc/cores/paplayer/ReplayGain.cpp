#include "../../stdafx.h"
#include "ReplayGain.h"

CReplayGain::CReplayGain()
{
  iTrackGain = 0;
  iAlbumGain = 0;
  fTrackPeak = 0.0f;
  fAlbumPeak = 0.0f;
  iHasGainInfo = 0;
}

CReplayGain::~CReplayGain()
{
}
