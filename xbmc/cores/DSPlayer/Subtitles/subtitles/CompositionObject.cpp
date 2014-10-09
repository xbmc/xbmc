/* 
 * $Id: CompositionObject.cpp 1785 2010-04-09 14:12:59Z xhmikosr $
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

#include "stdafx.h"
#include "CompositionObject.h"
#include "../DSUtil/GolombBuffer.h"

CompositionObject::CompositionObject()
{
  m_rtStart         = 0;
  m_rtStop          = 0;
  m_nColorNumber    = 0;
  m_cropping_height = 0;
  m_cropping_width  = 0;
  m_cropping_horizontal_position = 0;
  m_cropping_vertical_position   = 0;
  m_horizontal_position          = 0;
  m_vertical_position            = 0;
  m_object_id_ref   = 0;
  m_object_cropped_flag = false;
  m_forced_on_flag      = false;
  m_version_number      = 0;
  m_pData               = NULL;
  memsetd (m_Colors, 0xFF000000, sizeof(m_Colors));
}

CompositionObject::~CompositionObject()
{
}

void CompositionObject::SetPalette (int nNbEntry, HDMV_PALETTE* pPalette, bool bIsHD)
{
  m_nColorNumber  = nNbEntry;

  for (int i=0; i<m_nColorNumber; i++)
  {
//    if (pPalette[i].T != 0)  // Prevent ugly background when Alpha=0 (but RGB different from 0)
    {
      if (bIsHD)
        m_Colors[pPalette[i].entry_id] = YCrCbToRGB_Rec709 (pPalette[i].T, pPalette[i].Y, pPalette[i].Cr, pPalette[i].Cb);
      else
        m_Colors[pPalette[i].entry_id] = YCrCbToRGB_Rec601 (pPalette[i].T, pPalette[i].Y, pPalette[i].Cr, pPalette[i].Cb);
    }
//    TRACE_HDMVSUB ("%03d : %08x\n", pPalette[i].entry_id, m_Colors[pPalette[i].entry_id]);
  }
}


void CompositionObjectData::SetRLEData(BYTE* pBuffer, int nSize, int nTotalSize)
{
  delete[] m_pRLEData;
  m_pRLEData      = DNew BYTE[nTotalSize];
  m_nRLEDataSize  = nTotalSize;
  m_nRLEPos       = nSize;

  memcpy (m_pRLEData, pBuffer, nSize);
}

void CompositionObjectData::AppendRLEData(BYTE* pBuffer, int nSize)
{
  ASSERT (m_nRLEPos+nSize <= m_nRLEDataSize);
  if (m_nRLEPos+nSize <= m_nRLEDataSize)
  {
    memcpy (m_pRLEData+m_nRLEPos, pBuffer, nSize);
    m_nRLEPos += nSize;
  }
}

void CompositionObjectData::Copy(CompositionObjectData* pData)
{
  if (! pData)
    return;

  m_nRLEDataSize    = pData->m_nRLEDataSize;
  m_nRLEPos         = pData->m_nRLEPos;
  m_object_id       = pData->m_object_id;
  m_version_number  = pData->m_version_number;

  m_width           = pData->m_width;
  m_height          = pData->m_height;

  delete[] m_pRLEData;
  m_pRLEData        = DNew BYTE[m_nRLEDataSize];
  memcpy(m_pRLEData, pData->m_pRLEData, m_nRLEDataSize);
}


void CompositionObject::RenderHdmv(SubPicDesc& spd, SHORT x, SHORT y)
{
  if (GetObjectData())
  {
    CGolombBuffer GBuffer(GetObjectData()->GetRLEData(), GetObjectData()->GetRLEDataSize());
    BYTE      bTemp;
    BYTE      bSwitch;

    BYTE      nPaletteIndex = 0;
    SHORT     nCount;
    SHORT     nX  = x;
    SHORT     nY  = y;

    while ((nY < GetObjectData()->m_height) && !GBuffer.IsEOF())
    {
      bTemp = GBuffer.ReadByte();
      if (bTemp != 0)
      {
        nPaletteIndex = bTemp;
        nCount = 1;
      }
      else
      {
        bSwitch = GBuffer.ReadByte();
        if (!(bSwitch & 0x80))
        {
          if (!(bSwitch & 0x40))
          {
            nCount = bSwitch & 0x3F;
            if (nCount > 0)
              nPaletteIndex = 0;
          }
          else
          {
            nCount = (bSwitch & 0x3F) << 8 | (SHORT)GBuffer.ReadByte();
            nPaletteIndex = 0;
          }
        }
        else
        {
          if (!(bSwitch & 0x40))
          {
            nCount = bSwitch & 0x3F;
            nPaletteIndex = GBuffer.ReadByte();
          }
          else
          {
            nCount = (bSwitch & 0x3F) << 8 | (SHORT)GBuffer.ReadByte();
            nPaletteIndex  = GBuffer.ReadByte();
          }
        }
      }

      if (nCount > 0)
      {
        if (nPaletteIndex != 0xFF)    // Fully transparent (§9.14.4.2.2.1.1)
          FillSolidRect(spd, nX, nY, nCount, 1, m_Colors[nPaletteIndex]);
        nX += nCount;
      }
      else
      {
        nY++;
        nX = x;
      }
    }
  }
}


void CompositionObject::RenderDvb(SubPicDesc& spd, SHORT nX, SHORT nY)
{
  if (GetObjectData())
  {
    CGolombBuffer  gb (GetObjectData()->GetRLEData(), GetObjectData()->GetRLEDataSize());
    SHORT      sTopFieldLength;
    SHORT      sBottomFieldLength;

    sTopFieldLength     = gb.ReadShort();
    sBottomFieldLength  = gb.ReadShort();

    DvbRenderField (spd, gb, nX, nY,   sTopFieldLength);
    DvbRenderField (spd, gb, nX, nY+1, sBottomFieldLength);
  }
}


void CompositionObject::DvbRenderField(SubPicDesc& spd, CGolombBuffer& gb, SHORT nXStart, SHORT nYStart, SHORT nLength)
{
  //FillSolidRect (spd, 0,  0, 300, 10, 0xFFFF0000);  // Red opaque
  //FillSolidRect (spd, 0, 10, 300, 10, 0xCC00FF00);  // Green 80%
  //FillSolidRect (spd, 0, 20, 300, 10, 0x100000FF);  // Blue 60%
  //return;
  SHORT  nX    = nXStart;
  SHORT  nY    = nYStart;
  INT64  nEnd  = gb.GetPos()+nLength;
  while (gb.GetPos() < nEnd)
  {
    BYTE  bType  = gb.ReadByte();
    switch (bType)
    {
    case 0x10 :
      Dvb2PixelsCodeString(spd, gb, nX, nY);
      break;
    case 0x11 :
      Dvb4PixelsCodeString(spd, gb, nX, nY);
      break;
    case 0x12 :
      Dvb8PixelsCodeString(spd, gb, nX, nY);
      break;
    case 0x20 :
      gb.SkipBytes (2);
      break;
    case 0x21 :
      gb.SkipBytes (4);
      break;
    case 0x22 :
      gb.SkipBytes (16);
      break;
    case 0xF0 :
      nX  = nXStart;
      nY += 2;
      break;
    default :
      ASSERT(FALSE);
      break;
    }
  }
}


void CompositionObject::Dvb2PixelsCodeString(SubPicDesc& spd, CGolombBuffer& gb, SHORT& nX, SHORT& nY)
{
  BYTE      bTemp;
  BYTE      nPaletteIndex = 0;
  SHORT      nCount;
  bool      bQuit  = false;

  while (!bQuit && !gb.IsEOF())
  {
    nCount      = 0;
    nPaletteIndex  = 0;
    bTemp      = (BYTE)gb.BitRead(2);
    if (bTemp != 0)
    {
      nPaletteIndex = bTemp;
      nCount      = 1;
    }
    else
    {
      if (gb.BitRead(1) == 1)                // switch_1
      {
        nCount      = 3 + (SHORT)gb.BitRead(3);    // run_length_3-9
        nPaletteIndex = (BYTE)gb.BitRead(2);
      }
      else
      {
        if (gb.BitRead(1) == 0)              // switch_2
        {
          switch (gb.BitRead(2))            // switch_3
          {
          case 0 :
            bQuit      = true;
            break;
          case 1 :
            nCount      = 2;
            break;
          case 2 :                    // if (switch_3 == '10')
            nCount      = 12 + (SHORT)gb.BitRead(4);  // run_length_12-27
            nPaletteIndex = (BYTE)gb.BitRead(2);    // 4-bit_pixel-code
            break;
          case 3 :
            nCount      = 29 + gb.ReadByte();      // run_length_29-284
            nPaletteIndex = (BYTE)gb.BitRead(2);    // 4-bit_pixel-code
            break;
          }
        }
        else
          nCount = 1;
      }
    }

    if (nX+nCount > GetObjectData()->m_width) 
    {
      ASSERT (FALSE);
      break;
    }

    if (nCount>0)
    {
      FillSolidRect (spd, nX, nY, nCount, 1, m_Colors[nPaletteIndex]);
      nX += nCount;
    }
  }

  gb.BitByteAlign();
}

void CompositionObject::Dvb4PixelsCodeString(SubPicDesc& spd, CGolombBuffer& gb, SHORT& nX, SHORT& nY)
{
  BYTE      bTemp;
  BYTE      nPaletteIndex = 0;
  SHORT      nCount;
  bool      bQuit  = false;

  while (!bQuit && !gb.IsEOF())
  {
    nCount      = 0;
    nPaletteIndex  = 0;
    bTemp      = (BYTE)gb.BitRead(4);
    if (bTemp != 0)
    {
      nPaletteIndex = bTemp;
      nCount      = 1;
    }
    else
    {
      if (gb.BitRead(1) == 0)                // switch_1
      {
        nCount = (SHORT)gb.BitRead(3);          // run_length_3-9
        if (nCount != 0)
          nCount += 2;
        else
          bQuit = true;
      }
      else
      {
        if (gb.BitRead(1) == 0)              // switch_2
        {
          nCount      = 4 + (SHORT)gb.BitRead(2);  // run_length_4-7
          nPaletteIndex = (BYTE)gb.BitRead(4);    // 4-bit_pixel-code
        }
        else
        {
          switch (gb.BitRead(2))            // switch_3
          {
          case 0 :
            nCount      = 1;
            break;
          case 1 :
            nCount      = 2;
            break;
          case 2 :                    // if (switch_3 == '10')
            nCount      = 9 + (SHORT)gb.BitRead(4);  // run_length_9-24
            nPaletteIndex = (BYTE)gb.BitRead(4);    // 4-bit_pixel-code
            break;
          case 3 :
            nCount      = 25 + gb.ReadByte();      // run_length_25-280
            nPaletteIndex = (BYTE)gb.BitRead(4);    // 4-bit_pixel-code
            break;
          }
        }
      }
    }

    if (nX+nCount > GetObjectData()->m_width) 
    {
      ASSERT (FALSE);
      break;
    }

    if (nCount>0)
    {
      FillSolidRect (spd, nX, nY, nCount, 1, m_Colors[nPaletteIndex]);
      nX += nCount;
    }
  }

  gb.BitByteAlign();
}

void CompositionObject::Dvb8PixelsCodeString(SubPicDesc& spd, CGolombBuffer& gb, SHORT& nX, SHORT& nY)
{
  BYTE      bTemp;
  BYTE      nPaletteIndex = 0;
  SHORT      nCount;
  bool      bQuit  = false;

  while (!bQuit && !gb.IsEOF())
  {
    nCount      = 0;
    nPaletteIndex  = 0;
    bTemp      = gb.ReadByte();
    if (bTemp != 0)
    {
      nPaletteIndex = bTemp;
      nCount      = 1;
    }
    else
    {
      if (gb.BitRead(1) == 0)                // switch_1
      {
        nCount = (SHORT)gb.BitRead(7);          // run_length_1-127
        if (nCount == 0)
          bQuit = true;
      }
      else
      {
        nCount      = (SHORT)gb.BitRead(7);      // run_length_3-127
        nPaletteIndex  = gb.ReadByte();
      }
    }

    if (nX+nCount > GetObjectData()->m_width) 
    {
      ASSERT (FALSE);
      break;
    }

    if (nCount>0)
    {
      FillSolidRect (spd, nX, nY, nCount, 1, m_Colors[nPaletteIndex]);
      nX += nCount;
    }
  }

  gb.BitByteAlign();
}