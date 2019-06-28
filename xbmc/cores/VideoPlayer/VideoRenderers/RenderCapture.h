/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#ifdef HAS_DX
  #include "guilib/D3DResource.h"
  #include <wrl/client.h>
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
    virtual ~CRenderCaptureBase();

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

#if defined(HAS_GL) || defined(HAS_GLES)
#include "system_gl.h"

class CRenderCaptureGL : public CRenderCaptureBase
{
  public:
    CRenderCaptureGL();
    ~CRenderCaptureGL() override;

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
    CRenderCapture() = default;
};

#elif HAS_DX /*HAS_GL*/

class CRenderCaptureDX : public CRenderCaptureBase, public ID3DResource
{
  public:
    CRenderCaptureDX();
    ~CRenderCaptureDX();

    int GetCaptureFormat();

    void BeginRender();
    void EndRender();
    void ReadOut();

    void OnDestroyDevice(bool fatal) override;
    void OnCreateDevice() override {};
    CD3DTexture& GetTarget() { return m_renderTex; }

  private:
    void SurfaceToBuffer();
    void CleanupDX();

    unsigned int m_surfaceWidth;
    unsigned int m_surfaceHeight;
    Microsoft::WRL::ComPtr<ID3D11Query> m_query;
    CD3DTexture m_renderTex;
    CD3DTexture m_copyTex;
};

class CRenderCapture : public CRenderCaptureDX
{
  public:
    CRenderCapture() {};
};

#endif
