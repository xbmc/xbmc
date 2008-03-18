#pragma once

#define REPLAY_GAIN_HAS_TRACK_INFO 1
#define REPLAY_GAIN_HAS_ALBUM_INFO 2
#define REPLAY_GAIN_HAS_TRACK_PEAK 4
#define REPLAY_GAIN_HAS_ALBUM_PEAK 8

class CReplayGain
{
public:
  CReplayGain();
  ~CReplayGain();

  int iTrackGain; // measured in milliBels
  int iAlbumGain;
  float fTrackPeak; // 1.0 == full digital scale
  float fAlbumPeak;
  int iHasGainInfo;   // valid info
};
