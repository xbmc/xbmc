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

#include "Boblight.h"

#ifdef HAVE_LIBBOBLIGHT_LIBBOBLIGHT_H

#define BOBLIGHT_DLOPEN
#include <libboblight/libboblight.h>

#include "Util.h"

using namespace std;

CBoblight::CBoblight()
{
  //boblight_loadlibrary returns NULL when the function pointers can be loaded
  //returns dlerror() otherwise
  char* liberror = boblight_loadlibrary(NULL);
  if (liberror) m_liberror = liberror;
  
  m_isenabled = false;
  
  Create();
}

void CBoblight::Process()
{
  //have to sort this out, can't log from constructor
  Sleep(1000);
  CLog::Log(LOGDEBUG, "CBoblight: starting");
  
  if (!m_liberror.empty())
  {
    CLog::Log(LOGDEBUG, "CBoblight: %s", m_liberror.c_str());
    return;
  }
}

#endif //HAVE_LIBBOBLIGHT_LIBBOBLIGHT_H

CBoblight g_boblight; //might make this a member of application

