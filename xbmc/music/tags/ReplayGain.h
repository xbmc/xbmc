/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>

#define REPLAY_GAIN_NO_PEAK -1.0f
#define REPLAY_GAIN_NO_GAIN -1000.0f

class ReplayGain
{
public:
  enum Type {
    NONE = 0,
    ALBUM,
    TRACK
  };
public:
  class Info
  {
  public:
    void SetGain(float aGain);
    void SetGain(const std::string& aStrGain);
    float Gain() const;
    void SetPeak(const std::string& aStrPeak);
    void SetPeak(float aPeak);
    float Peak() const;
    bool HasGain() const;
    bool HasPeak() const;
    bool Valid() const;
  private:
    float m_gain = REPLAY_GAIN_NO_GAIN;   // measured in milliBels
    float m_peak = REPLAY_GAIN_NO_PEAK;   // 1.0 == full digital scale
  };
  const Info& Get(Type aType) const;
  void Set(Type aType, const Info& aInfo);
  void ParseGain(Type aType, const std::string& aStrGain);
  void SetGain(Type aType, float aGain);
  void ParsePeak(Type aType, const std::string& aStrPeak);
  void SetPeak(Type aType, float aPeak);
  std::string Get() const;
  void Set(const std::string& strReplayGain);
private:
  Info m_data[TRACK];
};
