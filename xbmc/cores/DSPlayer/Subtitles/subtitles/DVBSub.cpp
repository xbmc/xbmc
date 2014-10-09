/* 
 * $Id: DVBSub.cpp 1785 2010-04-09 14:12:59Z xhmikosr $
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
#include "DVBSub.h"
#include "../DSUtil/GolombBuffer.h"

#if (1)    // Set to 1 to activate DVB subtitles traces
  #define TRACE_DVB    TRACE
#else
  #define TRACE_DVB
#endif

#define BUFFER_CHUNK_GROW    0x1000

CDVBSub::CDVBSub(void)
     : CBaseSub(ST_DVB)
{
  m_nBufferReadPos   = 0;
  m_nBufferWritePos  = 0;
  m_nBufferSize      = 0;
  m_pBuffer          = NULL;
}

CDVBSub::~CDVBSub(void)
{
  Reset();
  delete m_pBuffer;
  m_pBuffer = NULL;
}

CDVBSub::DVB_PAGE* CDVBSub::FindPage(REFERENCE_TIME rt)
{
  for (std::list<DVB_PAGE*>::iterator it = m_Pages.begin();
      it != m_Pages.end(); it++)
  {
    DVB_PAGE* pPage = (*it);

    if (rt >= pPage->rtStart && rt < pPage->rtStop)
      return pPage;
  }

  return NULL;
}

CDVBSub::DVB_REGION* CDVBSub::FindRegion(DVB_PAGE* pPage, BYTE bRegionId)
{
  if (pPage != NULL)
  {
    for (int i=0; i<pPage->RegionCount; i++)
    {
      if (pPage->Regions[i].Id == bRegionId)
        return &pPage->Regions[i];
    }
  }
  return NULL;
}

CDVBSub::DVB_CLUT* CDVBSub::FindClut(DVB_PAGE* pPage, BYTE bClutId)
{
  if (pPage != NULL)
  {
    for (int i=0; i<pPage->RegionCount; i++)
    {
      if (pPage->Regions[i].CLUT_id == bClutId)
        return &pPage->Regions[i].Clut;
    }
  }
  return NULL;
}

CompositionObject* CDVBSub::FindObject(DVB_PAGE* pPage, SHORT sObjectId)
{
  if (pPage != NULL)
  {
    for (std::list<CompositionObject*>::iterator it = pPage->Objects.begin();
      it != pPage->Objects.end(); it++)
    {
      if ((*it)->m_object_id_ref == sObjectId)
        return *it;
    }
  }
  return NULL;
}

HRESULT CDVBSub::AddToBuffer(BYTE* pData, int nSize)
{
  bool  bFirstChunk = (*((LONG*)pData) & 0x00FFFFFF) == 0x000f0020;  // DVB sub start with 0x20 0x00 0x0F ...

  if (m_nBufferWritePos > 0 || bFirstChunk)
  {
    if (bFirstChunk) 
    {
      m_nBufferWritePos  = 0;
      m_nBufferReadPos  = 0;
    }

    if (m_nBufferWritePos+nSize > m_nBufferSize)
    {
      if (m_nBufferWritePos+nSize > 20*BUFFER_CHUNK_GROW)
      {
        // Too big to be a DVB sub !
        TRACE_DVB (L"DVB - Too much data receive...\n");
        ASSERT (FALSE);

        Reset();
        return E_INVALIDARG;
      }

      BYTE*  pPrev = m_pBuffer;
      m_nBufferSize  = max (m_nBufferWritePos+nSize, m_nBufferSize+BUFFER_CHUNK_GROW);
      m_pBuffer    = new BYTE[m_nBufferSize];
      if (pPrev != NULL)
      {
        memcpy_s (m_pBuffer, m_nBufferSize, pPrev, m_nBufferWritePos);
        delete pPrev; pPrev = NULL;
      }
    }
    memcpy_s (m_pBuffer+m_nBufferWritePos, m_nBufferSize, pData, nSize);
    m_nBufferWritePos += nSize;
    return S_OK;
  }
  return S_FALSE;
}

#define MARKER if(gb.BitRead(1) != 1) {ASSERT(0); return(E_FAIL);}

HRESULT CDVBSub::ParseSample (IMediaSample* pSample)
{
  CheckPointer (pSample, E_POINTER);
  HRESULT        hr;
  BYTE*        pData = NULL;
  int          nSize;
  DVB_SEGMENT_TYPE  nCurSegment;

  hr = pSample->GetPointer(&pData);
  if(FAILED(hr) || pData == NULL) return hr;
  nSize = pSample->GetActualDataLength();

  if (*((LONG*)pData) == 0xBD010000)
  {
    CGolombBuffer  gb (pData, nSize);

    gb.SkipBytes(4);
    WORD  wLength  = (WORD)gb.BitRead(16);
    
    if (gb.BitRead(2) != 2) return E_FAIL;    // type

    gb.BitRead(2);    // scrambling
    gb.BitRead(1);    // priority
    gb.BitRead(1);    // alignment
    gb.BitRead(1);    // copyright
    gb.BitRead(1);    // original
    BYTE fpts = (BYTE)gb.BitRead(1);    // fpts
    BYTE fdts = (BYTE)gb.BitRead(1);    // fdts
    gb.BitRead(1);  // escr
    gb.BitRead(1);  // esrate
    gb.BitRead(1);  // dsmtrickmode
    gb.BitRead(1);  // morecopyright
    gb.BitRead(1);  // crc
    gb.BitRead(1);  // extension
    gb.BitRead(8);  // hdrlen

    if(fpts)
    {
      BYTE b = (BYTE)gb.BitRead(4);
      if(!(fdts && b == 3 || !fdts && b == 2)) {ASSERT(0); return(E_FAIL);}

      REFERENCE_TIME  pts = 0;
      pts |= gb.BitRead(3) << 30; MARKER; // 32..30
      pts |= gb.BitRead(15) << 15; MARKER; // 29..15
      pts |= gb.BitRead(15); MARKER; // 14..0
      pts = 10000*pts/90;

      m_rtStart  = pts;
      m_rtStop  = pts+1;
    }
    else
    {
      m_rtStart  = INVALID_TIME;
      m_rtStop  = INVALID_TIME;
    }

    nSize -= 14;
    pData += 14;
    pSample->GetTime(&m_rtStart, &m_rtStop);
    pSample->GetMediaTime(&m_rtStart, &m_rtStop);
  }
  else
    if (SUCCEEDED (pSample->GetTime(&m_rtStart, &m_rtStop)))
      pSample->SetTime(&m_rtStart, &m_rtStop);

  //FILE* hFile = fopen ("D:\\Sources\\mpc-hc\\A garder\\TestSubRip\\dvbsub.dat", "ab");
  //if(hFile != NULL)
  //{
  //  //BYTE  Buff[5] = {48};

  //  //*((DWORD*)(Buff+1)) = lSampleLen;
  //  //fwrite (Buff,  1, sizeof(Buff), hFile);
  //  fwrite (pData, 1, lSampleLen, hFile);
  //  fclose(hFile);
  //}

  if (AddToBuffer (pData, nSize) == S_OK)
  {
    CGolombBuffer    gb (m_pBuffer+m_nBufferReadPos, m_nBufferWritePos-m_nBufferReadPos);
    int          nLastPos = 0;

    while (!gb.IsEOF())
    {
      if (gb.ReadByte() == 0x0F)
      {
        WORD        wPageId;
        WORD        wSegLength;

        nCurSegment  = (DVB_SEGMENT_TYPE) gb.ReadByte();
        wPageId      = gb.ReadShort();
        wSegLength    = gb.ReadShort();

        if (gb.RemainingSize() < wSegLength)
        {
          hr = S_FALSE;
          break;
        }

        switch (nCurSegment)
        {
        case PAGE :
          {
            std::auto_ptr<DVB_PAGE>  pPage;
            ParsePage(gb, wSegLength, pPage);

            if (pPage->PageState == DPS_ACQUISITION)
            {
              m_pCurrentPage = pPage;
              m_pCurrentPage->rtStart = m_rtStart;
              TRACE_DVB (L"DVB - Page started  %s\n", ReftimeToString(m_rtStart).c_str());
              m_rtStart = INVALID_TIME;
            }
            else
              TRACE_DVB (L"DVB - Page update\n");
          }
          break;
        case REGION :
          ParseRegion(gb, wSegLength);
          TRACE_DVB (L"DVB - Region\n");
          break;
        case CLUT :
          ParseClut(gb, wSegLength);
          TRACE_DVB (L"DVB - Clut \n");
          break;
        case OBJECT :
          ParseObject(gb, wSegLength);
          TRACE_DVB (L"DVB - Object\n");
          break;
        case DISPLAY :
          ParseDisplay(gb, wSegLength);
          break;
        case END_OF_DISPLAY :
          if (m_pCurrentPage.get() != NULL && m_rtStart != INVALID_TIME)
          {
            m_pCurrentPage->rtStop = m_rtStart;
            TRACE_DVB (L"DVB - End display %s - %s\n", ReftimeToString(m_pCurrentPage->rtStart).c_str(), ReftimeToString(m_pCurrentPage->rtStop).c_str());
            m_Pages.push_back (m_pCurrentPage.release());
          }
          break;
        default :
//          gb.SkipBytes(wSegLength);
          break;
        }
        nLastPos = (int) gb.GetPos();
      }
    }
    m_nBufferReadPos += nLastPos;
  }

  return hr;
}

void CDVBSub::Render(SubPicDesc& spd, REFERENCE_TIME rt, RECT& bbox)
{
  DVB_PAGE*    pPage = FindPage (rt);

  if (pPage != NULL)
  {
    pPage->Rendered = true;
    for (int i=0; i<pPage->RegionCount; i++)
    {
      CDVBSub::DVB_REGION*  pRegion = &pPage->Regions[i];
      for (int j=0; j<pRegion->ObjectCount; j++)
      {
        CompositionObject*  pObject = FindObject (pPage, pRegion->Objects[j].object_id);
        if (pObject)
        {
          SHORT    nX, nY;
          nX = pRegion->HorizAddr + pRegion->Objects[j].object_horizontal_position;
          nY = pRegion->VertAddr  + pRegion->Objects[j].object_vertical_position;
          pObject->GetObjectData()->m_width  = pRegion->width;
          pObject->GetObjectData()->m_height = pRegion->height;
          pObject->SetPalette(pRegion->Clut.Size, pRegion->Clut.Palette, false);
          pObject->RenderDvb(spd, nX, nY);
        }
      }
    }

    bbox.left  = 0;
    bbox.top  = 0;
    bbox.right  = m_Display.width;
    bbox.bottom  = m_Display.height;

  }
}

HRESULT CDVBSub::GetTextureSize (uint32_t pos, SIZE& MaxTextureSize, SIZE& VideoSize, POINT& VideoTopLeft)
{
  // TODO : limit size for HDTV

  // Texture size should be video size width. Height is limited (to prevent performances issues with
  // more than 1024x768 pixels)
  MaxTextureSize.cx  = min (m_Display.width, 1920);
  MaxTextureSize.cy  = min (m_Display.height, 1024*768/MaxTextureSize.cx);

  VideoSize.cx  = m_Display.width;
  VideoSize.cy  = m_Display.height;

  VideoTopLeft.x  = 0;
  VideoTopLeft.y  = 0;

  return S_OK;
}

uint32_t CDVBSub::GetStartPosition(REFERENCE_TIME rt, double fps)
{
  DVB_PAGE*  pPage;

  // Cleanup old PG
  while (m_Pages.size() > 0)
  {
    pPage = m_Pages.front();
    if (pPage->rtStop < rt)
    {
      if (!pPage->Rendered)
        TRACE_DVB (L"DVB - remove unrendered object, %s - %s\n", ReftimeToString(pPage->rtStart).c_str(), ReftimeToString(pPage->rtStop).c_str());

      //TRACE_HDMVSUB ("CHdmvSub:HDMV remove object %d  %S => %S (rt=%S)\n", pPage->GetRLEDataSize(), 
      //         ReftimeToString (pPage->rtStart), ReftimeToString(pPage->rtStop), ReftimeToString(rt));
      m_Pages.pop_front();
      delete pPage;
    }
    else
      break;
  }

  return 0;
}

uint32_t CDVBSub::GetNext(uint32_t pos) 
{ 
  return pos + 1; 
}


REFERENCE_TIME CDVBSub::GetStart(uint32_t nPos)  
{
  std::list<DVB_PAGE*>::const_iterator it = m_Pages.begin();
  std::advance(it, nPos);
  DVB_PAGE*  pPage = *it;
  return pPage!=NULL ? pPage->rtStart : INVALID_TIME; 
}

REFERENCE_TIME  CDVBSub::GetStop(uint32_t nPos)  
{ 
  std::list<DVB_PAGE*>::const_iterator it = m_Pages.begin();
  std::advance(it, nPos);
  DVB_PAGE*  pPage = *it;
  return pPage!=NULL ? pPage->rtStop : INVALID_TIME; 
}


void CDVBSub::Reset()
{
  m_nBufferReadPos  = 0;
  m_nBufferWritePos  = 0;
  m_pCurrentPage.reset();

  DVB_PAGE*  pPage;
  while (m_Pages.size() > 0)
  {
    pPage = m_Pages.front(); m_Pages.pop_front();
    delete pPage;
  }

}

HRESULT CDVBSub::ParsePage(CGolombBuffer& gb, WORD wSegLength, std::auto_ptr<DVB_PAGE>& pPage)
{
  HRESULT    hr    = S_OK;
  WORD    wEnd  = (WORD)gb.GetPos() + wSegLength;
  int      nPos  = 0;
  
  pPage.reset (DNew DVB_PAGE());
  pPage->PageTimeOut      = gb.ReadByte();
  pPage->PageVersionNumber  = (BYTE)gb.BitRead(4);
  pPage->PageState      = (BYTE)gb.BitRead(2);
  pPage->RegionCount      = 0;
  gb.BitRead(2);  // Reserved
  while (gb.GetPos() < wEnd)
  {
    if (nPos < MAX_REGIONS)
    {
      pPage->Regions[nPos].Id      = gb.ReadByte();
      gb.ReadByte();  // Reserved
      pPage->Regions[nPos].HorizAddr  = gb.ReadShort();
      pPage->Regions[nPos].VertAddr  = gb.ReadShort();
      pPage->RegionCount++;
    }
    nPos++;
  }

  return S_OK;
}

HRESULT CDVBSub::ParseDisplay(CGolombBuffer& gb, WORD wSegLength)
{
  m_Display.version_number    = (BYTE)gb.BitRead (4);
  m_Display.display_window_flag  = (BYTE)gb.BitRead (1);
  gb.BitRead(3);  // reserved
  m_Display.width          = gb.ReadShort();
  m_Display.height        = gb.ReadShort();
  if (m_Display.display_window_flag)
  {
    m_Display.horizontal_position_minimun  = gb.ReadShort();
    m_Display.horizontal_position_maximum  = gb.ReadShort();
    m_Display.vertical_position_minimun    = gb.ReadShort();
    m_Display.vertical_position_maximum    = gb.ReadShort();
  }

  return S_OK;
}

HRESULT CDVBSub::ParseRegion(CGolombBuffer& gb, WORD wSegLength)
{
  HRESULT          hr    = S_OK;
  WORD          wEnd  = (WORD)gb.GetPos() + wSegLength;
  CDVBSub::DVB_REGION*  pRegion;
  CDVBSub::DVB_REGION    DummyRegion;

  pRegion = FindRegion (m_pCurrentPage.get(), gb.ReadByte());

  if (pRegion == NULL)
    pRegion = &DummyRegion;

  if (pRegion != NULL)
  {
    pRegion->version_number      = (BYTE)gb.BitRead(4);
    pRegion->fill_flag        = (BYTE)gb.BitRead(1);
    gb.BitRead(3);  // Reserved
    pRegion->width          = gb.ReadShort();
    pRegion->height          = gb.ReadShort();
    pRegion->level_of_compatibility  = (BYTE)gb.BitRead(3);
    pRegion->depth          = (BYTE)gb.BitRead(3);
    gb.BitRead(2);  // Reserved
    pRegion->CLUT_id        = gb.ReadByte();
    pRegion->_8_bit_pixel_code    = gb.ReadByte();
    pRegion->_4_bit_pixel_code    = (BYTE)gb.BitRead(4);
    pRegion->_2_bit_pixel_code    = (BYTE)gb.BitRead(2);
    gb.BitRead(2);  // Reserved

    pRegion->ObjectCount = 0;
    while (gb.GetPos() < wEnd)
    {
      DVB_OBJECT*    pObject = &pRegion->Objects[pRegion->ObjectCount];
      pObject->object_id          = gb.ReadShort();
      pObject->object_type        = (BYTE)gb.BitRead(2);
      pObject->object_provider_flag    = (BYTE)gb.BitRead(2);
      pObject->object_horizontal_position  = (SHORT)gb.BitRead(12);
      gb.BitRead(4);  // Reserved
      pObject->object_vertical_position  = (SHORT)gb.BitRead(12);
      if (pObject->object_type == 0x01 || pObject->object_type == 0x02)
      {
        pObject->foreground_pixel_code  = gb.ReadByte();
        pObject->background_pixel_code  = gb.ReadByte();
      }
      pRegion->ObjectCount++;
    }
  }
  else
    gb.SkipBytes (wSegLength-1);

  return S_OK;
}

HRESULT CDVBSub::ParseClut(CGolombBuffer& gb, WORD wSegLength)
{
  HRESULT        hr    = S_OK;
  WORD        wEnd  = (WORD)gb.GetPos() + wSegLength;
  CDVBSub::DVB_CLUT*  pClut;

  pClut  = FindClut (m_pCurrentPage.get(), gb.ReadByte());
//  ASSERT (pClut != NULL);
  if (pClut != NULL)
  {
    pClut->version_number  = (BYTE)gb.BitRead(4);
    gb.BitRead(4);  // Reserved

    pClut->Size = 0;
    while (gb.GetPos() < wEnd)
    {
      BYTE entry_id  = gb.ReadByte()+1;
      BYTE _2_bit    = (BYTE)gb.BitRead(1);
      BYTE _4_bit    = (BYTE)gb.BitRead(1);
      BYTE _8_bit    = (BYTE)gb.BitRead(1);
      gb.BitRead(4);  // Reserved
      
      pClut->Palette[entry_id].entry_id = entry_id;
      if (gb.BitRead(1))
      {
        pClut->Palette[entry_id].Y  = gb.ReadByte();
        pClut->Palette[entry_id].Cr  = gb.ReadByte();
        pClut->Palette[entry_id].Cb  = gb.ReadByte();
        pClut->Palette[entry_id].T  = 255-gb.ReadByte();
      }
      else
      {
        pClut->Palette[entry_id].Y  = (BYTE)gb.BitRead(6)<<2;
        pClut->Palette[entry_id].Cr  = (BYTE)gb.BitRead(4)<<4;
        pClut->Palette[entry_id].Cb  = (BYTE)gb.BitRead(4)<<4;
        pClut->Palette[entry_id].T  = 255-((BYTE)gb.BitRead(2)<<6);
      }
      pClut->Size = max (pClut->Size, entry_id);
    }
  }

  return hr;
}

HRESULT CDVBSub::ParseObject(CGolombBuffer& gb, WORD wSegLength)
{
  HRESULT        hr    = E_FAIL;

  if (m_pCurrentPage.get() && wSegLength > 2)
  {
    CompositionObject*  pObject = DNew CompositionObject();
    CompositionObjectData* pData = DNew CompositionObjectData();
    BYTE        object_coding_method;

    pObject->SetObjectData(pData);

    pObject->m_object_id_ref = pData->m_object_id = gb.ReadShort();
    pObject->m_version_number  = (BYTE)gb.BitRead(4);
    
    object_coding_method = (BYTE)gb.BitRead(2);  // object_coding_method
    gb.BitRead(1);  // non_modifying_colour_flag
    gb.BitRead(1);  // reserved

    if (object_coding_method == 0x00)
    {
      pData->SetRLEData (gb.GetBufferPos(), wSegLength-3, wSegLength-3);
      gb.SkipBytes(wSegLength-3);
      m_pCurrentPage->Objects.push_back (pObject);
      hr = S_OK;
    }
    else
    {
      delete pObject;
      hr = E_NOTIMPL;
    }
  }


  return hr;
}

