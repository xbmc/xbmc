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

#include "system.h"

#ifdef HAS_DX
  #include "guilib/D3DResource.h"
#endif

#include "threads/Event.h"

enum ECAPTURESTATE
{
  CAPTURESTATE_WORKING,
  CAPTURESTATE_NEEDSRENDER,
  CAPTURESTATE_NEEDSREADOUT,
  CAPTURESTATE_DONE,
  CAPTURESTATE_FAILED,
  CAPTURESTATE_NEEDSDELETE
};

#define CAPTUREFLAG_CONTINUOUS  0x01 //after a render is done, render a new one immediately
#define CAPTUREFLAG_IMMEDIATELY 0x02 //read out immediately after render, this can cause a busy wait

class CRenderCaptureBase
{
  public:
    CRenderCaptureBase();
    ~CRenderCaptureBase();

    void SetState(ECAPTURESTATE state)          { m_state = state;     }
    ECAPTURESTATE GetState()                    { return m_state;      }
    void SetUserState(ECAPTURESTATE state)      { m_userState = state; }
    ECAPTURESTATE GetUserState()                { return m_userState;  }

    CEvent& GetEvent()                          { return m_event; }

    void         SetFlags(int flags)            { m_flags = flags; }
    int          GetFlags()                     { return m_flags;  }

    void         SetWidth(unsigned int width)   { m_width = width;   }
    void         SetHeight(unsigned int height) { m_height = height; }
    unsigned int GetWidth()                     { return m_width;    }
    unsigned int GetHeight()                    { return m_height;   }
    uint8_t*     GetPixels()                    { return m_pixels;   }

    bool         IsAsync()                      { return m_asyncSupported; }

  protected:
    ECAPTURESTATE    m_state;     //state for the rendermanager
    ECAPTURESTATE    m_userState; //state for the thread that wants the capture
    int              m_flags;
    CEvent           m_event;

    uint8_t*         m_pixels;
    unsigned int     m_width;
    unsigned int     m_height;
    unsigned int     m_bufferSize;

    //this is set after the first render
    bool             m_asyncSupported;
    bool             m_asyncChecked;
};

#if defined(HAS_GL) || defined(HAS_GLES)

class CRenderCaptureGL : public CRenderCaptureBase
{
  public:
    CRenderCaptureGL();
    ~CRenderCaptureGL();

    void  BeginRender();
    void  EndRender();
    void  ReadOut();

    void* GetRenderBuffer();

  private:
    void   PboToBuffer();
    GLuint m_pbo;
    GLuint m_query;
};

typedef CRenderCaptureGL CRenderCapture;

#elif HAS_DX /*HAS_GL*/

class CRenderCaptureDX : public CRenderCaptureBase, public ID3DResource
{
  public:
    CRenderCaptureDX();
    ~CRenderCaptureDX();

    void BeginRender();
    void EndRender();
    void ReadOut();
    
    virtual void OnDestroyDevice();
    virtual void OnLostDevice();
    virtual void OnCreateDevice() {};

  private:
    void SurfaceToBuffer();
    void CleanupDX();

    LPDIRECT3DSURFACE9 m_renderSurface;
    LPDIRECT3DSURFACE9 m_copySurface;
    LPDIRECT3DQUERY9   m_query;

    unsigned int       m_surfaceWidth;
    unsigned int       m_surfaceHeight;
};

typedef CRenderCaptureDX CRenderCapture;

#endif
