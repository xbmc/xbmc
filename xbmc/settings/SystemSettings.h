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

class CSystemSettings
{
  public:
    typedef std::vector<std::pair<CStdString, CStdString>> StringMapping;

    CSystemSettings(bool isStandalone);
    CSystemSettings(bool isStandalone, TiXmlElement *pRootElement);
    int AutoDetectPingTime();
    int CurlConnectTimeout();
    int CurlLowSpeedTime();
    int CurlRetries();
    int SambaClientTimeout();
    void SetLogLevel(int logLevel);
    int LogLevel();
    void SetLogLevelAndHint(int logLevelHint);
    int LogLevelHint();
    int BGInfoLoaderMaxThreads();
    int RemoteDelay();        ///< brief number of remote messages to ignore before repeating
    unsigned int CacheMemBufferSize();
    unsigned int JSONTCPPort();
    unsigned int RestrictCapsMask();
    float SleepBeforeSlip();  ///< if greater than zero, XBMC waits for raster to be this amount through the frame prior to calling the flip
    bool CurlDisableIPV6();
    bool OutputCompactJSON();
    bool SambaStatFiles();
    bool HTTPDirectoryStatFilesize();
    bool FTPThumbs();
    void SetHandleMounting(bool handleMounting);
    bool HandleMounting();
    bool NoDVDROM();
    bool CanQuit();
    bool CanWindowed();
    bool ShowSplash();
    bool StartFullScreen();
    void SetStartFullScreen(bool startFullScreen);
    bool UseVirtualShares();
    bool UseEvilB();
    bool AlwaysOnTop();
    bool EnableMultimediaKeys();
    CStdString SambaDOSCodePage();
    CStdString CachePath();
    CStdString CPUTempCMD();
    CStdString GPUTempCMD();
    StringMapping PathSubstitutions() { return m_pathSubstitutions; };
  private:
    void Initialise(bool isStandalone);
    int m_autoDetectPingTime;
    int m_curlConnectTimeout;
    int m_curlLowSpeedTime;
    int m_curlRetries;
    int m_sambaClientTimeout;
    int m_logLevel;
    int m_logLevelHint;
    int m_bgInfoLoaderMaxThreads;
    int m_remoteDelay;
    unsigned int m_cacheMemBufferSize;
    unsigned int m_jsonTcpPort;
    unsigned int m_restrictCapsMask;
    float m_sleepBeforeFlip;
    bool m_curlDisableIPV6;
    bool m_jsonOutputCompact;
    bool m_sambaStatFiles;
    bool m_HTTPDirectoryStatFilesize;
    bool m_FTPThumbs;
    bool m_handleMounting;
    bool m_noDVDROM;
    bool m_canQuit;
    bool m_canWindowed;
    bool m_showSplash;
    bool m_startFullScreen;
    bool m_useVirtualShares;
    bool m_useEvilB;
    bool m_alwaysOnTop;
    bool m_enableMultimediaKeys;
    CStdString m_sambaDOSCodePage;
    CStdString m_cachePath;
    CStdString m_cpuTempCmd;
    CStdString m_gpuTempCmd;
    StringMapping m_pathSubstitutions;
};
