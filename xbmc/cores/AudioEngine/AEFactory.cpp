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

bool CAEFactory::LoadEngine(enum AEEngine engine)
{
  switch(engine)
  {
#ifndef __APPLE__
    case AE_ENGINE_NULL : return AE.SetEngine(NULL);
#endif
      
#ifdef __APPLE__
    case AE_ENGINE_COREAUDIO : return AE.SetEngine(new CCoreAudioAE ());
#else
    case AE_ENGINE_SOFT : return AE.SetEngine(new CSoftAE ());
#endif

#ifdef HAS_PULSEAUDIO
    case AE_ENGINE_PULSE: return AE.SetEngine(new CPulseAE());
#endif
  }

  return false;
}

