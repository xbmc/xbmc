#ifndef __XRANDR__
#define __XRANDR__

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

#ifdef HAS_XRANDR

#include "utils/StdString.h"
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
  CStdString id;
  CStdString name;
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
  CStdString name;
  bool isConnected;
  int w;
  int h;
  int x;
  int y;
  int wmm;
  int hmm;
  std::vector<XMode> modes;
  bool isRotated;
};

class CXRandR
{
public:
  CXRandR(bool query=false);
  bool Query(bool force=false);
  std::vector<XOutput> GetModes(void);
  XOutput GetCurrentOutput();
  XMode   GetCurrentMode(CStdString outputName);
  bool SetMode(XOutput output, XMode mode);
  void LoadCustomModeLinesToAllOutputs(void);
  void SaveState();
  //bool Has1080i();
  //bool Has1080p();
  //bool Has720p();
  //bool Has480p();

private:
  bool m_bInit;
  std::vector<XOutput> m_current;
  std::vector<XOutput> m_outputs;
  CStdString m_currentOutput;
  CStdString m_currentMode;
};

extern CXRandR g_xrandr;

#endif

#endif
