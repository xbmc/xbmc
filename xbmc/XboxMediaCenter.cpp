/*
 *      Copyright (C) 2005-2007 Team XboxMediaCenter
 *      http://www.xboxmediacenter.com
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
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
// XboxMediaCenter
//
// libraries:
//   - CDRipX   : doesnt support section loading yet
//   - xbfilezilla : doesnt support section loading yet
//

#include "stdafx.h"
#include "Application.h"


CApplication g_application;
#ifndef _LINUX
void main()
#else
int main(int argc, char* argv[])
#endif
{
  if (argc > 1)
  {
    for (int i=1; i<argc;i++)
    {
      if (strnicmp(argv[i], "-q", 2) == 0)
        g_application.SetQuiet(true);
      
      if (strnicmp(argv[i], "-fs", 3) == 0)
      {
        printf("Running in fullscreen mode...\n");
        g_advancedSettings.m_fullScreen = true;
      }
    }
  }
  
  g_application.Create(NULL);
  while (1)
  {
    g_application.Run();
  }

#ifndef _LINUX
  return 0;
#endif
}

extern "C"
{

  void mp_msg( int x, int lev, const char *format, ... )
  {
    va_list va;
    static char tmp[2048];
    va_start(va, format);
#ifndef _LINUX
    _vsnprintf(tmp, 2048, format, va);
#else
    vsnprintf(tmp, 2048, format, va);
#endif
    va_end(va);
    tmp[2048 - 1] = 0;

    OutputDebugString(tmp);
  }
}
