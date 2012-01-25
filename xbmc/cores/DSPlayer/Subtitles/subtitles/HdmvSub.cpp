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
  m_pCurrentSub             = NULL;
  m_pDefaultPalette         = NULL;
  m_nDefaultPaletteNbEntry  = 0;
}

CHdmvSub::~CHdmvSub()
{
  while (! m_pSubs.empty())
  {
    delete m_pSubs.back();
    m_pSubs.pop_back();
  }

  while (! m_pObjectsCache.empty())
  {
    delete m_pObjectsCache.back();
    m_pObjectsCache.pop_back();
  }

  delete[] m_pSegBuffer;
  delete[] m_pDefaultPalette;
  delete m_pCurrentSub;
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

uint32_t CHdmvSub::GetStartPosition(REFERENCE_TIME rt, double fps)
{
  PGSSubs*  pObject;

  // First index is 1
  // Cleanup old PG
  size_t i = 0;
  while (m_pSubs.size() > 0)
  {
    pObject = m_pSubs.front(); i++;
    if (pObject->m_rtStop < rt)
    {
      //if (!m_bFromFile)
      //{
        TRACE_HDMVSUB (L"CHdmvSub:HDMV remove object %s => %s (rt=%s)\n", ReftimeToString (pObject->m_rtStart).c_str(),
          ReftimeToString(pObject->m_rtStop).c_str(), ReftimeToString(rt).c_str());
        m_pSubs.pop_front(); i--;
        delete pObject;
      //}
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
        USHORT             nUnitSize  = SampleBuffer.ReadShort();
        TRACE_HDMVSUB(L"New segment: %s (size: %d)", SegmentToString(nSegType).c_str(), nUnitSize);
        size -= 3;

        switch (nSegType)
        {
        case PALETTE:
        case OBJECT:
        case PRESENTATION_SEG:
        case END_OF_DISPLAY:
        case WINDOW_DEF:
          m_nCurSegment = nSegType;
          AllocSegment (nUnitSize);
          break;
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
            break;
          case OBJECT :
            //TRACE_HDMVSUB (L"CHdmvSub:OBJECT             rtStart=%s\n", ReftimeToString(rtStart).c_str());
            ParseObject(&SegmentBuffer, m_nSegSize);
            break;
          case PRESENTATION_SEG :
            TRACE_HDMVSUB (L"CHdmvSub:PRESENTATION_SEG   rtStart=%s (size=%d)\n", ReftimeToString(rtStart).c_str(), m_nSegSize);
            ParsePresentationSegment(&SegmentBuffer, rtStart);
            break;
          case WINDOW_DEF :
            TRACE_HDMVSUB (L"CHdmvSub:WINDOW_DEF         %s\n", ReftimeToString(rtStart).c_str());
            ParseWindow(&SegmentBuffer, m_nSegSize);
            break;
          case END_OF_DISPLAY :
            TRACE_HDMVSUB (L"CHdmvSub:END_OF_DISPLAY     %s\n", ReftimeToString(rtStart).c_str());
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

void CHdmvSub::ParseWindow(CGolombBuffer* pGBuffer, USHORT nSize)
{
 /*
  * Window Segment Structure (No new information provided):
  *     2 bytes: Unkown,
  *     2 bytes: X position of subtitle,
  *     2 bytes: Y position of subtitle,
  *     2 bytes: Width of subtitle,
  *     2 bytes: Height of subtitle.
  */

  // TODO

  if (m_pCurrentSub == NULL)
    return;

  int numWindows = pGBuffer->ReadByte();
 
  for (int i = 0; i < numWindows; i++)
  {
    BYTE window_id            = pGBuffer->ReadByte();
    
    int horizontal_position   = pGBuffer->ReadShort();
    int vertical_position     = pGBuffer->ReadShort();
    int width                 = pGBuffer->ReadShort();
    int height                = pGBuffer->ReadShort();

    TRACE_HDMVSUB (L"CHdmvSub:Window[id: %d]  size: %dx%d pos: %dx%d", window_id, width, height, horizontal_position, vertical_position);

    /*if (m_pCurrentObject->m_objects[j].m_horizontal_position == 0)
      m_pCurrentObject->m_objects[j].m_horizontal_position = horizontal_position;
    if (m_pCurrentObject->m_objects[j].m_vertical_position == 0)
      m_pCurrentObject->m_objects[j].m_vertical_position = vertical_position;
    if (m_pCurrentObject->m_objects[j].m_width == 0)
      m_pCurrentObject->m_objects[j].m_width = width;
    if (m_pCurrentObject->m_objects[j].m_height == 0)
      m_pCurrentObject->m_objects[j].m_height = height;*/

    break;
  }
}

int CHdmvSub::ParsePresentationSegment(CGolombBuffer* pGBuffer, REFERENCE_TIME rtStart)
{
  BYTE          nObjectNumber;
  bool          palette_update_flag;
  BYTE          palette_id_ref;

  if (m_pCurrentSub)
  {
    m_pCurrentSub->m_rtStop = rtStart;

    m_pSubs.push_back (m_pCurrentSub);
    TRACE_HDMVSUB (L"CHdmvSub:HDMV : %s => %s\n", ReftimeToString (m_pCurrentSub->m_rtStart).c_str(), ReftimeToString(rtStart).c_str());
    m_pCurrentSub = NULL;
  }

  VIDEO_DESCRIPTOR mVideo;
  COMPOSITION_DESCRIPTOR mComposition;
  ParseVideoDescriptor(pGBuffer, &mVideo);
  ParseCompositionDescriptor(pGBuffer, &mComposition);

  palette_update_flag  = !!(pGBuffer->ReadByte() & 0x80);
  palette_id_ref       = pGBuffer->ReadByte();
  nObjectNumber        = pGBuffer->ReadByte();

  if (nObjectNumber > 0)
  {
    delete m_pCurrentSub;
    m_pCurrentSub = DNew PGSSubs();

    memcpy(&m_pCurrentSub->m_videoDescriptor, &mVideo, sizeof(VIDEO_DESCRIPTOR));
    memcpy(&m_pCurrentSub->m_compositionDescriptor, &mComposition, sizeof(COMPOSITION_DESCRIPTOR));

    ParseCompositionObject (pGBuffer, m_pCurrentSub, nObjectNumber);

    m_pCurrentSub->m_rtStart  = rtStart;
    m_pCurrentSub->m_rtStop   = _I64_MAX;
  }

  return nObjectNumber;
}

void CHdmvSub::ParsePalette(CGolombBuffer* pGBuffer, USHORT nSize) // #497
{
  int    nNbEntry;
  BYTE  palette_id              = pGBuffer->ReadByte();
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

  if (m_pCurrentSub)
  {
    for (size_t i = 0; i < m_pCurrentSub->m_objects.size(); i++)
      m_pCurrentSub->m_objects[i]->SetPalette (nNbEntry, pPalette, m_pCurrentSub->m_videoDescriptor.nVideoWidth>720);
  }
}

void CHdmvSub::ParseObject(CGolombBuffer* pGBuffer, USHORT nUnitSize)  // #498
{
  SHORT object_id = pGBuffer->ReadShort();
  BYTE m_sequence_desc;

  CompositionObjectData* pObject = NULL;
  pObject = FindObject(object_id);

  if (! pObject) {
    pObject = new CompositionObjectData();
  } else {
    // The object already exists.
    // However, we can't replace because it may be used in previous subtitles
    // Two choices:
    // 1) Don't use a shared cache between subs (bigger mem print)
    // 2) Move the id to a free one
    // We use 1) for the moment
    m_pObjectsCache.remove(pObject);
    delete pObject;
    pObject = new CompositionObjectData();
  }

  if (! pObject)
    return;

  pObject->m_object_id       = object_id;
  pObject->m_version_number  = pGBuffer->ReadByte();
  m_sequence_desc            = pGBuffer->ReadByte();

  if (m_sequence_desc & 0x80)
  {
    DWORD  object_data_length  = (DWORD)pGBuffer->BitRead(24);
      
    pObject->m_width           = pGBuffer->ReadShort();
    pObject->m_height          = pGBuffer->ReadShort();

    pObject->SetRLEData (pGBuffer->GetBufferPos(), nUnitSize-11, object_data_length-4);

    TRACE_HDMVSUB (L"CHdmvSub:NewObject  id=%d  size=%ld, total obj=%d, drawing size=%dx%d", object_id, object_data_length, m_pSubs.size(), pObject->m_width, pObject->m_height);
  }
  else
    pObject->AppendRLEData (pGBuffer->GetBufferPos(), nUnitSize-4);
  
  m_pObjectsCache.push_back(pObject);
  if (m_pCurrentSub)
  {
    for (std::vector<CompositionObject*>::iterator it = m_pCurrentSub->m_objects.begin();
      it != m_pCurrentSub->m_objects.end(); ++it)
    {
      if ((*it)->m_object_id_ref == pObject->m_object_id)
        (*it)->SetObjectData(pObject);
    }
  }
}

void CHdmvSub::ParseCompositionObject(CGolombBuffer* pGBuffer, PGSSubs* pSubs, BYTE numWindows)
{
  pSubs->m_numObjects = numWindows;
  for (int i = 0; i < numWindows; i++)
  {
    BYTE  bTemp;
    SHORT objectId = pGBuffer->ReadShort();

    CompositionObject *pCompositionObject = new CompositionObject();

    pCompositionObject->m_object_id_ref  = objectId;
    pCompositionObject->m_window_id_ref  = pGBuffer->ReadByte();
    bTemp = pGBuffer->ReadByte();
    pCompositionObject->m_object_cropped_flag  = !!(bTemp & 0x80);
    pCompositionObject->m_forced_on_flag    = !!(bTemp & 0x40);
    pCompositionObject->m_horizontal_position  = pGBuffer->ReadShort();
    pCompositionObject->m_vertical_position    = pGBuffer->ReadShort();
    TRACE_HDMVSUB(L"ParseCompositionObject object_id_ref: %d  window_id_ref: %d  horz pos: %d  vert pos: %d",
      pCompositionObject->m_object_id_ref, pCompositionObject->m_window_id_ref,
      pCompositionObject->m_horizontal_position, pCompositionObject->m_vertical_position);

    if (pCompositionObject->m_object_cropped_flag)
    {
      pCompositionObject->m_cropping_horizontal_position  = pGBuffer->ReadShort();
      pCompositionObject->m_cropping_vertical_position    = pGBuffer->ReadShort();
      pCompositionObject->m_cropping_width                = pGBuffer->ReadShort();
      pCompositionObject->m_cropping_height               = pGBuffer->ReadShort();
    }

    CompositionObjectData * pData = FindObject(objectId);
    pCompositionObject->SetObjectData(pData);

    pSubs->m_objects.push_back(pCompositionObject);
  }
}

void CHdmvSub::ParseVideoDescriptor(CGolombBuffer* pGBuffer, VIDEO_DESCRIPTOR* pVideoDescriptor)
{
  pVideoDescriptor->nVideoWidth   = pGBuffer->ReadShort();
  pVideoDescriptor->nVideoHeight  = pGBuffer->ReadShort();
  pVideoDescriptor->bFrameRate    = pGBuffer->ReadByte();
}

void CHdmvSub::ParseCompositionDescriptor(CGolombBuffer* pGBuffer, COMPOSITION_DESCRIPTOR* pCompositionDescriptor)
{
  pCompositionDescriptor->nNumber  = pGBuffer->ReadShort();
  pCompositionDescriptor->bState  = pGBuffer->ReadByte();
  TRACE_HDMVSUB(L"CHdmvSub::CompositionDescriptor : Num: %d     State: %d", pCompositionDescriptor->nNumber, pCompositionDescriptor->bState);
}

void CHdmvSub::Render(SubPicDesc& spd, REFERENCE_TIME rt, RECT& bbox)
{
  PGSSubs* pObj = FindSub(rt);
  if (! pObj)
    return;

  POINT position;
  GetTopLeft(pObj, position);

  for (int i = 0; i < pObj->m_numObjects; i++)
  {
    CompositionObject* pObject = (pObj->m_objects[i]);
    if (!pObject->GetObjectData())
      continue;

    ASSERT (pObject!=NULL && spd.w >= pObject->GetObjectData()->m_width && spd.h >= pObject->GetObjectData()->m_height);

    if (pObject && spd.w >= pObject->GetObjectData()->m_width && spd.h >= pObject->GetObjectData()->m_height)
    {
      if (!pObject->HavePalette())
        pObject->SetPalette (m_nDefaultPaletteNbEntry, m_pDefaultPalette, pObj->m_videoDescriptor.nVideoWidth>720);

      TRACE_HDMVSUB (L"CHdmvSub:Render      size=%ld,  ObjRes=%dx%d,  SPDRes=%dx%d\n", pObject->GetObjectData()->GetRLEDataSize(), 
               pObject->GetObjectData()->m_width, pObject->GetObjectData()->m_height, spd.w, spd.h);

      short x = 0, y = 0;
      x = pObject->m_horizontal_position - (short) position.x;
      y = pObject->m_vertical_position - (short) position.y;
      pObject->RenderHdmv(spd, x, y);

      bbox.left     = 0;
      bbox.top      = 0;
      bbox.right    = bbox.left + bbox.right + pObject->GetObjectData()->m_width;
      bbox.bottom   = bbox.top  + bbox.bottom + pObject->GetObjectData()->m_height;
    }
  }
}

HRESULT CHdmvSub::GetTextureSize (uint32_t pos, SIZE& MaxTextureSize, SIZE& VideoSize, POINT& VideoTopLeft)
{
  std::list<PGSSubs *>::iterator it = m_pSubs.begin();
  std::advance(it, pos - 1);
  PGSSubs* pObject = *it;
  VideoSize.cx = 0; VideoSize.cy = 0;
  if (pObject)
  {
    Com::SmartRect pRect;
    GetDrawingRect(pObject, pRect);

    MaxTextureSize = pRect.Size();
    VideoTopLeft = pRect.TopLeft();

    VideoSize.cx = pObject->m_videoDescriptor.nVideoWidth;
    VideoSize.cy = pObject->m_videoDescriptor.nVideoHeight;

    return S_OK;
  }

  ASSERT (FALSE);
  return E_INVALIDARG;
}

void CHdmvSub::GetDrawingRect(PGSSubs* pSub, Com::SmartRect& pRect)
{
  pRect.SetRectEmpty();
  for (int i = 0; i < pSub->m_numObjects; i++)
  {
    Com::SmartPoint pos(pSub->m_objects[i]->m_horizontal_position, pSub->m_objects[i]->m_vertical_position);
    Com::SmartSize size(pSub->m_objects[i]->GetObjectData()->m_width, pSub->m_objects[i]->GetObjectData()->m_height);
    Com::SmartRect rect(pos, size);
    pRect.UnionRect(pRect, rect);
  }
}

void CHdmvSub::GetTopLeft(PGSSubs* pSub, POINT& point)
{
  Com::SmartRect rect;
  GetDrawingRect(pSub, rect);

  point = rect.TopLeft();
}

void CHdmvSub::Reset()
{
  while (! m_pSubs.empty())
  {
    delete m_pSubs.back();
    m_pSubs.pop_back();
  }

  while (! m_pObjectsCache.empty())
  {
    delete m_pObjectsCache.back();
    m_pObjectsCache.pop_back();
  }

  delete m_pCurrentSub;
  m_pCurrentSub = NULL;

  m_nCurSegment = NO_SEGMENT;
}

CHdmvSub::PGSSubs* CHdmvSub::FindSub(REFERENCE_TIME rt)
{
  std::list<PGSSubs *>::const_iterator it = m_pSubs.begin();
  for(; it != m_pSubs.end(); ++it)
  {
    PGSSubs* pObject = *it;

    if (rt >= pObject->m_rtStart && rt < pObject->m_rtStop)
      return pObject;
  }

  return NULL;
}