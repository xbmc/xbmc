/* 
 * $Id: CompositionObject.h 1785 2010-04-09 14:12:59Z xhmikosr $
 *
 * (C) 2006-2010 see AUTHORS
 *
 * This file is part of mplayerc.
 *
 * Mplayerc is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * Mplayerc is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */


#pragma once

#include "Rasterizer.h"


struct HDMV_PALETTE
{
  BYTE    entry_id;
  BYTE    Y;
  BYTE    Cr;
  BYTE    Cb;
  BYTE    T;    // HDMV rule : 0 transparent, 255 opaque (compatible DirectX)
};

class CGolombBuffer;

class CompositionObject : Rasterizer
{
public :
  SHORT        m_object_id_ref;
  BYTE        m_window_id_ref;
  bool        m_object_cropped_flag;
  bool        m_forced_on_flag;
  BYTE        m_version_number;

  SHORT        m_horizontal_position;
  SHORT        m_vertical_position;
  SHORT        m_width;
  SHORT        m_height;

  SHORT        m_cropping_horizontal_position;
  SHORT        m_cropping_vertical_position;
  SHORT        m_cropping_width;
  SHORT        m_cropping_height;

  REFERENCE_TIME    m_rtStart;
  REFERENCE_TIME    m_rtStop;

  CompositionObject();
  ~CompositionObject();

  void        SetRLEData(BYTE* pBuffer, int nSize, int nTotalSize);
  void        AppendRLEData(BYTE* pBuffer, int nSize);
  int         GetRLEDataSize()  { return m_nRLEDataSize; };
  bool        IsRLEComplete() { return m_nRLEPos >= m_nRLEDataSize; };
  void        RenderHdmv(SubPicDesc& spd);
  void        RenderDvb(SubPicDesc& spd, SHORT nX, SHORT nY);
  void        WriteSeg (SubPicDesc& spd, SHORT nX, SHORT nY, SHORT nCount, SHORT nPaletteIndex);
  void        SetPalette (int nNbEntry, HDMV_PALETTE* pPalette, bool bIsHD);
  void        SetPalette (int nNbEntry, DWORD* dwColors);
  bool        HavePalette() { return m_nColorNumber>0; };

private :
  BYTE*    m_pRLEData;
  int      m_nRLEDataSize;
  int      m_nRLEPos;
  int      m_nColorNumber;
  DWORD    m_Colors[256];

  void    DvbRenderField(SubPicDesc& spd, CGolombBuffer& gb, SHORT nXStart, SHORT nYStart, SHORT nLength);
  void    Dvb2PixelsCodeString(SubPicDesc& spd, CGolombBuffer& gb, SHORT& nX, SHORT& nY);
  void    Dvb4PixelsCodeString(SubPicDesc& spd, CGolombBuffer& gb, SHORT& nX, SHORT& nY);
  void    Dvb8PixelsCodeString(SubPicDesc& spd, CGolombBuffer& gb, SHORT& nX, SHORT& nY);
};
