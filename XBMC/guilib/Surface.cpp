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

/*!
\file Surface.cpp
\brief
*/
#include "include.h"
#include "Surface.h"
#ifdef __APPLE__
#include "CocoaInterface.h"
#endif
#include <string>
#include "Settings.h"

using namespace Surface;

CSurface::CSurface(CSurface* src) 
{
  
}

CSurface::CSurface(int width, int height, bool doublebuffer, CSurface* shared,
                   CSurface* window, CBaseTexture* parent, bool fullscreen,
                   bool pixmap, bool pbuffer, int antialias)
{
  CLog::Log(LOGDEBUG, "Constructing surface %dx%d, shared=%p, fullscreen=%d\n", width, height, (void *)shared, fullscreen);
  m_bOK = false;
  m_iWidth = width;
  m_iHeight = height;
  m_bDoublebuffer = doublebuffer;
  m_iRedSize = 8;
  m_iGreenSize = 8;
  m_iBlueSize = 8;
  m_iAlphaSize = 8;
  m_bFullscreen = fullscreen;
  m_pShared = shared;
  m_bVSync = false;
  m_iVSyncMode = 0;
  m_iSwapStamp = 0;
  m_iSwapTime = 0;
  m_iSwapRate = 0;
  m_bVsyncInit = false;

#ifdef _WIN32
  if ( !g_advancedSettings.m_alwaysOnTop )
    m_iOnTop = ONTOP_AUTO;
  else
    m_iOnTop = ONTOP_ALWAYS;

  timeBeginPeriod(1);
#endif
}

CSurface::~CSurface()
{
#ifdef _WIN32
  timeEndPeriod(1);
#endif
}

// function should return the timestamp
// of the frame where a call to flip,
// earliest can land upon.
DWORD CSurface::GetNextSwap()
{
  if (m_iVSyncMode && m_iSwapRate != 0)
  {
    __int64 curr, freq;
    QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
    QueryPerformanceCounter((LARGE_INTEGER*)&curr);
    DWORD timestamp = timeGetTime();

    curr  -= m_iSwapStamp;
    curr  %= m_iSwapRate;
    curr  -= m_iSwapRate;
    return timestamp - (int)(1000 * curr / freq);
  }
  return timeGetTime();
}

#ifdef _WIN32
void CSurface::SetOnTop(ONTOP onTop)
{
  m_iOnTop = onTop;
}

bool CSurface::IsOnTop() {
  CStdString strRenderVendor = g_graphicsContext.GetRenderVendor();

  return m_iOnTop == ONTOP_ALWAYS || (m_iOnTop == ONTOP_FULLSCREEN && m_bCoversScreen) ||
       (m_iOnTop == ONTOP_AUTO && m_bCoversScreen && (strRenderVendor.find("Intel") != strRenderVendor.npos));
}
#endif

void CSurface::NotifyAppFocusChange(bool bGaining)
{
  /* Notification from the Application that we are either becoming the foreground window or are losing focus */
#ifdef _WIN32
  /* Remove our TOPMOST status when we lose focus.  TOPMOST seems to be required for Intel vsync, it isn't
     like I actually want to be on top. Seems like a lot of work. */

  if (m_iOnTop != ONTOP_ALWAYS && IsOnTop())
  {
    CLog::Log(LOGDEBUG, "NotifyAppFocusChange: bGaining=%d, m_bCoverScreen=%d", bGaining, m_bCoversScreen);

    HWND hwnd = (HWND)GetRenderWindow();
    {
      if (bGaining)
      {
        // For intel IGPs Vsync won't start working after we've lost focus and then regained it,
        // to kick-start it we hide then re-show the window; it's gonna look a bit ugly but it
        // seems to be the only option
        /*
        if (s_RenderVendor.find("Intel") != s_RenderVendor.npos)
        {
          CLog::Log(LOGDEBUG, "NotifyAppFocusChange: hiding XBMC window (to workaround Intel Vsync bug)");
          SetWindowPos(hwnd, 0, 0, 0, 0, 0, SWP_HIDEWINDOW | SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOSENDCHANGING);
        }
        */
        CLog::Log(LOGDEBUG, "NotifyAppFocusChange: showing XBMC window TOPMOST");
        SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOSENDCHANGING);
        SetForegroundWindow(hwnd);
        LockSetForegroundWindow(LSFW_LOCK);
      }
      else
      {
        /*
         * We can't just SetWindowPos(hwnd, hNewFore, ....) because (at least) on Vista focus may be lost to the
         * Alt-Tab window-list which is also TOPMOST, so going just below that in the z-order will leave us still
         * TOPMOST and so probably still above the window you eventually select from the window-list.
         * Checking whether hNewFore is TOPMOST and, if so, making ourselves just NOTOPMOST is almost good enough;
         * we should be NOTOPMOST when you select a window from the window-list and that will be raised above us.
         * However, if the timing is just right (i.e. wrong) then we lose focus to the window-list, see that it's
         * another TOPMOST window, at which time focus leaves the window-list and goes to the selected window
         * and then make ourselves NOTOPMOST and so raise ourselves above it.
         * We could just go straight to the bottom of the z-order but that'll mean we disappear as soon as the
         * window-list appears.
         * So, the best I can come up with is to go NOTOPMOST asap and then if hNewFore is NOTOPMOST go below that,
         * this should minimize the timespan in which the above timing issue can happen.
         */
        SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        CLog::Log(LOGDEBUG, "NotifyAppFocusChange: making XBMC window NOTOPMOST");

        HWND hNewFore = GetForegroundWindow();
        LONG newStyle = GetWindowLong(hNewFore, GWL_EXSTYLE);
        if (!(newStyle & WS_EX_TOPMOST))
        {
          CLog::Log(LOGDEBUG, "NotifyAppFocusChange: lowering XBMC window beneath new foreground window");
          SetWindowPos(hwnd, hNewFore, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        }

        LockSetForegroundWindow(LSFW_UNLOCK);
      }
    }
  }
#endif
}

