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

#include "AEFilter.h"
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

float CAEFilter::Ino(float x)
{
  float x2 = x*x;

  float d  = 0;
  float ds = 1;
  float s  = 1;

  do
  {
    d  += 2;
    ds *= x2 / (d*d);
    s  += ds;
  }
  while(ds > s * 1e-6);

  return s;
}

CAEFilter::FInfo* CAEFilter::InitializeLPF(unsigned int cutoff, unsigned int sampleRate)
{
  FInfo *info = new FInfo();
  info->lpf   = true;
  info->f.SetSampleRate(sampleRate);
  info->f.Set(cutoff, 0);
  return info;
}

CAEFilter::FInfo* CAEFilter::InitializeHPF(unsigned int cutoff, unsigned int sampleRate)
{
  FInfo *info = new FInfo();
  info->lpf   = false;
  info->v     = (1.0 / ((float)sampleRate / 2.0)) * cutoff;
  info->s     = 0.0f;
  return info;
}

void CAEFilter::Filter(FInfo *info, float *data, unsigned int samples)
{
  if (info->lpf)
  {
    *data = info->f.Run(*data);
    return;
  }

  float t = 0;
  t        = info->v * (*data - info->s);
  info->s += 2.0 * t;
  *data = *data + t - info->s;
}

