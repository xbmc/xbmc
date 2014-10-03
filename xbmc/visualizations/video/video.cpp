/*
 *      Copyright (C) 2005-2013 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */


#ifdef __GNUC__
#define __cdecl
#endif

#include "guilib/GraphicContext.h"
#include "guilib/Texture.h"
#include "guilib/GUITexture.h"
#include "Application.h"
#include "filesystem/SpecialProtocol.h"
#include "settings/AdvancedSettings.h"
#include "settings/DisplaySettings.h"
#include "video/FFmpegVideoDecoder.h"
#include "system.h"
#include "utils/log.h"

#include "addons/include/xbmc_vis_types.h"
#include "addons/include/xbmc_vis_dll.h"


extern "C" {
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavutil/avutil.h"
#include "libswscale/swscale.h"
}



class CBaseTexture;
class FFmpegVideoDecoder;

CBaseTexture       *m_texture;
FFmpegVideoDecoder *m_decoder;

int    m_videoWidth, m_videoHeight; // shown video width/height, i.e. upscaled or downscaled as necessary
int    m_displayLeft, m_displayRight, m_displayTop, m_displayBottom; // Video as shown at the display
double m_secondsPerFrame, m_nextFrameTime, m_timeFromPrevSong;
char   m_videoFile[1024] = "";

bool openVideoFile(const CStdString& filename)
{
    CLog::Log(LOGDEBUG, "Video Visualization: Open: '%s'", filename.c_str());

    CStdString realPath = CSpecialProtocol::TranslatePath(filename);

    if (!m_decoder->open(realPath))
    {
        CLog::Log(LOGERROR, "Video Visualization: %s, video file %s (%s)", m_decoder->getErrorMsg().c_str(), filename.c_str(), realPath.c_str());
        return false;
    }

    m_videoWidth  = m_decoder->getWidth();
    m_videoHeight = m_decoder->getHeight();
  
    // Find out the necessary aspect ratio for height (assuming fit by width) and width (assuming fit by height)
    const RESOLUTION_INFO info = g_graphicsContext.GetResInfo();
    m_displayLeft   = info.Overscan.left;
    m_displayRight  = info.Overscan.right;
    m_displayTop    = info.Overscan.top;
    m_displayBottom = info.Overscan.bottom;
  
    int screen_width  = m_displayRight  - m_displayLeft;
    int screen_height = m_displayBottom - m_displayTop;

    // Do we need to modify the output video size? This could happen in two cases:
    // 1. Either video dimension is larger than the screen - video needs to be downscaled
    // 2. Both video dimensions are smaller than the screen - video needs to be upscaled
    if ((m_videoWidth > 0 && m_videoHeight > 0)
    && ((m_videoWidth > screen_width || m_videoHeight > screen_height)
    || (m_videoWidth < screen_width && m_videoHeight < screen_height)))
    {
        // Calculate the scale coefficients for width/height separately
        double scale_width = (double)screen_width / (double)m_videoWidth;
        double scale_height = (double)screen_height / (double)m_videoHeight;

        // And apply the smallest
        double scale  = scale_width < scale_height ? scale_width : scale_height;
        m_videoWidth  = (int)(m_videoWidth * scale);
        m_videoHeight = (int)(m_videoHeight * scale);
    }

    // Calculate the desktop dimensions to show the video
    if (m_videoWidth < screen_width || m_videoHeight < screen_height)
    {
        m_displayLeft = (screen_width - m_videoWidth) / 2;
        m_displayRight -= m_displayLeft;

        m_displayTop = (screen_height - m_videoHeight) / 2;
        m_displayBottom -= m_displayTop;
    }
  
    m_secondsPerFrame = 1.0 / m_decoder->getFramesPerSecond();

    CLog::Log(LOGDEBUG, "Video Visualization: Video file %s (%dx%d) length %g seconds opened successfully, will be shown as %dx%d at (%d, %d - %d, %d) rectangle",
        filename.c_str(), m_decoder->getWidth(), m_decoder->getHeight(), m_decoder->getDuration(),
        m_videoWidth, m_videoHeight, m_displayLeft, m_displayTop, m_displayRight, m_displayBottom);
  
    return true;
}


extern "C" ADDON_STATUS ADDON_Create(void* hdl, void* props)
{
    CLog::Log(LOGDEBUG, "Video Visualization: Create");

    m_decoder = new FFmpegVideoDecoder();
    m_timeFromPrevSong = 0.0;
    m_texture = 0;

    return ADDON_STATUS_NEED_SETTINGS;
}


extern "C" void Start(int iChannels, int iSamplesPerSec, int iBitsPerSample, const char* szSongName)
{
    CLog::Log(LOGDEBUG, "Video Visualization: Start");
    
    if (!openVideoFile(m_videoFile))
        return;

    if (m_timeFromPrevSong != 0.0 && !m_decoder->seek(m_timeFromPrevSong))
        m_timeFromPrevSong = 0;
  
    m_texture = new CTexture(m_videoWidth, m_videoHeight, XB_FMT_A8R8G8B8);
  
    if (!m_texture)
    {
        CLog::Log( LOGERROR, "Video Visualization: Could not allocate texture" );
        return;
    }
  
    m_nextFrameTime = 0.0;
}


extern "C" void Render()
{
//    CLog::Log(LOGDEBUG, "Video Visualization: Render");
    
    if (!m_texture)
        return;
  
    struct timeval now;
    gettimeofday(&now, NULL);
    double current = (double)now.tv_sec + ((double)now.tv_usec / (double)CLOCKS_PER_SEC);

    // We're supposed to show m_decoder->getFramesPerSecond() frames in one second.
    if (current >= m_nextFrameTime)
    {	// We don't care to adjust for the exact timing
	m_nextFrameTime = current + m_secondsPerFrame - (current - m_nextFrameTime);

	while (true)
        {
	    if (!m_decoder->nextFrame(m_texture))
            {	// End of video; restart
		m_decoder->seek(0.0);
		m_nextFrameTime = 0.0;
		continue;
            }
            break;
	}
    }

    CRect vertCoords((float)m_displayLeft, (float)m_displayTop, (float)m_displayRight, (float)m_displayBottom);
    CGUITexture::DrawQuad(vertCoords, 0xffffffff, m_texture);
}


extern "C" void ADDON_Stop()
{
    CLog::Log(LOGDEBUG, "Video Visualization: Stop");
    
    delete m_texture;
    m_texture = 0;

    m_timeFromPrevSong = m_decoder->getLastFrameTime();
    m_decoder->close();
}


extern "C" void ADDON_Destroy()
{
    CLog::Log(LOGDEBUG, "Video Visualization: Destroy");
    
    delete m_decoder;
    delete m_texture;
}


extern "C" void GetInfo(VIS_INFO* pInfo)
{
    pInfo->bWantsFreq = false;
    pInfo->iSyncDelay = 0;
}


extern "C" ADDON_STATUS ADDON_SetSetting(const char *strSetting, const void* value)
{
    CLog::Log(LOGDEBUG, "Video Visualization: SetSetting %s: '%s'", strSetting, value);

    if (!strSetting || !value)
        return ADDON_STATUS_UNKNOWN;

    if (!strncmp(strSetting, "video", 5))
    {
        strcpy(m_videoFile, (char*)value);
    }

    return ADDON_STATUS_OK;
}


extern "C" bool ADDON_HasSettings()
{
    return true;
}


// These do nothing
extern "C" bool IsLocked() { return false; }
extern "C" bool OnAction(long flags, const void *param) { return false; }
extern "C" void AudioData(const float* pAudioData, int iAudioDataLength, float*, int) { }
extern "C" unsigned GetPreset() { return 0; }
extern "C" unsigned int GetPresets(char ***presets) { return 0; }
extern "C" unsigned int GetSubModules(char ***names) { return 0; }
extern "C" unsigned int ADDON_GetSettings(ADDON_StructSetting ***sSet) { return 0; }
extern "C" void ADDON_Announce(const char *flag, const char *sender, const char *message, const void *data) { }
extern "C" void ADDON_FreeSettings() { }
extern "C" ADDON_STATUS ADDON_GetStatus() { return ADDON_STATUS_OK; }
