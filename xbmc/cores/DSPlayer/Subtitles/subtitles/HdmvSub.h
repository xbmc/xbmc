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
    NO_SEGMENT      = 0xFFFF,
    PALETTE        = 0x14,
    OBJECT        = 0x15,
    PRESENTATION_SEG  = 0x16,
    WINDOW_DEF      = 0x17,
    INTERACTIVE_SEG    = 0x18,
    END_OF_DISPLAY    = 0x80,
    HDMV_SUB1      = 0x81,
    HDMV_SUB2      = 0x82
  };

  
  struct VIDEO_DESCRIPTOR
  {
    SHORT    nVideoWidth;
    SHORT    nVideoHeight;
    BYTE    bFrameRate;    // <= Frame rate here!
  };

  struct COMPOSITION_DESCRIPTOR
  {
    SHORT    nNumber;
    BYTE    bState;
  };

  struct SEQUENCE_DESCRIPTOR
  {
    BYTE    bFirstIn  : 1;
    BYTE    bLastIn    : 1;
    BYTE    bReserved : 8;
  };

  CHdmvSub();
  ~CHdmvSub();

  HRESULT      ParseSample (IMediaSample* pSample);


  int    GetStartPosition(REFERENCE_TIME rt, double fps);
  int    GetNext(int pos) { return ((pos >=  m_pObjects.size()) ? NULL : ++pos); };


  virtual REFERENCE_TIME	GetStart(int nPos)	
  {
    std::list<CompositionObject*>::iterator it = m_pObjects.begin();
    std::advance(it, nPos);
    CompositionObject*  pObject = *it;
    return pObject!=NULL ? pObject->m_rtStart : INVALID_TIME; 
  };
  virtual REFERENCE_TIME  GetStop(int nPos)  
  { 
    std::list<CompositionObject*>::iterator it = m_pObjects.begin();
    std::advance(it, nPos);
    CompositionObject*  pObject = *(it);
    return pObject!=NULL ? pObject->m_rtStop : INVALID_TIME; 
  };

  void      Render(SubPicDesc& spd, REFERENCE_TIME rt, RECT& bbox);
  HRESULT   GetTextureSize (int pos, SIZE& MaxTextureSize, SIZE& VideoSize, POINT& VideoTopLeft);
  void      Reset();

private :

  HDMV_SEGMENT_TYPE  m_nCurSegment;
  BYTE*              m_pSegBuffer;
  int                m_nTotalSegBuffer;
  int                m_nSegBufferPos;
  int                m_nSegSize;

  VIDEO_DESCRIPTOR        m_VideoDescriptor;

  CompositionObject*        m_pCurrentObject;
  std::list<CompositionObject*>  m_pObjects;

  HDMV_PALETTE*          m_pDefaultPalette;
  int                m_nDefaultPaletteNbEntry;

  int                m_nColorNumber;


  int         ParsePresentationSegment(CGolombBuffer* pGBuffer);
  void        ParsePalette(CGolombBuffer* pGBuffer, USHORT nSize);
  void        ParseObject(CGolombBuffer* pGBuffer, USHORT nUnitSize);

  void        ParseVideoDescriptor(CGolombBuffer* pGBuffer, VIDEO_DESCRIPTOR* pVideoDescriptor);
  void        ParseCompositionDescriptor(CGolombBuffer* pGBuffer, COMPOSITION_DESCRIPTOR* pCompositionDescriptor);
  void        ParseCompositionObject(CGolombBuffer* pGBuffer, CompositionObject* pCompositionObject);

  void        AllocSegment(int nSize);

  CompositionObject*  FindObject(REFERENCE_TIME rt);
};
