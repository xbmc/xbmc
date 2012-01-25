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

#ifndef HAS_DS_PLAYER
#error DSPlayer's header file included without HAS_DS_PLAYER defined
#endif

#if (defined HAVE_CONFIG_H) && (!defined WIN32)
  #include "config.h"
#endif

#include "DynamicDll.h"
#include "libsubs/ISubManager.h"
#include "ILog.h"

class CSubManager;

class DllLibSubsInterface
{
public:
  virtual ~DllLibSubsInterface() {}
  virtual bool CreateSubtitleManager(IDirect3DDevice9* d3DDev, SIZE size, ILog* logger, SSubSettings settings, ISubManager** pManager) = 0;
  virtual bool DeleteSubtitleManager(ISubManager * pManager) = 0;
};



class DllLibSubs : public DllDynamic, DllLibSubsInterface
{
  DECLARE_DLL_WRAPPER(DllLibSubs, "libsubs.dll")
  
  /** @brief Create the Subtitle Manager from libsubs.dll
   * @param[in] p1 Pointer to the Direct3D9 Device
   * @param[in] p2 Size of the current rendering area
   * @param[in] p3 Pointer to a ILog interface for loging inside the library
   * @param[in] p4 Pointer to a SSubSettings struct
   * @param[out] p5 Receive a pointer to ISubManager
   * @return A HRESULT code
   * @remark The ISubManager interface _MUST_ be released by calling DeleteSubtitleManager. Don't use delete on it
   */
  DEFINE_METHOD5(bool, CreateSubtitleManager, (IDirect3DDevice9 * p1, SIZE p2, ILog* p3, SSubSettings p4, ISubManager ** p5)) ///< Caller take ownership of ISubManager
  /** @brief Delete the subtitle manager
   * @param[in] p1 Pointer to ISubManager allocated by CreateSubtitleManager
   * @return A HRESULT code
   * @remark The ISubManager interface _MUST_ be allocated by CreateSubtitleManager
   */
  DEFINE_METHOD1(bool, DeleteSubtitleManager, (ISubManager* p1))
  
  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD(CreateSubtitleManager)
    RESOLVE_METHOD(DeleteSubtitleManager)
  END_METHOD_RESOLVE()
};


