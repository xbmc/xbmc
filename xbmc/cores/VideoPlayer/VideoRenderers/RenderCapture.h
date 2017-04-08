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

#pragma once

#include "system.h" //HAS_DX, HAS_GL, HAS_GLES, opengl headers, direct3d headers

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

class CRenderCaptureBase
{
  public:
    CRenderCaptureBase();
    ~CRenderCaptureBase();

    /* \brief Called by the rendermanager to set the state, should not be called by anything else */
    void SetState(ECAPTURESTATE state) { m_state = state; }

    /* \brief Called by the rendermanager to get the state, should not be called by anything else */
    ECAPTURESTATE GetState() { return m_state;}

    /* \brief Called by the rendermanager to set the userstate, should not be called by anything else */
    void SetUserState(ECAPTURESTATE state) { m_userState = state; }

    /* \brief Called by the code requesting the capture
       \return CAPTURESTATE_WORKING when the capture is in progress,
       CAPTURESTATE_DONE when the capture has succeeded,
       CAPTURESTATE_FAILED when the capture has failed
    */
    ECAPTURESTATE GetUserState() { return m_userState; }

    /* \brief The internal event will be set when the rendermanager has captured and read a videoframe, or when it has failed
       \return A reference to m_event
    */
    CEvent& GetEvent() { return m_event; }

    /* \brief Called by the rendermanager to set the flags, should not be called by anything else */
    void SetFlags(int flags) { m_flags = flags; }

    /* \brief Called by the rendermanager to get the flags, should not be called by anything else */
    int GetFlags() { return m_flags; }

    /* \brief Called by the rendermanager to set the width, should not be called by anything else */
    void  SetWidth(unsigned int width) { m_width = width; }

    /* \brief Called by the rendermanager to set the height, should not be called by anything else */
    void SetHeight(unsigned int height) { m_height = height; }

    /* \brief Called by the code requesting the capture to get the width */
    unsigned int GetWidth() { return m_width; }

    /* \brief Called by the code requesting the capture to get the height */
    unsigned int GetHeight() { return m_height; }

    /* \brief Called by the code requesting the capture to get the buffer where the videoframe is stored,
       the format is BGRA, this buffer is only valid when GetUserState returns CAPTURESTATE_DONE.
       The size of the buffer is GetWidth() * GetHeight() * 4.
    */
    uint8_t*  GetPixels() const { return m_pixels; }

    /* \brief Called by the rendermanager to know if the capture is readout async (using dma for example),
       should not be called by anything else.
    */
    bool  IsAsync() { return m_asyncSupported; }

  protected:
    bool UseOcclusionQuery();

    ECAPTURESTATE  m_state;     //state for the rendermanager
    ECAPTURESTATE  m_userState; //state for the thread that wants the capture
    int m_flags;
    CEvent m_event;

    uint8_t*  m_pixels;
    unsigned int m_width;
    unsigned int m_height;
    unsigned int m_bufferSize;

    //this is set after the first render
    bool m_asyncSupported;
    bool m_asyncChecked;
};


#if defined(HAS_IMXVPU)
#include "../VideoPlayer/DVDCodecs/Video/DVDVideoCodecIMX.h"

class CRenderCaptureIMX : public CRenderCaptureBase
{
  public:
    CRenderCaptureIMX();
    ~CRenderCaptureIMX();

    int   GetCaptureFormat();

    void  BeginRender();
    void  EndRender();
    void  ReadOut();

    void* GetRenderBuffer();
};

class CRenderCapture : public CRenderCaptureIMX
{
  public:
    CRenderCapture() {};
};


#elif defined(TARGET_RASPBERRY_PI)
#include "xbmc/linux/RBP.h"

class CRenderCaptureDispmanX : public CRenderCaptureBase
{
  public:
    CRenderCaptureDispmanX();
    ~CRenderCaptureDispmanX();

    int   GetCaptureFormat();

    void  BeginRender();
    void  EndRender();
    void  ReadOut();

    void* GetRenderBuffer();
};

//used instead of typedef CRenderCaptureGL CRenderCapture
//since C++ doesn't allow you to forward declare a typedef
class CRenderCapture : public CRenderCaptureDispmanX
{
  public:
    CRenderCapture() {};
};

#elif defined(HAS_GL) || defined(HAS_GLES)
#include "system_gl.h"

class CRenderCaptureGL : public CRenderCaptureBase
{
  public:
    CRenderCaptureGL();
    ~CRenderCaptureGL();

    int   GetCaptureFormat();

    void  BeginRender();
    void  EndRender();
    void  ReadOut();

    void* GetRenderBuffer();

  private:
    void   PboToBuffer();
    GLuint m_pbo;
    GLuint m_query;
    bool   m_occlusionQuerySupported;
};

//used instead of typedef CRenderCaptureGL CRenderCapture
//since C++ doesn't allow you to forward declare a typedef
class CRenderCapture : public CRenderCaptureGL
{
  public:
    CRenderCapture() {};
};

#elif HAS_DX /*HAS_GL*/

class CRenderCaptureDX : public CRenderCaptureBase, public ID3DResource
{
  public:
    CRenderCaptureDX();
    ~CRenderCaptureDX();

    int  GetCaptureFormat();

    void BeginRender();
    void EndRender();
    void ReadOut();
    
    virtual void OnDestroyDevice(bool fatal);
    virtual void OnLostDevice();
    virtual void OnCreateDevice() {};

  private:
    void SurfaceToBuffer();
    void CleanupDX();

    ID3D11Texture2D*        m_renderTexture;
    ID3D11RenderTargetView* m_renderSurface;
    ID3D11Texture2D*        m_copySurface;
    ID3D11Query*            m_query;
    unsigned int            m_surfaceWidth;
    unsigned int            m_surfaceHeight;
};

class CRenderCapture : public CRenderCaptureDX
{
  public:
    CRenderCapture() {};
};

#endif
