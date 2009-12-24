/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
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

#ifndef BOBLIGHT
#define BOBLIGHT

#include "Thread.h"
#include "Texture.h"
#include "utils/CriticalSection.h"

class CBoblightClient : public CThread
{
  public:
    void Initialize();
    void Start();
    void CaptureVideo();
    void ProcessVideo();
    void Disable();
    void Stop();

  private:
    void Process();

    bool     m_haslib;

    CTexture m_texture;
    CEvent   m_captureevent;
    bool     m_enabled;
    bool     m_captureandsend;
    bool     m_needsprocessing;

    bool     LoadSettings();
    bool     m_needsettingload;
    float    m_value;
    float    m_valuerangemin;
    float    m_valuerangemax;
    float    m_saturation;
    float    m_satrangemin;
    float    m_satrangemax;
    float    m_speed;
    float    m_autospeed;

    bool     Connect();
    bool     SetBoblightSettings();
    void*    m_boblight;
    int      m_priority;

    unsigned char*   m_pixels;
    GLuint           m_pbo;
    CCriticalSection m_critsection;
};

extern CBoblightClient g_boblight;

#endif //BOBLIGHT
