#pragma once
/*
 *      Copyright (C) 2005-2011 Team XBMC
 *      http://xbmc.org
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

#include "system.h"
#include "threads/Thread.h"
#include "threads/CriticalSection.h"

#include "Peripheral.h"
#include "guilib/Geometry.h"

#include "cores/VideoRenderers/BaseRenderer.h"

#include "utils/AutoPtrHandle.h"

namespace PERIPHERALS
{
  struct AverageYUV {
    float y,u,v;
  };

  struct AverageRGB {
    float r,g,b;
  };

  struct YUV {
    BYTE y,u,v;
  };

  struct RGB {
    BYTE r,g,b;
  };


  struct Tile
  {
    unsigned int      m_x;
    unsigned int      m_y;
    CRect             m_sampleRect;
    YUV               m_yuv;
  };

  struct TileData {
    BYTE *stream;
    DWORD streamLength; 
  };

  class CAmbiPiGrid
  {
  public:
    CAmbiPiGrid(unsigned int width, unsigned int height);
    ~CAmbiPiGrid(void);
    void UpdateSampleRectangles(unsigned int imageWidth, unsigned int imageHeight);
    void UpdateTilesFromImage(const YV12Image* pImage);
    TileData *GetTileData(void);

  protected:
    unsigned int m_width;
    unsigned int m_height;
    unsigned int m_numTiles;
    Tile* m_tiles;
    TileData m_tileData;

    void UpdateTileCoordinates(unsigned int width, unsigned int height);    

  private:
    void UpdateSingleTileCoordinates(unsigned int leftTileIndex, unsigned int x, unsigned int y);
    void DumpCoordinates(void);
    void CalculateAverageColorForTile(const YV12Image* pImage, Tile *pTile);
    void UpdateAverageColorForTile(Tile *pTile, const AverageYUV *pAverageYuv);
    void UpdateAverageYuv(const YUV *pYuv, unsigned long int numPixels, AverageYUV *pAverageYuv);
    void UpdateAverageRgb(const RGB *pRgb, unsigned long int numPixels, AverageRGB *pAverageRgb);
  };

  class CImageConversion
  {
  public:
    static void ConvertYuvToRgb(const YUV *pYuv, RGB *pRgb);
  };


  class CAmbiPiConnection  
  {
  public:
    CAmbiPiConnection(void);
    ~CAmbiPiConnection(void);
    void Connect(const CStdString ip_address_or_name, unsigned int port);
    void Send(const BYTE *buffer, int length);

  private:
    struct addrinfo *GetAddressInfo(const CStdString ip_address_or_name, unsigned int port);
    void AttemptConnection(struct addrinfo *pAddressInfo);
    AUTOPTR::CAutoPtrSocket m_socket;
  };

  class CPeripheralAmbiPi : public CPeripheral, private CThread
  {
  public:
    CPeripheralAmbiPi(const PeripheralType type, const PeripheralBusType busType, const CStdString &strLocation, const CStdString &strDeviceName, int iVendorId, int iProductId);
    virtual ~CPeripheralAmbiPi(void);

  protected:
	  bool InitialiseFeature(const PeripheralFeature feature);

    void ConnectToDevice(void);
    void LoadAddressFromConfiguration(void);
    bool ConfigureRenderCallback(void);
    void Process(void);

    void ProcessImage(void);
    void UpdateImage(void);
    void GenerateDataStreamFromImage(void);
    void UpdateGridFromConfiguration(void);
    void SendData(void);

    bool                              m_bStarted;
    bool                              m_bIsRunning;
    int                               m_port;
    CStdString                        m_address;

  private:
    CCriticalSection                  m_critSection;
    YV12Image                         m_image;    
    CAmbiPiGrid*                      m_pGrid;
    unsigned int                      m_previousImageWidth;
    unsigned int                      m_previousImageHeight;

    static void RenderUpdateCallBack(const void *ctx, const CRect &SrcRect, const CRect &DestRect);

    void UpdateSampleRectangles(unsigned int imageWidth, unsigned int imageHeight);
    CAmbiPiConnection                 m_connection;
  };
}
