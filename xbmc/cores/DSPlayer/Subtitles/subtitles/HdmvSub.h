/* 
 * $Id: HdmvSub.h 1048 2009-04-18 17:39:19Z casimir666 $
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

#pragma once

#include "BaseSub.h"

class CGolombBuffer;

class CHdmvSub : public CBaseSub
{
public:

  static const REFERENCE_TIME INVALID_TIME = _I64_MIN;

  enum HDMV_SEGMENT_TYPE
  {
    NO_SEGMENT          = 0xFFFF,
    PALETTE             = 0x14,
    OBJECT              = 0x15,
    PRESENTATION_SEG    = 0x16,
    WINDOW_DEF          = 0x17,
    INTERACTIVE_SEG     = 0x18,
    END_OF_DISPLAY      = 0x80,
    HDMV_SUB1           = 0x81,
    HDMV_SUB2           = 0x82
  };

  static CStdStringW SegmentToString(HDMV_SEGMENT_TYPE segType)
  {
    switch (segType)
    {
    case NO_SEGMENT:
      return "NO_SEGMENT";
    case PALETTE:
      return "PALETTE";
    case OBJECT:
      return "OBJECT";
    case PRESENTATION_SEG:
      return "PRESENTATION_SEG";
    case WINDOW_DEF:
      return "WINDOW_DEF";
    case INTERACTIVE_SEG:
      return "INTERACTIVE_SEG";
    case END_OF_DISPLAY:
      return "END_OF_DISPLAY";
    case HDMV_SUB1:
      return "HDMV_SUB1";
    case HDMV_SUB2:
      return "HDMV_SUB2";
    default:
      return "Unknown";
    }
  }

  struct VIDEO_DESCRIPTOR
  {
    SHORT    nVideoWidth;
    SHORT    nVideoHeight;
    BYTE     bFrameRate;    // <= Frame rate here!
  };

  struct COMPOSITION_DESCRIPTOR
  {
    SHORT    nNumber;
    BYTE     bState;
  };

  struct SEQUENCE_DESCRIPTOR
  {
    BYTE    bFirstIn  : 1;
    BYTE    bLastIn   : 1;
    BYTE    bReserved : 8;
  };

  struct PGSSubs
  {
    REFERENCE_TIME                   m_rtStart;
    REFERENCE_TIME                   m_rtStop;
    int                              m_numObjects;
    std::vector<CompositionObject*>  m_objects;

    VIDEO_DESCRIPTOR m_videoDescriptor;
    COMPOSITION_DESCRIPTOR m_compositionDescriptor;
    SEQUENCE_DESCRIPTOR m_sequenceDescriptor;

    PGSSubs()
    {
      m_rtStart = 0;
      m_rtStop = INVALID_TIME;
      m_numObjects = 0;

      memset (&m_videoDescriptor, 0, sizeof(VIDEO_DESCRIPTOR));
      memset (&m_compositionDescriptor, 0, sizeof(COMPOSITION_DESCRIPTOR));
    }

    ~PGSSubs()
    {
      for (std::vector<CompositionObject*>::iterator it = m_objects.begin();
        it != m_objects.end(); ++it)
        (*it)->SetObjectData(NULL);
    }
  };

  CHdmvSub(bool fromFile = false);
  ~CHdmvSub();

  HRESULT      ParseSample (IMediaSample* pSample);
  HRESULT      ParseData( REFERENCE_TIME rtStart, REFERENCE_TIME rtStop, BYTE* pData, long size );


  uint32_t    GetStartPosition(REFERENCE_TIME rt, double fps);
  uint32_t    GetNext(uint32_t pos) { return ((pos >= m_pSubs.size()) ? NULL : ++pos); };


  virtual REFERENCE_TIME GetStart(uint32_t nPos)
  {
    std::list<PGSSubs*>::iterator it = m_pSubs.begin();
    std::advance(it, nPos - 1);
    PGSSubs* pObject = *it;
    return pObject != NULL ? pObject->m_rtStart : INVALID_TIME; 
  };
  virtual REFERENCE_TIME  GetStop(uint32_t nPos)  
  {
    std::list<PGSSubs*>::iterator it = m_pSubs.begin();
    std::advance(it, nPos - 1);
    PGSSubs*  pObject = *(it);
    return pObject != NULL ? pObject->m_rtStop : INVALID_TIME; 
  };

  void      Render(SubPicDesc& spd, REFERENCE_TIME rt, RECT& bbox);
  HRESULT   GetTextureSize (uint32_t pos, SIZE& MaxTextureSize, SIZE& VideoSize, POINT& VideoTopLeft);
  void      Reset();

private :

  HDMV_SEGMENT_TYPE             m_nCurSegment;
  BYTE*                         m_pSegBuffer;
  int                           m_nTotalSegBuffer;
  int                           m_nSegBufferPos;
  int                           m_nSegSize;
  bool                          m_bFromFile;

  PGSSubs*                          m_pCurrentSub;
  std::list<PGSSubs*>               m_pSubs;
  std::list<CompositionObjectData*> m_pObjectsCache;

  HDMV_PALETTE*                 m_pDefaultPalette;
  int                           m_nDefaultPaletteNbEntry;

  int                           m_nColorNumber;

  void        GetTopLeft(PGSSubs* pSub, POINT& point);
  void        GetDrawingRect(PGSSubs* pSub, Com::SmartRect& pRect);

  int         ParsePresentationSegment(CGolombBuffer* pGBuffer, REFERENCE_TIME rtStart);
  void        ParsePalette(CGolombBuffer* pGBuffer, USHORT nSize);
  void        ParseObject(CGolombBuffer* pGBuffer, USHORT nUnitSize);
  void        ParseWindow(CGolombBuffer* pGBuffer, USHORT nSize);

  void        ParseVideoDescriptor(CGolombBuffer* pGBuffer, VIDEO_DESCRIPTOR* pVideoDescriptor);
  void        ParseCompositionDescriptor(CGolombBuffer* pGBuffer, COMPOSITION_DESCRIPTOR* pCompositionDescriptor);
  void        ParseCompositionObject(CGolombBuffer* pGBuffer, PGSSubs* pCompositionObject, BYTE numWindows);

  void        AllocSegment(int nSize);

  PGSSubs*    FindSub(REFERENCE_TIME rt);

  CompositionObjectData* FindObject(int id)
  {
    for (std::list<CompositionObjectData*>::iterator it = m_pObjectsCache.begin();
      it != m_pObjectsCache.end(); ++it)
    {
      if ((*it)->m_object_id == id)
        return (*it);
    }

    return NULL;
  }
};
