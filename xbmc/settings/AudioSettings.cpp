/*
 *      Copyright (C) 2005-2011 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "AudioSettings.h"
#include "utils/XMLUtils.h"

CAudioSettings::CAudioSettings()
{
  Initialise();
}

CAudioSettings::CAudioSettings(TiXmlElement *pRootElement)
{
  Initialise();
  TiXmlElement *pElement = pRootElement->FirstChildElement("audio");

  if (pElement)
  {
    XMLUtils::GetFloat(pElement, "ac3downmixgain", m_ac3Gain, -96.0f, 96.0f);
    XMLUtils::GetInt(pElement, "headroom", m_headRoom, 0, 12);
    XMLUtils::GetString(pElement, "defaultplayer", m_defaultPlayer);
    // 101 on purpose - can be used to never automark as watched
    XMLUtils::GetFloat(pElement, "playcountminimumpercent", m_playCountMinimumPercent, 0.0f, 101.0f);

    XMLUtils::GetBoolean(pElement, "usetimeseeking", m_musicUseTimeSeeking);
    XMLUtils::GetInt(pElement, "timeseekforward", m_musicTimeSeekForward, 0, 6000);
    XMLUtils::GetInt(pElement, "timeseekbackward", m_musicTimeSeekBackward, -6000, 0);
    XMLUtils::GetInt(pElement, "timeseekforwardbig", m_musicTimeSeekForwardBig, 0, 6000);
    XMLUtils::GetInt(pElement, "timeseekbackwardbig", m_musicTimeSeekBackwardBig, -6000, 0);

    XMLUtils::GetInt(pElement, "percentseekforward", m_musicPercentSeekForward, 0, 100);
    XMLUtils::GetInt(pElement, "percentseekbackward", m_musicPercentSeekBackward, -100, 0);
    XMLUtils::GetInt(pElement, "percentseekforwardbig", m_musicPercentSeekForwardBig, 0, 100);
    XMLUtils::GetInt(pElement, "percentseekbackwardbig", m_musicPercentSeekBackwardBig, -100, 0);

    XMLUtils::GetInt(pElement, "resample", m_musicResample, 0, 192000);

    TiXmlElement* pAudioExcludes = pElement->FirstChildElement("excludefromlisting");
    if (pAudioExcludes)
      GetCustomRegexps(pAudioExcludes, m_excludeFromListingRegExps);

    pAudioExcludes = pElement->FirstChildElement("excludefromscan");
    if (pAudioExcludes)
      GetCustomRegexps(pAudioExcludes, m_excludeFromScanRegExps);

    XMLUtils::GetString(pElement, "audiohost", m_host);
    XMLUtils::GetBoolean(pElement, "applydrc", m_applyDrc);
    XMLUtils::GetBoolean(pElement, "dvdplayerignoredtsinwav", m_dvdplayerIgnoreDTSinWAV);
  }
}

void CAudioSettings::Clear()
{
  m_excludeFromScanRegExps.clear();
  m_excludeFromListingRegExps.clear();
}

bool CAudioSettings::DVDPlayerIgnoreDTSInWav()
{
  return m_dvdplayerIgnoreDTSinWAV;
}

bool CAudioSettings::ApplyDRC()
{
  return m_applyDrc;
}

bool CAudioSettings::CanMusicUseTimeSeeking()
{
  return m_musicUseTimeSeeking;
}

int CAudioSettings::MusicTimeSeekForward()
{
  return m_musicTimeSeekForward;
}

int CAudioSettings::MusicTimeSeekBackward()
{
  return m_musicTimeSeekBackward;
}

int CAudioSettings::MusicTimeSeekForwardBig()
{
  return m_musicTimeSeekForwardBig;
}

int CAudioSettings::MusicTimeSeekBackwardBig()
{
  return m_musicTimeSeekBackwardBig;
}

int CAudioSettings::MusicPercentSeekForward()
{
  return m_musicPercentSeekForward;
}

int CAudioSettings::MusicPercentSeekBackward()
{
  return m_musicPercentSeekBackward;
}

int CAudioSettings::MusicPercentSeekForwardBig()
{
  return m_musicPercentSeekForwardBig;
}

int CAudioSettings::MusicPercentSeekBackwardBig()
{
  return m_musicPercentSeekBackwardBig;
}

int CAudioSettings::MusicResample()
{
  return m_musicResample;
}

float CAudioSettings::AC3Gain()
{
  return m_ac3Gain;
}

float CAudioSettings::PlayCountMinimumPercent()
{
  return m_playCountMinimumPercent;
}

CStdString CAudioSettings::DefaultPlayer()
{
  return m_defaultPlayer;
}

CStdStringArray CAudioSettings::ExcludeFromListingRegExps()
{
  return m_excludeFromListingRegExps;
}

CStdStringArray CAudioSettings::ExcludeFromScanRegExps()
{
  return m_excludeFromScanRegExps;
}

void CAudioSettings::Initialise()
{
  m_headRoom = 0;
  m_ac3Gain = 12.0f;
  m_applyDrc = true;
  m_dvdplayerIgnoreDTSinWAV = false;
  m_defaultPlayer = "paplayer";
  m_playCountMinimumPercent = 90.0f;
  m_host = "default";
  m_musicUseTimeSeeking = true;
  m_musicTimeSeekForward = 10;
  m_musicTimeSeekBackward = -10;
  m_musicTimeSeekForwardBig = 60;
  m_musicTimeSeekBackwardBig = -60;
  m_musicPercentSeekForward = 1;
  m_musicPercentSeekBackward = -1;
  m_musicPercentSeekForwardBig = 10;
  m_musicPercentSeekBackwardBig = -10;
  m_musicResample = 0;
}

void CAudioSettings::GetCustomRegexps(TiXmlElement *pRootElement, CStdStringArray& settings)
{
  TiXmlElement *pElement = pRootElement;
  while (pElement)
  {
    int iAction = 0; // overwrite
    // for backward compatibility
    const char* szAppend = pElement->Attribute("append");
    if ((szAppend && stricmp(szAppend, "yes") == 0))
      iAction = 1;
    // action takes precedence if both attributes exist
    const char* szAction = pElement->Attribute("action");
    if (szAction)
    {
      iAction = 0; // overwrite
      if (stricmp(szAction, "append") == 0)
        iAction = 1; // append
      else if (stricmp(szAction, "prepend") == 0)
        iAction = 2; // prepend
    }
    if (iAction == 0)
      settings.clear();
    TiXmlNode* pRegExp = pElement->FirstChild("regexp");
    int i = 0;
    while (pRegExp)
    {
      if (pRegExp->FirstChild())
      {
        CStdString regExp = pRegExp->FirstChild()->Value();
        if (iAction == 2)
          settings.insert(settings.begin() + i++, 1, regExp);
        else
          settings.push_back(regExp);
      }
      pRegExp = pRegExp->NextSibling("regexp");
    }

    pElement = pElement->NextSiblingElement(pRootElement->Value());
  }
}