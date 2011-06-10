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

#include "VideoAdvancedSettings.h"
#include "utils/XMLUtils.h"
#include "utils/log.h"

CVideoAdvancedSettings::CVideoAdvancedSettings()
{
  Initialise();
}

CVideoAdvancedSettings::CVideoAdvancedSettings(TiXmlElement *pRootElement)
{
  Initialise();
  TiXmlElement *pElement = pRootElement->FirstChildElement("video");
  if (pElement)
  {
    XMLUtils::GetFloat(pElement, "subsdelayrange", m_subsDelayRange, 10, 600);
    XMLUtils::GetFloat(pElement, "audiodelayrange", m_audioDelayRange, 10, 600);
    XMLUtils::GetInt(pElement, "blackbarcolour", m_blackBarColour, 0, 255);
    XMLUtils::GetString(pElement, "defaultplayer", m_defaultPlayer);
    XMLUtils::GetString(pElement, "defaultdvdplayer", m_defaultDVDPlayer);
    XMLUtils::GetBoolean(pElement, "fullscreenonmoviestart", m_fullScreenOnMovieStart);
    // 101 on purpose - can be used to never automark as watched
    XMLUtils::GetFloat(pElement, "playcountminimumpercent", m_playCountMinimumPercent, 0.0f, 101.0f);
    XMLUtils::GetInt(pElement, "ignoresecondsatstart", m_ignoreSecondsAtStart, 0, 900);
    XMLUtils::GetFloat(pElement, "ignorepercentatend", m_ignorePercentAtEnd, 0, 100.0f);

    XMLUtils::GetInt(pElement, "smallstepbackseconds", m_smallStepBackSeconds, 1, INT_MAX);
    XMLUtils::GetInt(pElement, "smallstepbacktries", m_smallStepBackTries, 1, 10);
    XMLUtils::GetInt(pElement, "smallstepbackdelay", m_smallStepBackDelay, 100, 5000); //MS

    XMLUtils::GetBoolean(pElement, "usetimeseeking", m_useTimeSeeking);
    XMLUtils::GetInt(pElement, "timeseekforward", m_timeSeekForward, 0, 6000);
    XMLUtils::GetInt(pElement, "timeseekbackward", m_timeSeekBackward, -6000, 0);
    XMLUtils::GetInt(pElement, "timeseekforwardbig", m_timeSeekForwardBig, 0, 6000);
    XMLUtils::GetInt(pElement, "timeseekbackwardbig", m_timeSeekBackwardBig, -6000, 0);

    XMLUtils::GetInt(pElement, "percentseekforward", m_percentSeekForward, 0, 100);
    XMLUtils::GetInt(pElement, "percentseekbackward", m_percentSeekBackward, -100, 0);
    XMLUtils::GetInt(pElement, "percentseekforwardbig", m_percentSeekForwardBig, 0, 100);
    XMLUtils::GetInt(pElement, "percentseekbackwardbig", m_percentSeekBackwardBig, -100, 0);

    TiXmlElement* pVideoExcludes = pElement->FirstChildElement("excludefromlisting");
    if (pVideoExcludes)
      GetCustomRegexps(pVideoExcludes, m_excludeFromListingRegExps);

    pVideoExcludes = pElement->FirstChildElement("excludefromscan");
    if (pVideoExcludes)
      GetCustomRegexps(pVideoExcludes, m_moviesExcludeFromScanRegExps);

    pVideoExcludes = pElement->FirstChildElement("excludetvshowsfromscan");
    if (pVideoExcludes)
      GetCustomRegexps(pVideoExcludes, m_tvshowExcludeFromScanRegExps);

    pVideoExcludes = pElement->FirstChildElement("cleanstrings");
    if (pVideoExcludes)
      GetCustomRegexps(pVideoExcludes, m_cleanStringRegExps);

    XMLUtils::GetString(pElement,"cleandatetime", m_cleanDateTimeRegExp);
    XMLUtils::GetString(pElement,"ppffmpegdeinterlacing",m_PPFFmpegDeint);
    XMLUtils::GetString(pElement,"ppffmpegpostprocessing",m_PPFFmpegPostProc);
    XMLUtils::GetBoolean(pElement,"vdpauscaling",m_VDPAUScaling);
    XMLUtils::GetFloat(pElement, "nonlinearstretchratio", m_nonLinStretchRatio, 0.01f, 1.0f);
    XMLUtils::GetBoolean(pElement,"allowlanczos3",m_allowLanczos3);
    XMLUtils::GetFloat(pElement,"autoscalemaxfps",m_autoScaleMaxFps, 0.0f, 1000.0f);
    XMLUtils::GetBoolean(pElement,"allowmpeg4vdpau",m_allowMpeg4VDPAU);

    TiXmlElement* pAdjustRefreshrate = pElement->FirstChildElement("adjustrefreshrate");
    if (pAdjustRefreshrate)
    {
      TiXmlElement* pRefreshOverride = pAdjustRefreshrate->FirstChildElement("override");
      while (pRefreshOverride)
      {
        RefreshOverride override = {0};

        float fps;
        if (XMLUtils::GetFloat(pRefreshOverride, "fps", fps))
        {
          override.fpsmin = fps - 0.01f;
          override.fpsmax = fps + 0.01f;
        }

        float fpsmin, fpsmax;
        if (XMLUtils::GetFloat(pRefreshOverride, "fpsmin", fpsmin) &&
            XMLUtils::GetFloat(pRefreshOverride, "fpsmax", fpsmax))
        {
          override.fpsmin = fpsmin;
          override.fpsmax = fpsmax;
        }

        float refresh;
        if (XMLUtils::GetFloat(pRefreshOverride, "refresh", refresh))
        {
          override.refreshmin = refresh - 0.01f;
          override.refreshmax = refresh + 0.01f;
        }

        float refreshmin, refreshmax;
        if (XMLUtils::GetFloat(pRefreshOverride, "refreshmin", refreshmin) &&
            XMLUtils::GetFloat(pRefreshOverride, "refreshmax", refreshmax))
        {
          override.refreshmin = refreshmin;
          override.refreshmax = refreshmax;
        }

        bool fpsCorrect     = (override.fpsmin > 0.0f && override.fpsmax >= override.fpsmin);
        bool refreshCorrect = (override.refreshmin > 0.0f && override.refreshmax >= override.refreshmin);

        if (fpsCorrect && refreshCorrect)
          m_adjustRefreshOverrides.push_back(override);
        else
          CLog::Log(LOGWARNING, "Ignoring malformed refreshrate override, fpsmin:%f fpsmax:%f refreshmin:%f refreshmax:%f",
              override.fpsmin, override.fpsmax, override.refreshmin, override.refreshmax);

        pRefreshOverride = pRefreshOverride->NextSiblingElement("override");
      }

      TiXmlElement* pRefreshFallback = pAdjustRefreshrate->FirstChildElement("fallback");
      while (pRefreshFallback)
      {
        RefreshOverride fallback = {0};
        fallback.fallback = true;

        float refresh;
        if (XMLUtils::GetFloat(pRefreshFallback, "refresh", refresh))
        {
          fallback.refreshmin = refresh - 0.01f;
          fallback.refreshmax = refresh + 0.01f;
        }

        float refreshmin, refreshmax;
        if (XMLUtils::GetFloat(pRefreshFallback, "refreshmin", refreshmin) &&
            XMLUtils::GetFloat(pRefreshFallback, "refreshmax", refreshmax))
        {
          fallback.refreshmin = refreshmin;
          fallback.refreshmax = refreshmax;
        }

        if (fallback.refreshmin > 0.0f && fallback.refreshmax >= fallback.refreshmin)
          m_adjustRefreshOverrides.push_back(fallback);
        else
          CLog::Log(LOGWARNING, "Ignoring malformed refreshrate fallback, fpsmin:%f fpsmax:%f refreshmin:%f refreshmax:%f",
              fallback.fpsmin, fallback.fpsmax, fallback.refreshmin, fallback.refreshmax);

        pRefreshFallback = pRefreshFallback->NextSiblingElement("fallback");
      }
    }

    m_DXVACheckCompatibilityPresent = XMLUtils::GetBoolean(pElement,"checkdxvacompatibility", m_DXVACheckCompatibility);

  }

  // trailer matching regexps
  TiXmlElement* pTrailerMatching = pRootElement->FirstChildElement("trailermatching");
  if (pTrailerMatching)
    GetCustomRegexps(pTrailerMatching, m_trailerMatchRegExps);

  //everything thats a trailer is not a movie
  m_moviesExcludeFromScanRegExps.insert(m_moviesExcludeFromScanRegExps.end(),
                                        m_trailerMatchRegExps.begin(),
                                        m_trailerMatchRegExps.end());

  XMLUtils::GetBoolean(pRootElement,"allowd3d9ex", m_allowD3D9Ex);
  XMLUtils::GetBoolean(pRootElement,"forced3d9ex", m_forceD3D9Ex);
  XMLUtils::GetBoolean(pRootElement,"allowdynamictextures", m_allowDynamicTextures);
  XMLUtils::GetBoolean(pRootElement,"glrectanglehack", m_GLRectangleHack);
  XMLUtils::GetBoolean(pRootElement, "measurerefreshrate", m_measureRefreshrate);
}

void CVideoAdvancedSettings::Clear()
{
  m_cleanStringRegExps.clear();
  m_excludeFromListingRegExps.clear();
  m_moviesExcludeFromScanRegExps.clear();
  m_tvshowExcludeFromScanRegExps.clear();
}

int CVideoAdvancedSettings::TimeSeekForward()
{
  return m_timeSeekForward;
}

int CVideoAdvancedSettings::TimeSeekBackward()
{
  return m_timeSeekBackward;
}

int CVideoAdvancedSettings::TimeSeekForwardBig()
{
  return m_timeSeekForwardBig;
}

int CVideoAdvancedSettings::TimeSeekBackwardBig()
{
  return m_timeSeekBackwardBig;
}

int CVideoAdvancedSettings::PercentSeekForward()
{
  return m_percentSeekForward;
}

int CVideoAdvancedSettings::PercentSeekBackward()
{
  return m_percentSeekBackward;
}

int CVideoAdvancedSettings::PercentSeekForwardBig()
{
  return m_percentSeekForwardBig;
}

int CVideoAdvancedSettings::PercentSeekBackwardBig()
{
  return m_percentSeekBackwardBig;
}

int CVideoAdvancedSettings::SmallStepBackSeconds()
{
  return m_smallStepBackSeconds;
}

int CVideoAdvancedSettings::SmallStepBackTries()
{
  return m_smallStepBackTries;
}

int CVideoAdvancedSettings::SmallStepBackDelay()
{
  return m_smallStepBackDelay;
}

int CVideoAdvancedSettings::BlackBarColour()
{
  return m_blackBarColour;
}

int CVideoAdvancedSettings::IgnoreSecondsAtStart()
{
  return m_ignoreSecondsAtStart;
}

float CVideoAdvancedSettings::IgnorePercentAtEnd()
{
  return m_ignorePercentAtEnd;
}

float CVideoAdvancedSettings::SubsDelayRange()
{
  return m_subsDelayRange;
}

float CVideoAdvancedSettings::AudioDelayRange()
{
  return m_audioDelayRange;
}

float CVideoAdvancedSettings::NonLinStretchRatio()
{
  return m_nonLinStretchRatio;
}

float CVideoAdvancedSettings::AutoScaleMaxFPS()
{
  return m_autoScaleMaxFps;
}

float CVideoAdvancedSettings::PlayCountMinimumPercent()
{
  return m_playCountMinimumPercent;
}

bool CVideoAdvancedSettings::AllowLanczos3()
{
  return m_allowLanczos3;
}

bool CVideoAdvancedSettings::VDPAUScaling()
{
  return m_VDPAUScaling;
}

bool CVideoAdvancedSettings::AllowMPEG4VDPAU()
{
  return m_allowMpeg4VDPAU;
}

bool CVideoAdvancedSettings::DXVACheckCompatibility()
{
  return m_DXVACheckCompatibility;
}

bool CVideoAdvancedSettings::DXVACheckCompatibilityPresent()
{
  return m_DXVACheckCompatibilityPresent;
}

bool CVideoAdvancedSettings::CanVideoUseTimeSeeking()
{
  return m_useTimeSeeking;
}

bool CVideoAdvancedSettings::FullScreenOnMovieStart()
{
  return m_fullScreenOnMovieStart;
}

bool CVideoAdvancedSettings::AllowD3D9Ex()
{
  return m_allowD3D9Ex;
}

bool CVideoAdvancedSettings::ForceD3D9Ex()
{
  return m_forceD3D9Ex;
}

bool CVideoAdvancedSettings::AllowDynamicTextures()
{
  return m_allowDynamicTextures;
}

bool CVideoAdvancedSettings::UseGLRectangeHack()
{
  return m_GLRectangleHack;
}

bool CVideoAdvancedSettings::MeasureRefreshRate()
{
  return m_measureRefreshrate;
}

bool CVideoAdvancedSettings::IsInFullScreen()
{
  return m_fullScreen;
}

void CVideoAdvancedSettings::SetFullScreenState(bool isFullScreen)
{
  m_fullScreen = isFullScreen;
}

std::vector<RefreshOverride> CVideoAdvancedSettings::AdjustRefreshOverrides()
{
  return m_adjustRefreshOverrides;
}

CStdString CVideoAdvancedSettings::PPFFMPEGDeint()
{
  return m_PPFFmpegDeint;
}

CStdString CVideoAdvancedSettings::PPFFMPEGPostProc()
{
  return m_PPFFmpegPostProc;
}

CStdString CVideoAdvancedSettings::DefaultPlayer()
{
  return m_defaultPlayer;
}

CStdString CVideoAdvancedSettings::DefaultDVDPlayer()
{
  return m_defaultDVDPlayer;
}

CStdString CVideoAdvancedSettings::CleanDateTimeRegExp()
{
  return m_cleanDateTimeRegExp;
}

CStdStringArray CVideoAdvancedSettings::CleanStringRegExps()
{
  return m_cleanStringRegExps;
}

CStdStringArray CVideoAdvancedSettings::ExcludeFromListingRegExps()
{
  return m_excludeFromListingRegExps;
}

CStdStringArray CVideoAdvancedSettings::MoviesExcludeFromScanRegExps()
{
  return m_moviesExcludeFromScanRegExps;
}

CStdStringArray CVideoAdvancedSettings::TVShowExcludeFromScanRegExps()
{
  return m_tvshowExcludeFromScanRegExps;
}

CStdStringArray CVideoAdvancedSettings::TrailerMatchRegExps()
{
  return m_trailerMatchRegExps;
}

void CVideoAdvancedSettings::Initialise()
{
  m_subsDelayRange = 10;
  m_audioDelayRange = 10;
  m_smallStepBackSeconds = 7;
  m_smallStepBackTries = 3;
  m_smallStepBackDelay = 300;
  m_useTimeSeeking = true;
  m_timeSeekForward = 30;
  m_timeSeekBackward = -30;
  m_timeSeekForwardBig = 600;
  m_timeSeekBackwardBig = -600;
  m_percentSeekForward = 2;
  m_percentSeekBackward = -2;
  m_percentSeekForwardBig = 10;
  m_percentSeekBackwardBig = -10;
  m_blackBarColour = 0;
  m_PPFFmpegDeint = "linblenddeint";
  m_PPFFmpegPostProc = "ha:128:7,va,dr";
  m_defaultPlayer = "dvdplayer";
  m_defaultDVDPlayer = "dvdplayer";
  m_ignoreSecondsAtStart = 3*60;
  m_ignorePercentAtEnd   = 8.0f;
  m_playCountMinimumPercent = 90.0f;
  m_VDPAUScaling = false;
  m_nonLinStretchRatio = 0.5f;
  m_allowLanczos3 = false;
  m_autoScaleMaxFps = 30.0f;
  m_allowMpeg4VDPAU = false;
  m_DXVACheckCompatibility = false;
  m_DXVACheckCompatibilityPresent = false;
  m_fullScreenOnMovieStart = true;
  m_allowD3D9Ex = true;
  m_forceD3D9Ex = false;
  m_allowDynamicTextures = true;
  m_fullScreen = false;
  m_GLRectangleHack = false;
  m_measureRefreshrate = false;
  m_cleanDateTimeRegExp = "(.*[^ _\\,\\.\\(\\)\\[\\]\\-])[ _\\.\\(\\)\\[\\]\\-]+(19[0-9][0-9]|20[0-1][0-9])([ _\\,\\.\\(\\)\\[\\]\\-]|[^0-9]$)";
  m_cleanStringRegExps.push_back("[ _\\,\\.\\(\\)\\[\\]\\-](ac3|dts|custom|dc|remastered|divx|divx5|dsr|dsrip|dutch|dvd|dvd5|dvd9|dvdrip|dvdscr|dvdscreener|screener|dvdivx|cam|fragment|fs|hdtv|hdrip|hdtvrip|internal|limited|multisubs|ntsc|ogg|ogm|pal|pdtv|proper|repack|rerip|retail|r3|r5|bd5|se|svcd|swedish|german|read.nfo|nfofix|unrated|extended|ws|telesync|ts|telecine|tc|brrip|bdrip|480p|480i|576p|576i|720p|720i|1080p|1080i|3d|hrhd|hrhdtv|hddvd|bluray|x264|h264|xvid|xvidvd|xxx|www.www|cd[1-9]|\\[.*\\])([ _\\,\\.\\(\\)\\[\\]\\-]|$)");
  m_cleanStringRegExps.push_back("(\\[.*\\])");
  m_moviesExcludeFromScanRegExps.push_back("-trailer");
  m_moviesExcludeFromScanRegExps.push_back("[-._ \\\\/]sample[-._ \\\\/]");
  m_tvshowExcludeFromScanRegExps.push_back("[-._ \\\\/]sample[-._ \\\\/]");
}

void CVideoAdvancedSettings::GetCustomRegexps(TiXmlElement *pRootElement, CStdStringArray& settings)
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