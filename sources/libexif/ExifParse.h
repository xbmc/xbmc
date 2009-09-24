#pragma once
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
#include "libexif.h"

class CExifParse
{
  public:
    CExifParse  ();
   ~CExifParse  (void)      {}
    bool        Process     (const unsigned char* const Data, const unsigned short length, ExifInfo_t *info);
    static int  Get16       (const void* const Short, const bool motorolaOrder=true);
    static int  Get32       (const void* const Long,  const bool motorolaOrder=true);

  private:
    ExifInfo_t *m_ExifInfo;
    double      m_FocalPlaneXRes;
    double      m_FocalPlaneUnits;
    unsigned    m_LargestExifOffset;          // Last exif data referenced (to check if thumbnail is at end)
    int         m_ExifImageWidth;
    bool        m_MotorolaOrder;
    bool        m_DateFound;

//    void    LocaliseDate        (void);
//    void    GetExposureTime     (const float exposureTime);
    double  ConvertAnyFormat    (const void* const ValuePtr, int Format);
    void    ProcessDir          (const unsigned char* const DirStart,
                                 const unsigned char* const OffsetBase,
                                 const unsigned ExifLength, int NestingLevel);
    void    ProcessGpsInfo      (const unsigned char* const DirStart,
                                 int ByteCountUnused,
                                 const unsigned char* const OffsetBase,
                                 unsigned ExifLength);
    void    GetLatLong          (const unsigned int Format,
                                 const unsigned char* ValuePtr,
                                 const int ComponentSize,
                                 char *latlongString);
};

