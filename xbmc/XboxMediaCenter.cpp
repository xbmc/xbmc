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


// XBMC
//
// libraries:
//   - CDRipX   : doesnt support section loading yet
//   - xbfilezilla : doesnt support section loading yet
//

#include "stdafx.h"
#include "Application.h"


CApplication g_application;
void main()
{
  g_application.Create(NULL);
  while (1)
  {
    g_application.Run();
  }
}

extern "C"
{

  void mp_msg( int x, int lev, const char *format, ... )
  {
    va_list va;
    static char tmp[2048];
    va_start(va, format);
    _vsnprintf(tmp, 2048, format, va);
    va_end(va);
    tmp[2048 - 1] = 0;

    OutputDebugString(tmp);
  }
}
