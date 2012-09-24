/*
 *      Copyright (C) 2005-2012 Team XBMC
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

#include "PeripheralAmbiPi.h"
#include "utils/log.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "cores/VideoRenderers/RenderManager.h"
#include "cores/VideoRenderers/WinRenderer.h"

#define AMBIPI_DEFAULT_PORT 20434

extern CXBMCRenderManager g_renderManager;

using namespace PERIPHERALS;
using namespace AUTOPTR;

CPeripheralAmbiPi::CPeripheralAmbiPi(const PeripheralType type, const PeripheralBusType busType, const CStdString &strLocation, const CStdString &strDeviceName, int iVendorId, int iProductId) :
  CPeripheral(type, busType, strLocation, strDeviceName, iVendorId, iProductId),
  CThread("AmbiPi"),
  m_bStarted(false),
  m_bIsRunning(false),
  m_pGrid(NULL),
  m_previousImageWidth(0),
  m_previousImageHeight(0)
{  
  m_features.push_back(FEATURE_AMBIPI);
}

CPeripheralAmbiPi::~CPeripheralAmbiPi(void)
{
  CLog::Log(LOGDEBUG, "%s - Removing AmbiPi on %s", __FUNCTION__, m_strLocation.c_str());

  DisconnectFromDevice();

  delete m_pGrid;
}

bool CPeripheralAmbiPi::InitialiseFeature(const PeripheralFeature feature)
{
  if (feature != FEATURE_AMBIPI || m_bStarted || !GetSettingBool("enabled")) {
    return CPeripheral::InitialiseFeature(feature);
  }

  CLog::Log(LOGDEBUG, "%s - Initialising AmbiPi on %s", __FUNCTION__, m_strLocation.c_str());

  LoadAddressFromConfiguration();
  UpdateGridFromConfiguration();

  ConnectToDevice();

  m_bStarted = true;
  Create();

  return CPeripheral::InitialiseFeature(feature);
}

void CPeripheralAmbiPi::LoadAddressFromConfiguration()
{
  
  CStdString portFromConfiguration = GetSettingString("port");

  int port;

  if (sscanf(portFromConfiguration.c_str(), "%d", &port) &&
      port >= 0 &&
      port <= 65535)
    m_port = port;
  else
    m_port = AMBIPI_DEFAULT_PORT;

  m_address = GetSettingString("address");
}

void CPeripheralAmbiPi::UpdateGridFromConfiguration()
{

  unsigned int width = 16;
  unsigned int height = 11;

  if (m_pGrid)
    delete m_pGrid;

  m_pGrid = new CAmbiPiGrid(width, height);

}

bool CPeripheralAmbiPi::ConfigureRenderCallback()
{
  if (!g_renderManager.IsStarted() || !g_renderManager.IsConfigured()) {
    return false;
  }

  g_renderManager.RegisterRenderUpdateCallBack((const void*)this, RenderUpdateCallBack);
  return true;
}

void CPeripheralAmbiPi::RenderUpdateCallBack(const void *ctx, const CRect &SrcRect, const CRect &DestRect)
{
  CPeripheralAmbiPi *peripheralAmbiPi = (CPeripheralAmbiPi*)ctx;

  peripheralAmbiPi->ProcessImage();
}

void CPeripheralAmbiPi::ProcessImage()
{
  UpdateImage();  
  GenerateDataStreamFromImage();
  SendData();
}

void CPeripheralAmbiPi::UpdateImage()
{
  CWinRenderer *pRenderer = g_renderManager.m_pRenderer;
  
  int index = pRenderer->GetImage(&m_image);
}

void CPeripheralAmbiPi::GenerateDataStreamFromImage()
{
  UpdateSampleRectangles(m_image.width, m_image.height);

  m_pGrid->UpdateTilesFromImage(&m_image);
}

void CPeripheralAmbiPi::UpdateSampleRectangles(unsigned int imageWidth, unsigned int imageHeight)
{
  if (imageWidth == m_previousImageWidth || imageHeight == m_previousImageHeight)
    return;

  m_previousImageWidth = imageWidth;
  m_previousImageHeight = imageHeight;

  m_pGrid->UpdateSampleRectangles(imageWidth, imageHeight);
}

void CPeripheralAmbiPi::SendData(void) {
  TileData *pTileData = m_pGrid->GetTileData();
  m_connection.Send(pTileData->stream, pTileData->streamLength);
}

void CPeripheralAmbiPi::ConnectToDevice()
{
  CLog::Log(LOGINFO, "%s - Connecting to AmbiPi on %s:%d", __FUNCTION__, m_address.c_str(), m_port);  

  m_connection.Disconnect();
  m_connection.Connect(m_address, m_port);
}

#define RETRY_DELAY_WHEN_UNCONFIGURED 1
#define RETRY_DELAY_WHEN_CONFIGURED 5

void CPeripheralAmbiPi::Process(void)
{

  {
    CSingleLock lock(m_critSection);
    m_bIsRunning = true;
  }

  int retryDelay = RETRY_DELAY_WHEN_UNCONFIGURED;

  while (!m_bStop)
  {
    if (!m_bStop) {
      bool configured = ConfigureRenderCallback();
      
      retryDelay = configured ? RETRY_DELAY_WHEN_CONFIGURED : RETRY_DELAY_WHEN_UNCONFIGURED;
    }

    if (!m_bStop)
      Sleep(retryDelay);
  }

  {
    CSingleLock lock(m_critSection);
    m_bStarted = false;
    m_bIsRunning = false;
  }

}

void CPeripheralAmbiPi::OnSettingChanged(const CStdString &strChangedSetting)
{
  CLog::Log(LOGDEBUG, "%s - handling configuration change, setting: '%s'", __FUNCTION__, strChangedSetting.c_str());

  DisconnectFromDevice();
  InitialiseFeature(FEATURE_AMBIPI);
}

void CPeripheralAmbiPi::DisconnectFromDevice(void)
{
  CLog::Log(LOGDEBUG, "%s - disconnecting from the AmbiPi", __FUNCTION__);
  if (IsRunning())
  {
    StopThread(true);
  }
  m_connection.Disconnect();
}

bool CPeripheralAmbiPi::IsRunning(void) const
{
  CSingleLock lock(m_critSection);
  return m_bIsRunning;
}


CAmbiPiGrid::~CAmbiPiGrid()
{
  free(m_tiles);
  free(m_tileData.stream);
}

#define SIZE_OF_X_AND_Y_COORDINATE 2
CAmbiPiGrid::CAmbiPiGrid(unsigned int width, unsigned int height) :
  m_width(width),
  m_height(height)
{  
  
  m_numTiles = (m_width * 2) + ((m_height - 2) * 2);
  unsigned int allocSize = m_numTiles * sizeof(Tile);
  m_tiles = (Tile*)malloc(allocSize);
  ZeroMemory(m_tiles, allocSize);


  m_tileData.streamLength = sizeof(m_tileData.streamLength) + (m_numTiles * (SIZE_OF_X_AND_Y_COORDINATE + sizeof(YUV)));
  m_tileData.stream = (BYTE *)malloc(m_tileData.streamLength);

  UpdateTileCoordinates(width, height);
  DumpCoordinates();
}

TileData *CAmbiPiGrid::GetTileData(void)
{
  BYTE *pStream = m_tileData.stream;

  ZeroMemory(pStream, m_tileData.streamLength);

  unsigned int tileIndex = 0;
  Tile* pTile;

  *pStream++ = (BYTE) (m_tileData.streamLength >> 24 & 0xFF);
  *pStream++ = (BYTE) (m_tileData.streamLength >> 16 & 0xFF);
  *pStream++ = (BYTE) (m_tileData.streamLength >> 8 & 0xFF);
  *pStream++ = (BYTE) (m_tileData.streamLength >> 0 & 0xFF);

  while (tileIndex < m_numTiles) {
    pTile = m_tiles + tileIndex;

    *pStream++ = (BYTE)pTile->m_x;
    *pStream++ = (BYTE)pTile->m_y;

    *pStream++ = pTile->m_yuv.y;
    *pStream++ = pTile->m_yuv.u;
    *pStream++ = pTile->m_yuv.v;

    tileIndex++;
  }  

  unsigned int bytesAddedToStream = pStream - m_tileData.stream;

  return &m_tileData;
}

void CAmbiPiGrid::UpdateTileCoordinates(unsigned int width, unsigned int height)
{
  unsigned int start_position_x = width / 2;

  unsigned int zeroBasedY;
  unsigned int leftHalfZeroBasedX;
  unsigned int rightHalfZeroBasedX;

  unsigned int x;
  unsigned int y;

  unsigned int leftTileIndex = 0;
  unsigned int rightTileIndex = m_numTiles / 2;

  for (y = 1; y <= height; y++)
  {
    zeroBasedY = y - 1;
    if (y == 1) 
    {
      // ProcessFromCenterToEdges

      for (x = start_position_x; x > 0; x--)
      {
        leftHalfZeroBasedX = x - 1;
        rightHalfZeroBasedX = width - x;
        UpdateSingleTileCoordinates(leftTileIndex++, leftHalfZeroBasedX, zeroBasedY);
        UpdateSingleTileCoordinates(rightTileIndex++, rightHalfZeroBasedX, zeroBasedY);
      }
    } 
    else if (y == height)
    {
      // ProcessFromEdgesToCenter

      for (x = 1; x <= start_position_x; x++)
      {
        leftHalfZeroBasedX = x - 1;
        rightHalfZeroBasedX = width - x;
        UpdateSingleTileCoordinates(leftTileIndex++, leftHalfZeroBasedX, zeroBasedY);
        UpdateSingleTileCoordinates(rightTileIndex++, rightHalfZeroBasedX, zeroBasedY);
      }
    }
    else
    {
      // ProcessEdgesOnly

      leftHalfZeroBasedX = 0;
      rightHalfZeroBasedX = width - 1;
      UpdateSingleTileCoordinates(leftTileIndex++, leftHalfZeroBasedX, y - 1);
      UpdateSingleTileCoordinates(rightTileIndex++, rightHalfZeroBasedX, y - 1);
    }
  }
  CLog::Log(LOGDEBUG, "%s - current indexes, left: %d, right: %d", __FUNCTION__, leftTileIndex, rightTileIndex);  
}

void CAmbiPiGrid::UpdateSingleTileCoordinates(unsigned int tileIndex, unsigned int x, unsigned int y)
{
  Tile* pTile = m_tiles + tileIndex;
  pTile->m_x = x;
  pTile->m_y = y;
}

void CAmbiPiGrid::DumpCoordinates(void)
{
  unsigned int tileIndex = 0;
  Tile* pTile;

  while (tileIndex < m_numTiles) {

    pTile = m_tiles + tileIndex;
    CLog::Log(LOGDEBUG, "%s - Coordinates, index: %d, x: %d, y: %d", __FUNCTION__, tileIndex, pTile->m_x, pTile->m_y);  

    tileIndex++;
  }  
}

void CAmbiPiGrid::UpdateSampleRectangles(unsigned int imageWidth, unsigned int imageHeight)
{
  unsigned int tileIndex = 0;
  Tile* pTile;

  unsigned int sampleWidth = imageWidth / m_width;
  unsigned int sampleHeight = imageHeight / m_height;

  CLog::Log(LOGDEBUG, "%s - Updating grid sample rectangles for image, width: %d, height: %d, sampleWidth: %d, sampleHeight: %d", __FUNCTION__, imageWidth, imageHeight, sampleWidth, sampleHeight);  

  while (tileIndex < m_numTiles) {

    pTile = m_tiles + tileIndex;

    pTile->m_sampleRect.SetRect(
      pTile->m_x * sampleWidth,
      pTile->m_y * sampleHeight,
      std::min(imageWidth, (pTile->m_x * sampleWidth) + sampleWidth),
      std::min(imageHeight, (pTile->m_y * sampleHeight) + sampleHeight)
    );

    CLog::Log(LOGDEBUG, "%s - updated rectangle, x: %d, y: %d, rect: (left: %f, top: %f, right: %f, bottom: %f)",
      __FUNCTION__, 
      pTile->m_x,
      pTile->m_y,
      pTile->m_sampleRect.x1,
      pTile->m_sampleRect.y1,
      pTile->m_sampleRect.x2,
      pTile->m_sampleRect.y2
    );  

    tileIndex++;
  }
}

#define PLANE_Y 0
#define PLANE_U 1
#define PLANE_V 2

void CAmbiPiGrid::UpdateTilesFromImage(const YV12Image* pImage)
{
  unsigned int tileIndex = 0;
  Tile* pTile;

  while (tileIndex < m_numTiles) {

    pTile = m_tiles + tileIndex;
    CalculateAverageColorForTile(pImage, pTile);
    tileIndex++;
  }  
}

void CAmbiPiGrid::CalculateAverageColorForTile(const YV12Image* pImage, Tile *pTile) {

  if (pImage->bpp != 1)
  {
    ZeroMemory(&pTile->m_yuv, sizeof(YUV));
    return; // TODO implement support for 2bpp images
  }

  BYTE *pLineY = pImage->plane[PLANE_Y];
  BYTE *pLineU = pImage->plane[PLANE_U];
  BYTE *pLineV = pImage->plane[PLANE_V];

  BYTE *pY, *pU, *pV;

  RGB rgb;
  YUV yuv;

  AverageRGB averageRgb = { 0, 0, 0 };
  AverageYUV averageYuv = { 0, 0, 0 };

  unsigned long int pixelsInTile = (pTile->m_sampleRect.y2 - pTile->m_sampleRect.y1) * (pTile->m_sampleRect.x2 - pTile->m_sampleRect.x1);
  
  for (int y = pTile->m_sampleRect.y1; y < pTile->m_sampleRect.y2; y++) 
  {
    pY = pLineY;
    pU = pLineU;
    pV = pLineV;
    for (int x = pTile->m_sampleRect.x1; x < pTile->m_sampleRect.x2; x++) 
    {
      yuv.y = *(pY++);
      yuv.u = *(pU++);
      yuv.v = *(pV++);

      CImageConversion::ConvertYuvToRgb(&yuv, &rgb);

      UpdateAverageYuv(&yuv, pixelsInTile, &averageYuv);
      UpdateAverageRgb(&rgb, pixelsInTile, &averageRgb);
    }
    pLineY += pImage->stride[PLANE_Y];
    pLineU += pImage->stride[PLANE_U];
    pLineV += pImage->stride[PLANE_V];
  }
  UpdateAverageColorForTile(pTile, &averageYuv);

  /*
  CLog::Log(LOGDEBUG, "%s - average color for tile, x: %d, y: %d, RGB: #%02x%02x%02x, YUV: #%02x%02x%02x",
    __FUNCTION__, 
    pTile->m_x, 
    pTile->m_y, 
    (BYTE)averageRgb->r, 
    (BYTE)averageRgb->g,
    (BYTE)averageRgb->b,
    (BYTE)averageYuv->y, 
    (BYTE)averageYuv->u,
    (BYTE)averageYuv->v
  );
  */

}

void CAmbiPiGrid::UpdateAverageYuv(const YUV *pYuv, unsigned long int numPixels, AverageYUV *pAverageYuv)
{
  pAverageYuv->y += ((float)pYuv->y / numPixels);
  pAverageYuv->u += ((float)pYuv->u / numPixels);
  pAverageYuv->v += ((float)pYuv->v / numPixels);
}

void CAmbiPiGrid::UpdateAverageRgb(const RGB *pRgb, unsigned long int numPixels, AverageRGB *pAverageRgb)
{
  pAverageRgb->r += ((float)pRgb->r / numPixels);
  pAverageRgb->g += ((float)pRgb->g / numPixels);
  pAverageRgb->b += ((float)pRgb->b / numPixels);
}

void CAmbiPiGrid::UpdateAverageColorForTile(Tile *pTile, const AverageYUV *pAverageYuv)
{
  pTile->m_yuv.y = pAverageYuv->y;
  pTile->m_yuv.u = pAverageYuv->u;
  pTile->m_yuv.v = pAverageYuv->v;
}

void CImageConversion::ConvertYuvToRgb(const YUV *pYuv, RGB *pRgb)
{
  pRgb->r = pYuv->y + 1.4075 * (pYuv->v - 128);
  pRgb->g = pYuv->y - 0.3455 * (pYuv->u - 128) - (0.7169 * (pYuv->v - 128));
  pRgb->b = pYuv->y + 1.7790 * (pYuv->u - 128);
}

CAmbiPiConnection::CAmbiPiConnection(void) :
  CThread("AmbiPi"),
  m_socket(INVALID_SOCKET),
  m_bConnected(false),
  m_bConnecting(false)
{
}

CAmbiPiConnection::~CAmbiPiConnection(void)
{
  Disconnect();
}

void CAmbiPiConnection::Connect(const CStdString ip_address_or_name, unsigned int port)
{
  Disconnect();
  m_ip_address_or_name = ip_address_or_name;
  m_port = port;
  Create();
}

void CAmbiPiConnection::Disconnect()
{
  if (m_bConnecting)
  {
    StopThread(true);
  }

  if (!m_bConnected)
  {
    return;
  }

  if (m_socket.isValid())
  {
    m_socket.reset();
  }
}

void CAmbiPiConnection::Process(void)
{
  {
    CSingleLock lock(m_critSection);
    m_bConnecting = true;
  }

  while (!m_bStop && !m_bConnected)
  {
    AttemptConnection();

    if (!m_bConnected && !m_bStop)
      Sleep(5);
  }

  {
    CSingleLock lock(m_critSection);
    m_bConnecting = false;
  }
}

void CAmbiPiConnection::AttemptConnection()
{
  struct addrinfo *pAddressInfo;

  BYTE *helloMessage = (BYTE *)"ambipi\n";
  try
  {
    pAddressInfo = GetAddressInfo(m_ip_address_or_name, m_port);
    AttemptConnection(pAddressInfo);
    Send(helloMessage, strlen((char *)helloMessage));
    freeaddrinfo(pAddressInfo);
  } 
  catch (...) 
  {
    CLog::Log(LOGERROR, "%s - connection to AmbiPi failed", __FUNCTION__);
    return;
  }

  {
    CSingleLock lock(m_critSection);
    m_bConnected = true;
  }
}

struct CAmbiPiConnectionException : std::exception { char const* what() const throw() { return "Connection exception"; }; };
struct CAmbiPiSendException : std::exception { char const* what() const throw() { return "Send exception"; }; };

struct addrinfo *CAmbiPiConnection::GetAddressInfo(const CStdString ip_address_or_name, unsigned int port)
{
  struct   addrinfo hints, *pAddressInfo;
  char service[33];
  
  memset(&hints, 0, sizeof(hints));
  hints.ai_family   = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;

  sprintf(service, "%d", port);

  int res = getaddrinfo(ip_address_or_name.c_str(), service, &hints, &pAddressInfo);
  if(res)
  {
    CLog::Log(LOGERROR, "%s - failed to lookup %s, error: '%s'", __FUNCTION__, ip_address_or_name.c_str(), gai_strerror(res));
    throw CAmbiPiConnectionException();
  }
  return pAddressInfo;
}

void CAmbiPiConnection::AttemptConnection(struct addrinfo *pAddressInfo)
{
  char     nameBuffer[NI_MAXHOST], portBuffer[NI_MAXSERV];

  SOCKET   socketHandle = INVALID_SOCKET;
  struct addrinfo *pCurrentAddressInfo;

  for (pCurrentAddressInfo = pAddressInfo; pCurrentAddressInfo; pCurrentAddressInfo = pCurrentAddressInfo->ai_next)
  {
    if (getnameinfo(
      pCurrentAddressInfo->ai_addr, 
      pCurrentAddressInfo->ai_addrlen,
      nameBuffer, sizeof(nameBuffer),
      portBuffer, sizeof(portBuffer),
      NI_NUMERICHOST)
    )
    {
      strcpy(nameBuffer, "[unknown]");
      strcpy(portBuffer, "[unknown]");
  	}
    CLog::Log(LOGDEBUG, "%s - connecting to: %s:%s ...", __FUNCTION__, nameBuffer, portBuffer);

    socketHandle = socket(pCurrentAddressInfo->ai_family, pCurrentAddressInfo->ai_socktype, pCurrentAddressInfo->ai_protocol);
    if (socketHandle == INVALID_SOCKET)
      continue;

    if (connect(socketHandle, pCurrentAddressInfo->ai_addr, pCurrentAddressInfo->ai_addrlen) != SOCKET_ERROR)
      break;

    closesocket(socketHandle);
    socketHandle = INVALID_SOCKET;
  }

  if(socketHandle == INVALID_SOCKET)
  {
    CLog::Log(LOGERROR, "%s - failed to connect", __FUNCTION__);
    throw CAmbiPiConnectionException();
  }

  m_socket.attach(socketHandle);
  CLog::Log(LOGINFO, "%s - connected to: %s:%s ...", __FUNCTION__, nameBuffer, portBuffer);
}

void CAmbiPiConnection::Send(const BYTE *buffer, int length)
{
  int iErr = send((SOCKET)m_socket, (const char *)buffer, length, 0);
  if (iErr <= 0)
  {
    throw CAmbiPiSendException();
  }
}
