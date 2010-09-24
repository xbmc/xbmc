/*
 *      Copyright (C) 2005-2010 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */
#include "AEFactory.h"
#include "Engines/SoftAE.h"
#include "Engines/PulseAE.h"

IAE     *CAEFactory::m_ae       = NULL;
bool     CAEFactory::m_ready    = false;

IAE& CAEFactory::GetAE()
{
  if (m_ae)
    return *m_ae;

#ifdef HAS_PULSEAUDIO
//  m_ae = new CPulseAE();
#endif
  /* CSoftAE - this should always be the fallback */
  if (m_ae == NULL)
    m_ae = (IAE*)new CSoftAE();
  return *m_ae;
}

/*
  We cant just initialize instantly as guisettings need loading first
  CApplication will call this when its ready 
*/
bool CAEFactory::Start()
{
  m_ready = true;
  if (!AE.Initialize())
  {
    Shutdown();
    return false;
  }

  return true;
}

void CAEFactory::Shutdown()
{
  if (!m_ae)
    return;

  /* destruct the engine */
  delete m_ae;
  m_ae = NULL;
}

bool CAEFactory::Restart()
{
  Shutdown();
  GetAE();
  if (m_ready)
    return Start();

  return true;
}

