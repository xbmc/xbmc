/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ReplayGain.h"

#include "utils/StringUtils.h"

#include <stdlib.h>

static bool TypeIsValid(ReplayGain::Type aType)
{
  return (aType > ReplayGain::NONE && aType <= ReplayGain::TRACK);
}

static int index(ReplayGain::Type aType)
{
  return static_cast<int>(aType) - 1;
}

///////////////////////////////////////////////////////////////
// class ReplayGain
///////////////////////////////////////////////////////////////

const ReplayGain::Info& ReplayGain::Get(Type aType) const
{
  if (TypeIsValid(aType))
    return m_data[index(aType)];

  static Info invalid;
  return invalid;
}

void ReplayGain::Set(Type aType, const Info& aInfo)
{
  if (TypeIsValid(aType))
    m_data[index(aType)] = aInfo;
}

void ReplayGain::ParseGain(Type aType, const std::string& aStrGain)
{
  if (TypeIsValid(aType))
    m_data[index(aType)].SetGain(aStrGain);
}

void ReplayGain::SetGain(Type aType, float aGain)
{
  if (TypeIsValid(aType))
    m_data[index(aType)].SetGain(aGain);
}

void ReplayGain::ParsePeak(Type aType, const std::string& aStrPeak)
{
  if (TypeIsValid(aType))
    m_data[index(aType)].SetPeak(aStrPeak);
}

void ReplayGain::SetPeak(Type aType, float aPeak)
{
  if (TypeIsValid(aType))
    m_data[index(aType)].SetPeak(aPeak);
}

std::string ReplayGain::Get() const
{
  if (!Get(ALBUM).Valid() && !Get(TRACK).Valid())
    return std::string();

  std::string rg;
  if (Get(ALBUM).Valid())
    rg = StringUtils::Format("{:.3f},{:.3f},", Get(ALBUM).Gain(), Get(ALBUM).Peak());
  else
    rg = "-1000, -1,";
  if (Get(TRACK).Valid())
    rg += StringUtils::Format("{:.3f},{:.3f}", Get(TRACK).Gain(), Get(TRACK).Peak());
  else
    rg += "-1000, -1";
  return rg;
}

void ReplayGain::Set(const std::string& strReplayGain)
{
  std::vector<std::string> values = StringUtils::Split(strReplayGain, ",");
  if (values.size() == 4)
  {
    ParseGain(ALBUM, values[0]);
    ParsePeak(ALBUM, values[1]);
    ParseGain(TRACK, values[2]);
    ParsePeak(TRACK, values[3]);
  }
}

///////////////////////////////////////////////////////////////
// class ReplayGain::Info
///////////////////////////////////////////////////////////////

void ReplayGain::Info::SetGain(float aGain)
{
  m_gain = aGain;
}

void ReplayGain::Info::SetGain(const std::string& aStrGain)
{
  SetGain(static_cast<float>(atof(aStrGain.c_str())));
}

float ReplayGain::Info::Gain() const
{
  return m_gain;
}

void ReplayGain::Info::SetPeak(const std::string& aStrPeak)
{
  SetPeak(static_cast<float>(atof(aStrPeak.c_str())));
}

void ReplayGain::Info::SetPeak(float aPeak)
{
  m_peak = aPeak;
}

float ReplayGain::Info::Peak() const
{
  return m_peak;
}

bool ReplayGain::Info::HasGain() const
{
  return m_gain != REPLAY_GAIN_NO_GAIN;
}

bool ReplayGain::Info::HasPeak() const
{
  return m_peak != REPLAY_GAIN_NO_PEAK;
}

bool ReplayGain::Info::Valid() const
{
  return HasPeak() && HasGain();
}
