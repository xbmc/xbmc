#pragma once

/*
 *      Copyright (C) 2011-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "guilib/Resolution.h"
#include "EGLQuirks.h"

typedef void* XBNativeDisplayType;
typedef void* XBNativeWindowType;

/*!
This class provides extra functionality on top of EGL in order to facilitate
the implementation-dependent functionality relating to creating, maintaining,
and destroying screens and displays.

Each implementation is required to implement each function, though it can
simply return false to signify that the functionality does not exist.

Internal state should be maintained by each implementation as little as possible.
If any information needs to be saved outside of the NativeWindow and NativeDisplay
for later retrieval, there is most likely a bug in the implementation, or in that
platform's EGL implementation.

Each implementation will be instantiated at runtime to see if it qualifies for use
until one is found. For this reason, each should avoid operations in its ctors and
dtors, instead using the provided Initialize() and Destroy() functions which are
only called once the implementation has been selected.
*/

class CEGLNativeType
{
public:

 /*! \brief Do NOT clean up in the destructor, use the Destroy function
    instead.

    \sa: Destroy() */
  virtual ~CEGLNativeType(){};

/*! \brief Unique identifier for this EGL implementaiton.

   It should be unique enough to set it apart from other possible implementations
   on a similar platform. */
  virtual std::string GetNativeName() const = 0;

/*! \brief A function for testing whether this implementation should be used.

  On platforms where several implementations are possible, it should provide a
  stringent test to rule out false-positives. */
  virtual bool  CheckCompatibility() = 0;

/*! \brief Initialize any local variables and/or structures here.

    This is called after the implementation has been chosen, which is why this
    should be used rather than the ctor. */
  virtual void  Initialize() = 0;

/*! \brief Destroy any local variables and/or structures here.

    This is called when the WindowSystem has been destroyed. */
  virtual void  Destroy() = 0;

/*! \brief EGL implementation quirks.

    Set any EGL oddities here so that they can be queried during the window's
    life-cycle. */
  virtual int   GetQuirks() = 0;

/*! \brief Create the EGL Native Display

    An Implementation-dependent method should be used to create a native
    display and store it in m_nativeDisplay. XBMC will terminate if this
    fails */
  virtual bool  CreateNativeDisplay() = 0;

/*! \brief Create the EGL Native Window

    An Implementation-dependent method should be used to create a native
    window and store it in m_nativeWindow. XBMC Will terminate if this fails.
    If possible, the created window should use the current display's geometry
    and allocate as needed so that it is immediately available for use.
    If not, it must be made ready by SetNativeResolution(). */
  virtual bool  CreateNativeWindow() = 0;

/*! \brief Returns the current Native Display */
  virtual bool  GetNativeDisplay(XBNativeDisplayType **nativeDisplay) const = 0;

/*! \brief Returns the current Native Window */
  virtual bool  GetNativeWindow(XBNativeWindowType **nativeWindow) const = 0;

/*! \brief Destroy the Native Window

    An Implementation-dependent method should be used to destroy the current
    Native Window */
  virtual bool  DestroyNativeWindow() = 0;

/*! \brief Destroy The Native Display

    An Implementation-dependent method should be used to destroy the current
    Native Display */
  virtual bool  DestroyNativeDisplay() = 0;

/*! \brief Return the current display's resolution

    This is completely independent of XBMC's internal resolution */
  virtual bool  GetNativeResolution(RESOLUTION_INFO *res) const = 0;

/*! \brief Set the current display's resolution

    This is completely independent of XBMC's internal resolution */
  virtual bool  SetNativeResolution(const RESOLUTION_INFO &res) = 0;

/*! \brief Query the display for all possible resolutions */
  virtual bool  ProbeResolutions(std::vector<RESOLUTION_INFO> &resolutions) = 0;

/*! \brief Provide a fall-back resolution

    If all queried resolutions fail, this one is guaranteed to be compatible
    with the display */
  virtual bool  GetPreferredResolution(RESOLUTION_INFO *res) const = 0;

/*! \brief Show/Hide the current window

    A platform-independent way of hiding XBMC (for example blanking the current
    framebuffer */
  virtual bool  ShowWindow(bool show) = 0;

protected:
  XBNativeDisplayType  m_nativeDisplay;
  XBNativeWindowType   m_nativeWindow;
};
