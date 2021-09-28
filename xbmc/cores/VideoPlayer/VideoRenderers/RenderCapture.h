/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

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

class CRenderCapture
{
  public:
    CRenderCapture() = default;
    virtual ~CRenderCapture() = default;

    virtual void BeginRender() = 0;
    virtual void EndRender() = 0;

    virtual void ReadOut() {}
    virtual void* GetRenderBuffer() { return m_pixels; }

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

    ECAPTURESTATE m_state{CAPTURESTATE_FAILED}; //state for the rendermanager
    ECAPTURESTATE m_userState{CAPTURESTATE_FAILED}; //state for the thread that wants the capture
    int m_flags{0};
    CEvent m_event;

    uint8_t* m_pixels{nullptr};
    unsigned int m_width{0};
    unsigned int m_height{0};
    unsigned int m_bufferSize{0};

    // this is set after the first render
    bool m_asyncSupported{false};
    bool m_asyncChecked{false};
};

