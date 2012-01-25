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

#define MAX_WINDOWS  3

class CGolombBuffer;

class CompositionObjectData
{
public:
  SHORT       m_object_id;
  BYTE        m_version_number;

  SHORT        m_width;
  SHORT        m_height;

  void        SetRLEData(BYTE* pBuffer, int nSize, int nTotalSize);
  void        AppendRLEData(BYTE* pBuffer, int nSize);
  int         GetRLEDataSize()  { return m_nRLEDataSize; };
  bool        IsRLEComplete() { return m_nRLEPos >= m_nRLEDataSize; };
  BYTE*       GetRLEData() { return m_pRLEData; }
  void        Copy(CompositionObjectData * pData);

  /*void        AddRef() { InterlockedIncrement(& m_increment); }
  void        Release() {
    InterlockedDecrement(& m_increment);
    if (! m_increment)
      delete this;
  }*/

  CompositionObjectData()
  {
    m_increment     = 0;
    m_object_id     = 0;
    m_pRLEData      = NULL;
    m_nRLEDataSize  = 0;
    m_nRLEPos       = 0;
    m_width         = 0;
    m_height        = 0;
  }

  ~CompositionObjectData()
  {
    delete[] m_pRLEData;
  }

private:
  BYTE*    m_pRLEData;
  int      m_nRLEDataSize;
  int      m_nRLEPos;
  volatile unsigned int m_increment;
};

class CompositionObject : Rasterizer
{
public :
  SHORT        m_object_id_ref;
  bool         m_object_cropped_flag;
  bool         m_forced_on_flag;
  BYTE         m_window_id_ref;

  SHORT        m_horizontal_position;
  SHORT        m_vertical_position;
  BYTE         m_version_number;

  SHORT        m_cropping_horizontal_position;
  SHORT        m_cropping_vertical_position;
  SHORT        m_cropping_width;
  SHORT        m_cropping_height;

  CompositionObjectData*    m_pData;

  /*WINDOW_DEFINITION   m_windows[MAX_WINDOWS];
  BYTE                m_windowsNumber;*/

  REFERENCE_TIME    m_rtStart;
  REFERENCE_TIME    m_rtStop;

  CompositionObject();
  ~CompositionObject();

  void        RenderHdmv(SubPicDesc& spd, SHORT x, SHORT y);
  void        RenderDvb(SubPicDesc& spd, SHORT nX, SHORT nY);
  void        WriteSeg (SubPicDesc& spd, SHORT nX, SHORT nY, SHORT nCount, SHORT nPaletteIndex);
  void        SetPalette (int nNbEntry, HDMV_PALETTE* pPalette, bool bIsHD);
  void        SetPalette (int nNbEntry, DWORD* dwColors);
  bool        HavePalette() { return m_nColorNumber>0; };
  void        SetObjectData(CompositionObjectData * pData)
  {
    if (m_pData)
      delete m_pData;

    if (! pData)
      return;

    m_pData = new CompositionObjectData();
    m_pData->Copy(pData);
  }
  CompositionObjectData*  GetObjectData() { return m_pData; }

private :
  int      m_nColorNumber;
  DWORD    m_Colors[256];

  void    DvbRenderField(SubPicDesc& spd, CGolombBuffer& gb, SHORT nXStart, SHORT nYStart, SHORT nLength);
  void    Dvb2PixelsCodeString(SubPicDesc& spd, CGolombBuffer& gb, SHORT& nX, SHORT& nY);
  void    Dvb4PixelsCodeString(SubPicDesc& spd, CGolombBuffer& gb, SHORT& nX, SHORT& nY);
  void    Dvb8PixelsCodeString(SubPicDesc& spd, CGolombBuffer& gb, SHORT& nX, SHORT& nY);
};
