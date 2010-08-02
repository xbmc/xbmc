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

#ifndef AEFILTER_H
#define AEFILTER_H

/*
  Implements a Kaiser-Bessel Window FIR Filter
  http://arc.id.au/FilterDesign.html
*/

#include "FilterButterworth24db.h"

class CAEFilter
{
private:
  static float Ino(float x);
public:
  typedef struct
  {
    bool lpf;
    CFilterButterworth24db f;
    float v, s;
  } FInfo;

  static FInfo* InitializeLPF(unsigned int cutoff, unsigned int sampleRate);
  static FInfo* InitializeHPF(unsigned int cutoff, unsigned int sampleRate);
  static void   Filter(FInfo *info, float *data, unsigned int samples);
};

#endif
