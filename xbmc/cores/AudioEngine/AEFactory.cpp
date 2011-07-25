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
#include "system.h"

#include "AEFactory.h"
#ifdef __APPLE__
# include "Engines/CoreAudioAE.h"
#else
# include "Engines/SoftAE.h"
#endif
#ifdef HAS_PULSEAUDIO
# include "Engines/PulseAE.h"
#endif

IAE* CAEFactory::AE = NULL;

bool CAEFactory::LoadEngine()
{
  bool loaded = false;

#ifdef HAS_PULSEAUDIO
  if (!loaded)
    loaded = CAEFactory::LoadEngine(AE_ENGINE_PULSE);
#endif

#ifdef __APPLE__
  if (!loaded)
    loaded = CAEFactory::LoadEngine(AE_ENGINE_COREAUDIO);
#endif

  if (!loaded)
    loaded = CAEFactory::LoadEngine(AE_ENGINE_SOFT);

  return loaded;
}

bool CAEFactory::LoadEngine(enum AEEngine engine)
{
  /* can only load the engine once, XBMC restart is required to change it */
  if (AE)
    return false;

  switch(engine)
  {
#ifndef __APPLE__
    case AE_ENGINE_NULL     :
#endif
#ifdef __APPLE__
    case AE_ENGINE_COREAUDIO: AE = new CCoreAudioAE(); break;
#else
    case AE_ENGINE_SOFT     : AE = new CSoftAE(); break;
#endif
#ifdef HAS_PULSEAUDIO
    case AE_ENGINE_PULSE    : AE = new CPulseAE(); break;
#endif
  }

  if (!AE)
    return false;

  if (AE->Initialize())
    return true;

  delete AE;
  AE = NULL;
  return false;
}

