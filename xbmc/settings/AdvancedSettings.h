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
#include "MediaProviderSettings.h"
#include "PictureSettings.h"
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
    CAudioSettings *AudioSettings() { return m_audioSettings; };
    CKaraokeSettings *KaraokeSettings() { return m_karaokeSettings; };
    CLibrarySettings *LibrarySettings() { return m_librarySettings; };
    CMediaProviderSettings *MediaProviderSettings() { return m_mediaProviderSettings; };
    CPictureSettings *PictureSettings() { return m_pictureSettings; };
    CSystemSettings *SystemSettings() { return m_systemSettings; };
    CVideoAdvancedSettings *VideoSettings() { return m_videoSettings; };
    int m_lcdRows;
    int m_lcdColumns;
    int m_lcdAddress1;
    int m_lcdAddress2;
    int m_lcdAddress3;
    int m_lcdAddress4;
    int m_lcdScrolldelay;
    bool m_lcdDimOnScreenSave;
    bool m_lcdHeartbeat;
    CStdString m_lcdHostName;
    float m_controllerDeadzone;
  private:
    void ParseSettingsFile(CStdString file);
    std::vector<CStdString> m_settingsFiles;
    CAudioSettings *m_audioSettings;
    CKaraokeSettings *m_karaokeSettings;
    CLibrarySettings *m_librarySettings;
    CMediaProviderSettings *m_mediaProviderSettings;
    CPictureSettings *m_pictureSettings;
    CSystemSettings *m_systemSettings;
    CVideoAdvancedSettings *m_videoSettings;
};

XBMC_GLOBAL(CAdvancedSettings,g_advancedSettings);