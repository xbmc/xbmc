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
#include "MathUtils.h"

#ifdef HAVE_BOBLIGHT

#define BOBLIGHT_DLOPEN
#include <libboblight/libboblight.h>

#include "Util.h"

#define SECTIMEOUT 5

using namespace std;

CBoblightClient::CBoblightClient() : m_texture(64, 64, 32)
{
  //boblight_loadlibrary returns NULL when the function pointers can be loaded
  //returns dlerror() otherwise
  char* liberror = boblight_loadlibrary(NULL);
  if (liberror)
    m_liberror = liberror;
    
  m_speed = 100.0;
  m_interpolation = false;
  m_saturation = 1.0;
  m_value = 1.0;
  m_threshold = 0;
    
  m_connected = false;
  m_hasinput = false;
  m_boblight = NULL;
  m_priority = 255;
    
  Create();
}

bool CBoblightClient::Connected()
{
  CSingleLock lock(m_critsection);
  return m_connected;
}

void CBoblightClient::GrabImage()
{
  CSingleLock lock(m_critsection);
  
  if (!m_connected)
    return;
  
  m_hasinput = true;
  
  //get a thumbnail of 64x64 pixels
  g_renderManager.CreateThumbnail(&m_texture, 64, 64);
}

void CBoblightClient::Send()
{
  //tell boblight thread to send input to boblightd
  if (m_hasinput)
    m_inputevent.Set();
}

void CBoblightClient::Disable()
{
  CSingleLock lock(m_critsection);
  if (m_hasinput)
  {
    m_hasinput = false;
    m_inputevent.Set();
  }
}

void CBoblightClient::Process()
{
  //have to sort this out, can't log from constructor
  Sleep(1000);
  CLog::Log(LOGDEBUG, "CBoblightClient: starting");
  
  //this means boblight_loadlibrary had an error
  if (!m_liberror.empty())
  {
    CLog::Log(LOGDEBUG, "CBoblightClient: %s", m_liberror.c_str());
    return;
  }
  
  //keep trying to connect to boblightd, useful in case it restarts
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

bool CBoblightClient::Setup()
{
  m_boblight = boblight_init();
  
  if (!boblight_connect(m_boblight, NULL, -1, SECTIMEOUT * 1000000))
  {
    CLog::Log(LOGDEBUG, "CBoblightClient: %s", boblight_geterror(m_boblight));
    return false;
  }
  
  CLog::Log(LOGDEBUG, "CBoblightClient: Connected");
  
  //these will be made into gui options
  boblight_setscanrange(m_boblight, 64, 64);
  //boblight_setoption(m_boblight, -1, "interpolation 1");
  //boblight_setoption(m_boblight, -1, "value 10.0");
  boblight_setoption(m_boblight, -1, "saturation 5.0");
  //boblight_setoption(m_boblight, -1, "speed 5.0");
  //boblight_setoption(m_boblight, -1, "threshold 20");*/
  
  CSingleLock lock(m_critsection);
  m_connected = true;
  
  return true;
}

void CBoblightClient::Cleanup()
{
  if (m_boblight)
  {
    boblight_destroy(m_boblight);
    m_boblight = NULL;
  }
  
  CSingleLock lock(m_critsection);
  m_connected = false;
  m_priority = 255;
}

void CBoblightClient::Run()
{
  while(!m_bStop)
  {
    CSingleLock lock(m_critsection);
    //if we have input, run it through boblight_addpixelxy() and call boblight_sendrgb()
    if (m_hasinput)
    {
      //a priority of 255 means we're an inactive boblightd client
      if (m_priority == 255)
      {
        lock.Leave();
        boblight_setpriority(m_boblight, 128);
        m_priority = 128;
        lock.Enter();
      }
            
      SetOptions();
      
      int            rgb[3];
      unsigned int   pitch  = m_texture.GetPitch();
      unsigned int   bpp    = m_texture.GetBPP();
      unsigned char* pixels = m_texture.GetPixels();
      
      for (unsigned int y = 0; y < m_texture.GetWidth(); y++)
      {
        for (unsigned int x = 0; x < m_texture.GetHeight(); x++)
        {
          rgb[0] = pixels[y * pitch + x * bpp + 2];
          rgb[1] = pixels[y * pitch + x * bpp + 1];
          rgb[2] = pixels[y * pitch + x * bpp + 0];
          
          boblight_addpixelxy(m_boblight, x, y, rgb);
        }
      }
      lock.Leave();
      
      //if it fails we have a problem, probably because boblightd died
      if (!boblight_sendrgb(m_boblight))
      {
        CLog::Log(LOGDEBUG, "CBoblightClient: %s", boblight_geterror(m_boblight));
        return;
      }
    }
    else
    {
      lock.Leave();
      //set our priority to 255 which means we're inactive
      if (m_priority != 255)
      {
        boblight_setpriority(m_boblight, 255);
        m_priority = 255;
      }
      //check if the daemon is still alive
      if (!boblight_ping(m_boblight))
      {
        CLog::Log(LOGDEBUG, "CBoblightClient: %s", boblight_geterror(m_boblight));
        return;
      }
    }
    
    //wait for input from application thread
    m_inputevent.WaitMSec(SECTIMEOUT * 1000);
    m_inputevent.Reset();
  }
}

void CBoblightClient::SetOptions()
{
  CStdString option;
  
  option.Format("speed %f", m_speed);
  boblight_setoption(m_boblight, -1, option.c_str());

  option.Format("interpolation %i", m_interpolation ? 1 : 0);
  boblight_setoption(m_boblight, -1, option.c_str());

  option.Format("saturation %f", m_saturation);
  boblight_setoption(m_boblight, -1, option.c_str());

  option.Format("value %f", m_value);
  boblight_setoption(m_boblight, -1, option.c_str());

  option.Format("threshold %i", MathUtils::round_int(m_threshold));
  boblight_setoption(m_boblight, -1, option.c_str());
}

CBoblightClient g_boblight; //might make this a member of application

#endif //HAVE_BOBLIGHT
