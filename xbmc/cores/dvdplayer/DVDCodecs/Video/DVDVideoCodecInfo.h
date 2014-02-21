/*
 *      Copyright (C) 2010-2013 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#ifndef DVDVIDEOCODECINFO_H
#define DVDVIDEOCODECINFO_H

class CDVDVideoCodecBuffer
{
public:
  // reference counting
  virtual void                Lock() = 0;
  virtual long                Release() = 0;
  virtual bool                IsValid() = 0;

  uint32_t            iWidth;
  uint32_t            iHeight;
  uint8_t*            data[4];      // [4] = alpha channel, currently not used
  int                 iLineSize[4];   // [4] = alpha channel, currently not used
};

#endif // DVDVIDEOCODECINFO_H
