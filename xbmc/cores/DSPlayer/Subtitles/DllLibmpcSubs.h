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

#if (defined HAVE_CONFIG_H) && (!defined WIN32)
  #include "config.h"
#endif
extern "C" {
  #include "libmpcSubs.h"
}
#include "DynamicDll.h"
#include "utils/log.h"


class DllLibMpcSubsInterface
{
public:
  virtual ~DllLibMpcSubsInterface() {}
  virtual void SetDefaultStyle (SubtitleStyle_t * style, BOOL overrideUserStyles)=0;
  virtual void SetAdvancedOptions(int subPicsBufferAhead, SIZE textureSize, BOOL pow2tex, BOOL disableAnim)=0;
//load subtitles for video file fn, with given (rendered) graph 
  virtual BOOL LoadSubtitles(IDirect3DDevice9* d3DDev, SIZE size, const wchar_t* fn, IGraphBuilder* pGB, const wchar_t* paths)=0;
//set sample time (set from EVR presenter, not used in case of vmr9)
  virtual void SetTime(REFERENCE_TIME nsSampleTime)=0;
//render subtitles
  virtual void Render(int x, int y, int width, int height)=0;
//save subtitles
  virtual BOOL IsModified()=0; //timings were modified
  virtual void SaveToDisk()=0;
//subs management functions
  virtual int GetCount()=0; //total subtitles
  virtual BSTR GetLanguage(int i)=0; //i  range from 0 to GetCount()-1
  virtual int GetCurrent()=0;
  virtual void SetCurrent(int i)=0;
  virtual BOOL GetEnable()=0;
  virtual void SetEnable(BOOL enable)=0;
  virtual int GetDelay()=0; //in milliseconds
  virtual void SetDelay(int delay_ms)=0; //in milliseconds
  virtual void FreeSubtitles()=0;
};



class DllLibMpcSubs : public DllDynamic, DllLibMpcSubsInterface
{
  DECLARE_DLL_WRAPPER(DllLibMpcSubs, "mpcSubs.dll")
  
  DEFINE_METHOD0(void,FreeSubtitles)
  DEFINE_METHOD0(int,GetCount)
  DEFINE_METHOD0(int,GetCurrent)
  DEFINE_METHOD0(int,GetDelay)
  DEFINE_METHOD0(BOOL,GetEnable)
  DEFINE_METHOD1(BSTR,GetLanguage,(int p1))
  DEFINE_METHOD0(BOOL,IsModified)
  DEFINE_METHOD5(BOOL,LoadSubtitles,(IDirect3DDevice9 * p1, SIZE p2, const wchar_t * p3, IGraphBuilder * p4, const wchar_t * p5))
  DEFINE_METHOD4(void,Render,(int p1, int p2, int p3, int p4))
  DEFINE_METHOD0(void,SaveToDisk)
  DEFINE_METHOD4(void,SetAdvancedOptions,(int p1, SIZE p2, BOOL p3, BOOL p4))
  DEFINE_METHOD1(void,SetCurrent,(int p1) )
  DEFINE_METHOD2(void,SetDefaultStyle, (SubtitleStyle_t * p1, BOOL p2))
  DEFINE_METHOD1(void,SetDelay, (int p1))
  DEFINE_METHOD1(void,SetEnable, (BOOL p1))
  DEFINE_METHOD1(void,SetTime, (REFERENCE_TIME p1))
  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD(FreeSubtitles)
    RESOLVE_METHOD(GetCount)
    RESOLVE_METHOD(GetCurrent)
    RESOLVE_METHOD(GetDelay)
    RESOLVE_METHOD(GetEnable)
    RESOLVE_METHOD(GetLanguage)
    RESOLVE_METHOD(IsModified)
    RESOLVE_METHOD(LoadSubtitles)
    RESOLVE_METHOD(Render)
    RESOLVE_METHOD(SaveToDisk)
    RESOLVE_METHOD(SetAdvancedOptions)
    RESOLVE_METHOD(SetCurrent)
    RESOLVE_METHOD(SetDefaultStyle)
    RESOLVE_METHOD(SetDelay)
    RESOLVE_METHOD(SetEnable)
    RESOLVE_METHOD(SetTime)
  END_METHOD_RESOLVE()
};


