/* 
 * $Id: HdmvSub.cpp 1048 2009-04-18 17:39:19Z casimir666 $
 *
 * (C) 2006-2007 see AUTHORS
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

#include "StdAfx.h"
#include "HdmvSub.h"
#include "..\DSUtil\GolombBuffer.h"
#include "..\subpic\ISubPic.h"

#if (1)    // Set to 1 to activate HDMV subtitles traces
  #define TRACE_HDMVSUB    TRACE
#else
  #define TRACE_HDMVSUB
#endif


CHdmvSub::CHdmvSub(bool fromFile /*= false*/)
  : CBaseSub(ST_HDMV), m_bFromFile(fromFile)
{
  m_nColorNumber            = 0;
  m_nCurSegment             = NO_SEGMENT;
  m_pSegBuffer              = NULL;
  m_nTotalSegBuffer         = 0;
  m_nSegBufferPos           = 0;
  m_nSegSize                = 0;
  m_pCurrentObject          = NULL;
  m_pDefaultPalette         = NULL;
  m_nDefaultPaletteNbEntry  = 0;
  m_bGotObjectData          = false;
  m_bGotPaletteData         = false;

  memset (&m_VideoDescriptor, 0, sizeof(VIDEO_DESCRIPTOR));
}

CHdmvSub::~CHdmvSub()
{
  while (! m_pObjects.empty())
  {
    delete m_pObjects.back();
    m_pObjects.pop_back();
  }

  delete[] m_pSegBuffer;
  delete[] m_pDefaultPalette;
  delete m_pCurrentObject;
}


void CHdmvSub::AllocSegment(int nSize)
{
  if (nSize > m_nTotalSegBuffer)
  {
    delete[] m_pSegBuffer;
    m_pSegBuffer    = DNew BYTE[nSize];
    m_nTotalSegBuffer  = nSize;
  }
  m_nSegBufferPos   = 0;
  m_nSegSize       = nSize;
}

__w64 int CHdmvSub::GetStartPosition(REFERENCE_TIME rt, double fps)
{
  CompositionObject*  pObject;

  // First index is 1
  // Cleanup old PG
  int i = 0;
  while (m_pObjects.size() > 0)
  {
    pObject = m_pObjects.front(); i++;
    if (pObject->m_rtStop < rt)
    {
      if (!m_bFromFile)
      {
        TRACE_HDMVSUB (L"CHdmvSub:HDMV remove object %d  %s => %s (rt=%s)\n", pObject->GetRLEDataSize(), 
                 ReftimeToString (pObject->m_rtStart).c_str(), ReftimeToString(pObject->m_rtStop).c_str(), ReftimeToString(rt).c_str());
        m_pObjects.pop_front(); i--;
        delete pObject;
      }
    }
    else
      break;
  }

  return i; 
}

HRESULT CHdmvSub::ParseData( REFERENCE_TIME rtStart, REFERENCE_TIME rtStop, BYTE* pData, long size )
{
  if (pData)
  {
    CGolombBuffer SampleBuffer (pData, size);
    while (!SampleBuffer.IsEOF())
    {
      if (m_nCurSegment == NO_SEGMENT)
      {
        HDMV_SEGMENT_TYPE  nSegType  = (HDMV_SEGMENT_TYPE)SampleBuffer.ReadByte();
        USHORT        nUnitSize  = SampleBuffer.ReadShort();
        size -= 3;

        switch (nSegType)
        {
        case PALETTE :
        case OBJECT :
        case PRESENTATION_SEG :
        case END_OF_DISPLAY :
          m_nCurSegment = nSegType;
          AllocSegment (nUnitSize);
          break;
        case WINDOW_DEF :
        case INTERACTIVE_SEG :
        case HDMV_SUB1 :
        case HDMV_SUB2 :
          // Ignored stuff...
          SampleBuffer.SkipBytes(nUnitSize);
          break;
        default :
          return VFW_E_SAMPLE_REJECTED;
        }
      }

      if (m_nCurSegment != NO_SEGMENT)
      {
        if (m_nSegBufferPos < m_nSegSize)
        {
          int nSize = min(m_nSegSize - m_nSegBufferPos, size);
          SampleBuffer.ReadBuffer(m_pSegBuffer + m_nSegBufferPos, nSize);
          m_nSegBufferPos += nSize;
        }
      
        if (m_nSegBufferPos >= m_nSegSize)
        {
          CGolombBuffer SegmentBuffer(m_pSegBuffer, m_nSegSize);

          switch (m_nCurSegment)
          {
          case PALETTE :
            TRACE_HDMVSUB (L"CHdmvSub:PALETTE            rtStart=%10I64d\n", rtStart);
            ParsePalette(&SegmentBuffer, m_nSegSize);
            m_bGotPaletteData = true;
            break;
          case OBJECT :
            //TRACE_HDMVSUB (L"CHdmvSub:OBJECT             rtStart=%s\n", ReftimeToString(rtStart).c_str());
            ParseObject(&SegmentBuffer, m_nSegSize);
            m_bGotObjectData = true;
            break;
          case PRESENTATION_SEG :
            TRACE_HDMVSUB (L"CHdmvSub:PRESENTATION_SEG   rtStart=%s (size=%d)\n", ReftimeToString(rtStart).c_str(), m_nSegSize);
          
            if (m_pCurrentObject)
            {
              if (! m_bGotObjectData)
              {
                // Sometimes, teh segment does not contain OBJECT data. Just use the data of the previous object
                CompositionObject* pObject = m_pObjects.back();
                m_pCurrentObject->Copy(pObject, !m_bGotPaletteData);
              }
              m_bGotObjectData = false;
              m_bGotPaletteData = false;
              m_pCurrentObject->m_rtStop = rtStart;
              m_pObjects.push_back (m_pCurrentObject);
              TRACE_HDMVSUB (L"CHdmvSub:HDMV : %s => %s\n", ReftimeToString (m_pCurrentObject->m_rtStart).c_str(), ReftimeToString(rtStart).c_str());
              m_pCurrentObject = NULL;
            }

            if (ParsePresentationSegment(&SegmentBuffer) > 0)
            {
              m_pCurrentObject->m_rtStart  = rtStart;
              m_pCurrentObject->m_rtStop  = _I64_MAX;
            }
            break;
          case WINDOW_DEF :
            // TRACE_HDMVSUB ("CHdmvSub:WINDOW_DEF         %S\n", ReftimeToString(rtStart));
            break;
          case END_OF_DISPLAY :
            // TRACE_HDMVSUB ("CHdmvSub:END_OF_DISPLAY     %S\n", ReftimeToString(rtStart));
            break;
          default :
            TRACE_HDMVSUB (L"CHdmvSub:UNKNOWN Seg %d     rtStart=0x%10dd\n", m_nCurSegment, rtStart);
          }

          m_nCurSegment = NO_SEGMENT;
        }
      }
    }
  }

  return S_OK;
}

HRESULT CHdmvSub::ParseSample(IMediaSample* pSample)
{
  CheckPointer (pSample, E_POINTER);
  HRESULT        hr;
  REFERENCE_TIME    rtStart = INVALID_TIME, rtStop = INVALID_TIME;
  BYTE*        pData = NULL;
  int          lSampleLen;

  hr = pSample->GetPointer(&pData);
  if(FAILED(hr) || pData == NULL) return hr;
  lSampleLen = pSample->GetActualDataLength();

  pSample->GetTime(&rtStart, &rtStop);

  return ParseData(rtStart, rtStop, pData, lSampleLen);
}

int CHdmvSub::ParsePresentationSegment(CGolombBuffer* pGBuffer)
{
  COMPOSITION_DESCRIPTOR  CompositionDescriptor;
  BYTE          nObjectNumber;
  bool          palette_update_flag;
  BYTE          palette_id_ref;

  ParseVideoDescriptor(pGBuffer, &m_VideoDescriptor);
  ParseCompositionDescriptor(pGBuffer, &CompositionDescriptor);
  palette_update_flag  = !!(pGBuffer->ReadByte() & 0x80);
  palette_id_ref       = pGBuffer->ReadByte();
  nObjectNumber        = pGBuffer->ReadByte();

  if (nObjectNumber > 0)
  {
    delete m_pCurrentObject;
    m_pCurrentObject = DNew CompositionObject();
    ParseCompositionObject (pGBuffer, m_pCurrentObject);
  }

  return nObjectNumber;
}

void CHdmvSub::ParsePalette(CGolombBuffer* pGBuffer, USHORT nSize) // #497
{
  int    nNbEntry;
  BYTE  palette_id        = pGBuffer->ReadByte();
  BYTE  palette_version_number  = pGBuffer->ReadByte();

  ASSERT ((nSize-2) % sizeof(HDMV_PALETTE) == 0);
  nNbEntry = (nSize-2) / sizeof(HDMV_PALETTE);
  HDMV_PALETTE*  pPalette = (HDMV_PALETTE*)pGBuffer->GetBufferPos();

  if (m_pDefaultPalette == NULL || m_nDefaultPaletteNbEntry != nNbEntry)
  {
    delete[] m_pDefaultPalette;
    m_pDefaultPalette = new HDMV_PALETTE[nNbEntry];
    m_nDefaultPaletteNbEntry = nNbEntry;
  }
  memcpy (m_pDefaultPalette, pPalette, nNbEntry*sizeof(HDMV_PALETTE));

  if (m_pCurrentObject)
    m_pCurrentObject->SetPalette (nNbEntry, pPalette, m_VideoDescriptor.nVideoWidth>720);
}

void CHdmvSub::ParseObject(CGolombBuffer* pGBuffer, USHORT nUnitSize)  // #498
{
  SHORT object_id = pGBuffer->ReadShort();
  BYTE m_sequence_desc;

  ASSERT (m_pCurrentObject != NULL);
  if (m_pCurrentObject)// && m_pCurrentObject->m_object_id_ref == object_id)
  {
    m_pCurrentObject->m_version_number  = pGBuffer->ReadByte();
    m_sequence_desc            = pGBuffer->ReadByte();

    if (m_sequence_desc & 0x80)
    {
      DWORD  object_data_length  = (DWORD)pGBuffer->BitRead(24);
      
      m_pCurrentObject->m_width      = pGBuffer->ReadShort();
      m_pCurrentObject->m_height       = pGBuffer->ReadShort();

      m_pCurrentObject->SetRLEData (pGBuffer->GetBufferPos(), nUnitSize-11, object_data_length-4);

      TRACE_HDMVSUB (L"CHdmvSub:NewObject  size=%ld, total obj=%d, %dx%d\n", object_data_length, m_pObjects.size(),
               m_pCurrentObject->m_width, m_pCurrentObject->m_height);
    }
    else
      m_pCurrentObject->AppendRLEData (pGBuffer->GetBufferPos(), nUnitSize-4);
  }
}

void CHdmvSub::ParseCompositionObject(CGolombBuffer* pGBuffer, CompositionObject* pCompositionObject)
{
  BYTE  bTemp;
  pCompositionObject->m_object_id_ref  = pGBuffer->ReadShort();
  pCompositionObject->m_window_id_ref  = pGBuffer->ReadByte();
  bTemp = pGBuffer->ReadByte();
  pCompositionObject->m_object_cropped_flag  = !!(bTemp & 0x80);
  pCompositionObject->m_forced_on_flag    = !!(bTemp & 0x40);
  pCompositionObject->m_horizontal_position  = pGBuffer->ReadShort();
  pCompositionObject->m_vertical_position    = pGBuffer->ReadShort();

  if (pCompositionObject->m_object_cropped_flag)
  {
    pCompositionObject->m_cropping_horizontal_position  = pGBuffer->ReadShort();
    pCompositionObject->m_cropping_vertical_position  = pGBuffer->ReadShort();
    pCompositionObject->m_cropping_width        = pGBuffer->ReadShort();
    pCompositionObject->m_cropping_height        = pGBuffer->ReadShort();
  }
}

void CHdmvSub::ParseVideoDescriptor(CGolombBuffer* pGBuffer, VIDEO_DESCRIPTOR* pVideoDescriptor)
{
  pVideoDescriptor->nVideoWidth   = pGBuffer->ReadShort();
  pVideoDescriptor->nVideoHeight  = pGBuffer->ReadShort();
  pVideoDescriptor->bFrameRate  = pGBuffer->ReadByte();
}

void CHdmvSub::ParseCompositionDescriptor(CGolombBuffer* pGBuffer, COMPOSITION_DESCRIPTOR* pCompositionDescriptor)
{
  pCompositionDescriptor->nNumber  = pGBuffer->ReadShort();
  pCompositionDescriptor->bState  = pGBuffer->ReadByte();
}

void CHdmvSub::Render(SubPicDesc& spd, REFERENCE_TIME rt, RECT& bbox)
{
  CompositionObject*  pObject = FindObject (rt);

  ASSERT (pObject!=NULL && spd.w >= pObject->m_width && spd.h >= pObject->m_height);

  if (pObject && spd.w >= pObject->m_width && spd.h >= pObject->m_height)
  {
    if (!pObject->HavePalette())
      pObject->SetPalette (m_nDefaultPaletteNbEntry, m_pDefaultPalette, m_VideoDescriptor.nVideoWidth>720);

    TRACE_HDMVSUB (L"CHdmvSub:Render      size=%ld,  ObjRes=%dx%d,  SPDRes=%dx%d\n", pObject->GetRLEDataSize(), 
             pObject->m_width, pObject->m_height, spd.w, spd.h);
    pObject->RenderHdmv(spd);

    bbox.left  = 0;
    bbox.top  = 0;
    bbox.right  = bbox.left + pObject->m_width;
    bbox.bottom  = bbox.top  + pObject->m_height;
  }
}

HRESULT CHdmvSub::GetTextureSize (int pos, SIZE& MaxTextureSize, SIZE& VideoSize, POINT& VideoTopLeft)
{
  std::list<CompositionObject *>::iterator it = m_pObjects.begin();
  std::advance(it, pos - 1);
  CompositionObject*  pObject = *it;
  if (pObject)
  {
    MaxTextureSize.cx	= m_VideoDescriptor.nVideoWidth;
    MaxTextureSize.cy	= m_VideoDescriptor.nVideoHeight;

    VideoSize.cx  = m_VideoDescriptor.nVideoWidth;
    VideoSize.cy  = m_VideoDescriptor.nVideoHeight;

    VideoTopLeft.x  = pObject->m_horizontal_position;
    VideoTopLeft.y  = pObject->m_vertical_position;

    return S_OK;
  }

  ASSERT (FALSE);
  return E_INVALIDARG;
}


void CHdmvSub::Reset()
{
  CompositionObject*  pObject;
  while (! m_pObjects.empty())
  {
    delete m_pObjects.back();
    m_pObjects.pop_back();
  }
}

CompositionObject*	CHdmvSub::FindObject(REFERENCE_TIME rt)
{
  std::list<CompositionObject *>::const_iterator it = m_pObjects.begin();
  for(; it != m_pObjects.end(); ++it)
  {
    CompositionObject*  pObject = *it;

    if (rt >= pObject->m_rtStart && rt < pObject->m_rtStop)
      return pObject;

  }

  return NULL;
}


