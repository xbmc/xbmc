/* 
 * $Id: DVBSub.h 1785 2010-04-09 14:12:59Z xhmikosr $
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

#include "BaseSub.h"
#include "boost\shared_ptr.hpp"

#define MAX_REGIONS      10
#define MAX_OBJECTS      10      // Max number of objects per region

class CGolombBuffer;

class CDVBSub : public CBaseSub
{
public:
  CDVBSub(void);
  ~CDVBSub(void);

  virtual HRESULT ParseSample (IMediaSample* pSample);
  virtual void Render(SubPicDesc& spd, REFERENCE_TIME rt, RECT& bbox);
  virtual HRESULT GetTextureSize (uint32_t pos, SIZE& MaxTextureSize, SIZE& VideoSize, POINT& VideoTopLeft);
  virtual uint32_t GetStartPosition(REFERENCE_TIME rt, double fps);
  virtual uint32_t GetNext(uint32_t pos);
  virtual REFERENCE_TIME GetStart(uint32_t nPos);  
  virtual REFERENCE_TIME GetStop(uint32_t nPos);
  virtual void Reset();

  // EN 300-743, table 2
  enum DVB_SEGMENT_TYPE
  {
    NO_SEGMENT     = 0xFFFF,
    PAGE           = 0x10,
    REGION         = 0x11,
    CLUT           = 0x12,
    OBJECT         = 0x13,
    DISPLAY        = 0x14,
    END_OF_DISPLAY = 0x80
  };

  // EN 300-743, table 6
  enum DVB_OBJECT_TYPE
  {
    OT_BASIC_BITMAP       = 0x00,
    OT_BASIC_CHAR         = 0x01,
    OT_COMPOSITE_STRING   = 0x02
  };

  enum DVB_PAGE_STATE
  {
    DPS_NORMAL        = 0x00,
    DPS_ACQUISITION   = 0x01,
    DPS_MODE          = 0x02,
    DPS_RESERVED      = 0x03
  };

  struct DVB_CLUT
  {
    BYTE      id;
    BYTE      version_number;
    BYTE      Size;

    HDMV_PALETTE  Palette[256];

    DVB_CLUT()
    {
      memset (Palette, 0, sizeof(Palette));
    }
  };

  struct DVB_DISPLAY
  {
    BYTE      version_number;
    BYTE      display_window_flag;
    SHORT     width;
    SHORT     height;
    SHORT     horizontal_position_minimun;
    SHORT     horizontal_position_maximum;
    SHORT     vertical_position_minimun;
    SHORT     vertical_position_maximum;
    
    DVB_DISPLAY()
    {
      // Default value (§5.1.3)
      version_number  = 0;
      width           = 720;
      height          = 576;
    }
  };

  struct DVB_OBJECT
  {
    SHORT       object_id;
    BYTE        object_type;
    BYTE        object_provider_flag;
    SHORT       object_horizontal_position;
    SHORT       object_vertical_position;
    BYTE        foreground_pixel_code;
    BYTE        background_pixel_code;

    DVB_OBJECT()
    {
      object_id                   = 0xFF;
      object_type                 = 0;
      object_provider_flag        = 0;
      object_horizontal_position  = 0;
      object_vertical_position    = 0;
      foreground_pixel_code       = 0;
      background_pixel_code       = 0;
    }
  };

  struct DVB_REGION
  {
    BYTE    Id;
    WORD    HorizAddr;
    WORD    VertAddr;
    BYTE    version_number;
    BYTE    fill_flag;
    WORD    width;
    WORD    height;
    BYTE    level_of_compatibility;
    BYTE    depth;
    BYTE    CLUT_id;
    BYTE    _8_bit_pixel_code;
    BYTE    _4_bit_pixel_code;
    BYTE    _2_bit_pixel_code;
    int      ObjectCount;
    DVB_OBJECT  Objects[MAX_OBJECTS];

    DVB_CLUT  Clut;

    DVB_REGION()
    {
      Id                       = 0;
      HorizAddr                = 0;
      VertAddr                 = 0;
      version_number           = 0;
      fill_flag                = 0;
      width                    = 0;
      height                   = 0;
      level_of_compatibility   = 0;
      depth                    = 0;
      CLUT_id                  = 0;
      _8_bit_pixel_code        = 0;
      _4_bit_pixel_code        = 0;
      _2_bit_pixel_code        = 0;
    }
  };

  class DVB_PAGE
  {
  public :
    REFERENCE_TIME          rtStart;
    REFERENCE_TIME          rtStop;
    BYTE                    PageTimeOut;
    BYTE                    PageVersionNumber;
    BYTE                    PageState;
    int                     RegionCount;
    DVB_REGION              Regions[MAX_REGIONS];
    std::list<CompositionObject*>  Objects;
    bool                    Rendered;

    DVB_PAGE()
    {
      PageTimeOut        = 0;
      PageVersionNumber  = 0;
      PageState          = 0;
      RegionCount        = 0;
      Rendered           = false;
    }

    ~DVB_PAGE()
    {
      CompositionObject*  pPage;
      while (Objects.size() > 0)
      {
        pPage = Objects.front(); Objects.pop_front();
        delete pPage;
      }
    }
  };

private:
  static const REFERENCE_TIME INVALID_TIME = _I64_MIN;

  int          m_nBufferSize;
  int          m_nBufferReadPos;
  int          m_nBufferWritePos;
  BYTE*        m_pBuffer;
  std::list<DVB_PAGE*>      m_Pages;
  std::auto_ptr<DVB_PAGE>   m_pCurrentPage;
  DVB_DISPLAY               m_Display;
  REFERENCE_TIME            m_rtStart;
  REFERENCE_TIME            m_rtStop;

  HRESULT             AddToBuffer(BYTE* pData, int nSize);
  DVB_PAGE*           FindPage(REFERENCE_TIME rt);
  DVB_REGION*         FindRegion(DVB_PAGE* pPage, BYTE bRegionId);
  DVB_CLUT*           FindClut(DVB_PAGE* pPage, BYTE bClutId);
  CompositionObject*  FindObject(DVB_PAGE* pPage, SHORT sObjectId);

  HRESULT        ParsePage(CGolombBuffer& gb, WORD wSegLength, std::auto_ptr<DVB_PAGE>& pPage);
  HRESULT        ParseDisplay(CGolombBuffer& gb, WORD wSegLength);
  HRESULT        ParseRegion(CGolombBuffer& gb, WORD wSegLength);
  HRESULT        ParseClut(CGolombBuffer& gb, WORD wSegLength);
  HRESULT        ParseObject(CGolombBuffer& gb, WORD wSegLength);

};
