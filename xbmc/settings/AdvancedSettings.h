#pragma once
/*
 *      Copyright (C) 2005-2008 Team XBMC
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
#include "utils/StdString.h"
#include "utils/GlobalsHandling.h"
#include "AudioSettings.h"
#include "KaraokeSettings.h"
#include "LibrarySettings.h"
#include "SystemSettings.h"
#include "VideoAdvancedSettings.h"

class TiXmlElement;

class CAdvancedSettings
{
  public:
    CAdvancedSettings();
    ~CAdvancedSettings();
    static CAdvancedSettings* getInstance();

    void Initialize();
    void AddSettingsFile(CStdString filename);
    bool Load();
    void Clear();

    static void GetCustomTVRegexps(TiXmlElement *pRootElement, SETTINGS_TVSHOWLIST& settings);
    static void GetCustomRegexps(TiXmlElement *pRootElement, CStdStringArray& settings);
    static void GetCustomRegexpReplacers(TiXmlElement *pRootElement, CStdStringArray& settings);
    static void GetCustomExtensions(TiXmlElement *pRootElement, CStdString& extensions);

    CAudioSettings *AudioSettings;
    CKaraokeSettings *KaraokeSettings;
    CLibrarySettings *LibrarySettings;
    CSystemSettings *SystemSettings;
    CVideoAdvancedSettings *VideoSettings;

    float m_slideshowBlackBarCompensation;
    float m_slideshowZoomAmount;
    float m_slideshowPanAmount;

    int m_lcdRows;
    int m_lcdColumns;
    int m_lcdAddress1;
    int m_lcdAddress2;
    int m_lcdAddress3;
    int m_lcdAddress4;
    bool m_lcdDimOnScreenSave;
    int m_lcdScrolldelay;
    bool m_lcdHeartbeat;
    CStdString m_lcdHostName;

    int m_songInfoDuration;
    int m_busyDialogDelay;

    CStdString m_cddbAddress;

    CStdStringArray m_pictureExcludeFromListingRegExps;
    
    int m_remoteDelay; ///< \brief number of remote messages to ignore before repeating
    float m_controllerDeadzone;

    //TuxBox
    int m_iTuxBoxStreamtsPort;
    bool m_bTuxBoxSubMenuSelection;
    int m_iTuxBoxDefaultSubMenu;
    int m_iTuxBoxDefaultRootMenu;
    bool m_bTuxBoxAudioChannelSelection;
    bool m_bTuxBoxPictureIcon;
    int m_iTuxBoxEpgRequestTime;
    int m_iTuxBoxZapWaitTime;
    bool m_bTuxBoxSendAllAPids;
    bool m_bTuxBoxZapstream;
    int m_iTuxBoxZapstreamPort;

    int m_iMythMovieLength;         // minutes

    // EDL Commercial Break
    bool m_bEdlMergeShortCommBreaks;
    int m_iEdlMaxCommBreakLength;   // seconds
    int m_iEdlMinCommBreakLength;   // seconds
    int m_iEdlMaxCommBreakGap;      // seconds
    int m_iEdlMaxStartGap;          // seconds
    int m_iEdlCommBreakAutowait;    // seconds
    int m_iEdlCommBreakAutowind;    // seconds

    bool m_bFirstLoop;


    bool UseGLRectangeHack() { return m_GLRectangleHack; };
    int m_iSkipLoopFilter;
    float m_ForcedSwapTime; /* if nonzero, set's the explicit time in ms to allocate for buffer swap */

    bool AllowD3D9Ex() { return m_AllowD3D9Ex; };
    bool ForceD3D9Ex() { return m_ForceD3D9Ex; };
    bool AllowDynamicTextures() { return m_AllowDynamicTextures; };


    bool MeasureRefreshRate() { return m_measureRefreshrate; }; //when true the videoreferenceclock will measure the refreshrate when direct3d is used
                                                                //otherwise it will use the windows refreshrate



    bool IsInFullScreen() { return m_fullScreen; };
    void SetFullScreenState(bool isFullScreen) { m_fullScreen = isFullScreen; };
  private:
    bool m_measureRefreshrate;
    bool m_fullScreen;
    bool m_GLRectangleHack;
    bool m_AllowD3D9Ex;
    bool m_ForceD3D9Ex;
    bool m_AllowDynamicTextures;

    std::vector<CStdString> m_settingsFiles;
    void ParseSettingsFile(CStdString file);
};

XBMC_GLOBAL(CAdvancedSettings,g_advancedSettings);