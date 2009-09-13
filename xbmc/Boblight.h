#pragma once

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

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#ifdef HAVE_LIBBOBLIGHT_LIBBOBLIGHT_H
#define HAVE_BOBLIGHT

#include <string>

#include "stdafx.h"
#include "Thread.h"
#include "utils/CriticalSection.h"
#include "cores/VideoRenderers/RenderManager.h"
#include "Texture.h"

class CBoblightClient : public CThread
{
  public:
    CBoblightClient();
    bool IsEnabled();
    void GrabImage();
    void Send();
    void Disable();
    
  private:
    void Process();
    
    std::string      m_liberror;    //where we store the error from boblight_loadlibrary()
    bool             m_isenabled;   //if we have a connection to boblightd
    bool             m_hasinput;    //if application is providing us with input
    int              m_priority;    //priority of us as a client of boblightd
    
    CEvent           m_inputevent;  //set when we receive input and we need to send it to boblightd
    CCriticalSection m_critsection; //lock for the sdl surface
    
    CTexture         m_texture;     //where we store the thumbnail
    
    void*            m_boblight;    //handle from boblight_init()
    
    bool             Setup();       //sets up a connection to boblightd
    void             Cleanup();     //cleans up connection to boblightd
    void             Run();         //waits for input, on a timeout calls boblight_ping to check if boblight is still alive
};

extern CBoblightClient g_boblight;

#endif //HAVE_LIBBOBLIGHT_LIBBOBLIGHT_H
#endif //BOBLIGHT