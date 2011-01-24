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

#include "PCMAmplifier.h"
#include "settings/Settings.h"

#include <math.h>

CPCMAmplifier::CPCMAmplifier() : m_nVolume(VOLUME_MAXIMUM), m_dFactor(0)
{
}

CPCMAmplifier::~CPCMAmplifier()
{
}

void CPCMAmplifier::SetVolume(int nVolume)
{
  m_nVolume = nVolume;
  if (nVolume > VOLUME_MAXIMUM)
    nVolume = VOLUME_MAXIMUM;

  if (nVolume < VOLUME_MINIMUM)
    nVolume = VOLUME_MINIMUM;

  if( nVolume == VOLUME_MINIMUM)
    m_dFactor = 0;
  else
    m_dFactor = pow(10,nVolume/2000.f);
}

int  CPCMAmplifier::GetVolume()
{
  return m_nVolume;
}

     // only works on 16bit samples
void CPCMAmplifier::DeAmplify(short *pcm, int nSamples)
{
  if (m_dFactor >= 1.0)
  {
    // no process required. using >= to make sure no amp is ever done (only de-amp)
    return;
  }

  for (int nSample=0; nSample<nSamples; nSample++)
  {
    int nSampleValue = pcm[nSample]; // must be int. so that we can check over/under flow
    nSampleValue = (int)((double)nSampleValue * m_dFactor);

    pcm[nSample] = (short)nSampleValue;
  }
}
