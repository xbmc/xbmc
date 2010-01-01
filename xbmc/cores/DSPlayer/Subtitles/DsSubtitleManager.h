#ifndef DSSUBTITLEMANAGER_H
#define DSSUBTITLEMANAGER_H

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


#include "streams.h"
#include "DllLibMpcSubs.h"
class CDsSubManager
{
public:
  CDsSubManager();
  bool Load();
  bool LoadSubtitles(const char* fn, IGraphBuilder* pGB, const char* paths);
  void EnableSubtitle(bool enable);
  bool Enabled() {return m_bCurrentlyEnabled;};
  void Render(int x, int y, int width, int height);
protected:
  DllLibMpcSubs m_dllMpcSubs;
  bool m_bCurrentlyEnabled;
};

extern CDsSubManager g_dllMpcSubs;



#endif