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

#include <vector>
#include "utils/XMLUtils.h"

struct RefreshOverride
{
  float fpsmin;
  float fpsmax;
  float refreshmin;
  float refreshmax;
  bool fallback;
};

class CVideoAdvancedSettings
{
  public:
    CVideoAdvancedSettings();
    CVideoAdvancedSettings(TiXmlElement *pRootElement);
    void Clear();
    int TimeSeekForward();
    int TimeSeekBackward();
    int TimeSeekForwardBig();
    int TimeSeekBackwardBig();
    int PercentSeekForward();
    int PercentSeekBackward();
    int PercentSeekForwardBig();
    int PercentSeekBackwardBig();
    int SmallStepBackSeconds();
    int SmallStepBackTries();
    int SmallStepBackDelay();
    int BlackBarColour();
    int IgnoreSecondsAtStart();
    float IgnorePercentAtEnd();
    float SubsDelayRange();
    float AudioDelayRange();
    float NonLinStretchRatio();
    float AutoScaleMaxFPS();
    float PlayCountMinimumPercent();
    bool AllowLanczos3();
    bool VDPAUScaling();
    bool AllowMPEG4VDPAU();
    bool DXVACheckCompatibility();
    bool DXVACheckCompatibilityPresent();
    bool CanVideoUseTimeSeeking();
    bool FullScreenOnMovieStart();
    std::vector<RefreshOverride> AdjustRefreshOverrides();
    CStdString PPFFMPEGDeint();
    CStdString PPFFMPEGPostProc();
    CStdString DefaultPlayer();
    CStdString DefaultDVDPlayer();
    CStdString CleanDateTimeRegExp();
    CStdStringArray CleanStringRegExps();
    CStdStringArray ExcludeFromListingRegExps();
    CStdStringArray MoviesExcludeFromScanRegExps();
    CStdStringArray TVShowExcludeFromScanRegExps();
    CStdStringArray TrailerMatchRegExps();
  private:
    void Initialise();
    static void GetCustomRegexps(TiXmlElement *pRootElement, CStdStringArray& settings);
    int m_timeSeekForward;
    int m_timeSeekBackward;
    int m_timeSeekForwardBig;
    int m_timeSeekBackwardBig;
    int m_percentSeekForward;
    int m_percentSeekBackward;
    int m_percentSeekForwardBig;
    int m_percentSeekBackwardBig;
    int m_smallStepBackSeconds;
    int m_smallStepBackTries;
    int m_smallStepBackDelay;
    int m_blackBarColour;
    int m_ignoreSecondsAtStart;
    float m_ignorePercentAtEnd;
    float m_subsDelayRange;
    float m_audioDelayRange;
    float m_nonLinStretchRatio;
    float m_autoScaleMaxFps;
    float m_playCountMinimumPercent;
    bool m_allowLanczos3;
    bool m_VDPAUScaling;
    bool m_allowMpeg4VDPAU;
    bool m_DXVACheckCompatibility;
    bool m_DXVACheckCompatibilityPresent;
    bool m_useTimeSeeking;
    bool m_fullScreenOnMovieStart;
    std::vector<RefreshOverride> m_adjustRefreshOverrides;
    CStdString m_PPFFmpegDeint;
    CStdString m_PPFFmpegPostProc;
    CStdString m_defaultPlayer;
    CStdString m_defaultDVDPlayer;
    CStdString m_cleanDateTimeRegExp;
    CStdStringArray m_cleanStringRegExps;
    CStdStringArray m_excludeFromListingRegExps;
    CStdStringArray m_moviesExcludeFromScanRegExps;
    CStdStringArray m_tvshowExcludeFromScanRegExps;
    CStdStringArray m_trailerMatchRegExps;
};