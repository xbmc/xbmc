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

#ifdef HAVE_BOBLIGHT

#define BOBLIGHT_DLOPEN
#include <libboblight/libboblight.h>

#include "Util.h"

#define SECTIMEOUT 5

using namespace std;

CBoblight::CBoblight()
{
  //boblight_loadlibrary returns NULL when the function pointers can be loaded
  //returns dlerror() otherwise
  char* liberror = boblight_loadlibrary(NULL);
  if (liberror) m_liberror = liberror;
  
  m_isenabled = false;
  m_hasinput = false;
  m_boblight = NULL;
  m_priority = 255;
  
  Create();
}

bool CBoblight::IsEnabled()
{
  CSingleLock lock(m_critsection);
  return m_isenabled;
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
  
  while(!m_bStop)
  {
    if (!Setup())
    {
      Cleanup();
      Sleep(SECTIMEOUT * 1000);
      continue;
    }
    
    Run();
    Cleanup();
  }
}

bool CBoblight::Setup()
{
  m_boblight = boblight_init();
  
  if (!boblight_connect(m_boblight, NULL, -1, SECTIMEOUT * 1000000))
  {
    CLog::Log(LOGDEBUG, "CBoblight: %s", boblight_geterror(m_boblight));
    return false;
  }
  
  CLog::Log(LOGDEBUG, "CBoblight: Connected");
  
  CSingleLock lock(m_critsection);
  m_isenabled = true;
  
  return true;
}

void CBoblight::Cleanup()
{
  if (m_boblight)
  {
    boblight_destroy(m_boblight);
    m_boblight = NULL;
  }
  
  CSingleLock lock(m_critsection);
  m_isenabled = false;
}

void CBoblight::Run()
{
  while(!m_bStop)
  {
    CSingleLock lock(m_critsection);
    if (m_hasinput)
    {
      //do stuff
    }
    else
    {
      lock.Leave();
      
      if (m_priority != 255)
      {
        boblight_setpriority(m_boblight, 255);
        m_priority = 255;
      }
      if (!boblight_ping(m_boblight))
      {
        CLog::Log(LOGDEBUG, "CBoblight: %s", boblight_geterror(m_boblight));
        return;
      }
    }
    
    m_inputevent.WaitMSec(SECTIMEOUT * 1000);
  }
}


CBoblight g_boblight; //might make this a member of application

#endif //HAVE_BOBLIGHT
