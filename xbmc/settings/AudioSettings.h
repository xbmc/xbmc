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

#include "utils/XMLUtils.h"

class CAudioSettings
{
  public:
    CAudioSettings();
    CAudioSettings(TiXmlElement *pRootElement);
    void Clear();
    bool DVDPlayerIgnoreDTSInWav();
    bool ApplyDRC();
    bool CanMusicUseTimeSeeking();
    int MusicTimeSeekForward();
    int MusicTimeSeekBackward();
    int MusicTimeSeekForwardBig();
    int MusicTimeSeekBackwardBig();
    int MusicPercentSeekForward();
    int MusicPercentSeekBackward();
    int MusicPercentSeekForwardBig();
    int MusicPercentSeekBackwardBig();
    int MusicResample();
    float AC3Gain();
    float PlayCountMinimumPercent();
    CStdString DefaultPlayer();
    CStdString Host();
    CStdStringArray ExcludeFromListingRegExps();
    CStdStringArray ExcludeFromScanRegExps();
  private:
    void Initialise();
    static void GetCustomRegexps(TiXmlElement *pRootElement, CStdStringArray& settings);
    bool m_dvdplayerIgnoreDTSinWAV;
    bool m_applyDrc;
    bool m_musicUseTimeSeeking;
    int m_headRoom;
    int m_musicTimeSeekForward;
    int m_musicTimeSeekBackward;
    int m_musicTimeSeekForwardBig;
    int m_musicTimeSeekBackwardBig;
    int m_musicPercentSeekForward;
    int m_musicPercentSeekBackward;
    int m_musicPercentSeekForwardBig;
    int m_musicPercentSeekBackwardBig;
    int m_musicResample;
    float m_ac3Gain;
    float m_playCountMinimumPercent;
    CStdString m_defaultPlayer;
    CStdString m_host;
    CStdStringArray m_excludeFromListingRegExps;
    CStdStringArray m_excludeFromScanRegExps;
};