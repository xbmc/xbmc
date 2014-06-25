#ifndef __XRANDR__
#define __XRANDR__

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

#include "system.h"

#ifdef HAS_XRANDR

#include <string>
#include <vector>
#include <map>

class XMode
{
public:
  XMode()
    {
      id="";
      name="";
      hz=0.0f;
      isPreferred=false;
      isCurrent=false;
      w=h=0;
    }
  bool operator==(XMode& mode) const
    {
      if (id!=mode.id)
        return false;
      if (name!=mode.name)
        return false;
      if (hz!=mode.hz)
        return false;
      if (isPreferred!=mode.isPreferred)
        return false;
      if (isCurrent!=mode.isCurrent)
        return false;
      if (w!=mode.w)
        return false;
      if (h!=mode.h)
        return false;
      return true;
    }
  std::string id;
  std::string name;
  float hz;
  bool isPreferred;
  bool isCurrent;
  unsigned int w;
  unsigned int h;
};

class XOutput
{
public:
  XOutput()
    {
      name="";
      isConnected=false;
      w=h=x=y=wmm=hmm=0;
    }
  std::string name;
  bool isConnected;
  int screen;
  int w;
  int h;
  int x;
  int y;
  int crtc;
  int wmm;
  int hmm;
  std::vector<XMode> modes;
  bool isRotated;
};

class CXRandR
{
public:
  CXRandR(bool query=false);
  bool Query(bool force=false, bool ignoreoff=true);
  bool Query(bool force, int screennum, bool ignoreoff=true);
  std::vector<XOutput> GetModes(void);
  XMode   GetCurrentMode(std::string outputName);
  XMode   GetPreferredMode(std::string outputName);
  XOutput *GetOutput(std::string outputName);
  bool SetMode(XOutput output, XMode mode);
  void LoadCustomModeLinesToAllOutputs(void);
  void SaveState();
  void SetNumScreens(unsigned int num);
  bool IsOutputConnected(std::string name);
  bool TurnOffOutput(std::string name);
  bool TurnOnOutput(std::string name);
  int GetCrtc(int x, int y);
  //bool Has1080i();
  //bool Has1080p();
  //bool Has720p();
  //bool Has480p();

private:
  bool m_bInit;
  std::vector<XOutput> m_outputs;
  std::string m_currentOutput;
  std::string m_currentMode;
  unsigned int m_numScreens;
};

extern CXRandR g_xrandr;

#endif

#endif
