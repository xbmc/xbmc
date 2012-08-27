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

//#include "system.h"
#include "PeripheralAmbiPi.h"
#include "utils/log.h"

//#include "threads/SingleLock.h"

#include "cores/VideoRenderers/RenderManager.h"
#include "cores/VideoRenderers/WinRenderer.h"

#define AMBIPI_DEFAULT_PORT 20434

extern CXBMCRenderManager g_renderManager;

using namespace PERIPHERALS;

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

  StopThread(true);

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
  //CLog::Log(LOGINFO, "%s - AmbiPi callback hit", __FUNCTION__);  

  peripheralAmbiPi->ProcessImage();
}

void CPeripheralAmbiPi::ProcessImage()
{
  UpdateImage();  
  GenerateDataStreamFromImage();
}

void CPeripheralAmbiPi::UpdateImage()
{
  CWinRenderer *pRenderer = g_renderManager.m_pRenderer;
  
  int index = pRenderer->GetImage(&m_image);
}

void CPeripheralAmbiPi::GenerateDataStreamFromImage()
{
  //CLog::Log(LOGDEBUG, "%s - Processing image", __FUNCTION__);  

  UpdateSampleRectangles(m_image.width, m_image.height);
}

void CPeripheralAmbiPi::UpdateSampleRectangles(unsigned int imageWidth, unsigned int imageHeight)
{
  if (imageWidth == m_previousImageWidth || imageHeight == m_previousImageHeight)
    return;

  m_previousImageWidth = imageWidth;
  m_previousImageHeight = imageHeight;

  m_pGrid->UpdateSampleRectangles(imageWidth, imageHeight);
}

void CPeripheralAmbiPi::ConnectToDevice()
{
  CLog::Log(LOGINFO, "%s - Connecting to AmbiPi on %s:%d", __FUNCTION__, m_address.c_str(), m_port);  

  // TODO connect ambi (TCP)
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


CAmbiPiGrid::~CAmbiPiGrid()
{
  free(m_gridPoints);
}

CAmbiPiGrid::CAmbiPiGrid(unsigned int width, unsigned int height) :
  m_width(width),
  m_height(height)
{  
  
  m_numGridPoints = (m_width * 2) + ((m_height - 2) * 2);
  unsigned int allocSize = m_numGridPoints * sizeof(SGridPoint);
  m_gridPoints = (SGridPoint*)malloc(allocSize);
  ZeroMemory(m_gridPoints, allocSize);

  UpdateGridPoints(width, height);
  DumpCoordinates();
}

void CAmbiPiGrid::UpdateGridPoints(unsigned int width, unsigned int height)
{
  unsigned int start_position_x = width / 2;

  unsigned int zeroBasedY;
  unsigned int leftHalfZeroBasedX;
  unsigned int rightHalfZeroBasedX;

  unsigned int x;
  unsigned int y;

  unsigned int leftGridPointIndex = 0;
  unsigned int rightGridPointIndex = m_numGridPoints / 2;

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
        UpdateGridPoint(leftGridPointIndex++, leftHalfZeroBasedX, zeroBasedY);
        UpdateGridPoint(rightGridPointIndex++, rightHalfZeroBasedX, zeroBasedY);
      }
    } 
    else if (y == height)
    {
      // ProcessFromEdgesToCenter

      for (x = 1; x <= start_position_x; x++)
      {
        leftHalfZeroBasedX = x - 1;
        rightHalfZeroBasedX = width - x;
        UpdateGridPoint(leftGridPointIndex++, leftHalfZeroBasedX, zeroBasedY);
        UpdateGridPoint(rightGridPointIndex++, rightHalfZeroBasedX, zeroBasedY);
      }
    }
    else
    {
      // ProcessEdgesOnly

      leftHalfZeroBasedX = 0;
      rightHalfZeroBasedX = width - 1;
      UpdateGridPoint(leftGridPointIndex++, leftHalfZeroBasedX, y - 1);
      UpdateGridPoint(rightGridPointIndex++, rightHalfZeroBasedX, y - 1);
    }
  }
  CLog::Log(LOGDEBUG, "%s - current indexes, left: %d, right: %d", __FUNCTION__, leftGridPointIndex, rightGridPointIndex);  
}

void CAmbiPiGrid::UpdateGridPoint(unsigned int gridPointIndex, unsigned int x, unsigned int y)
{
  SGridPoint* pGridPoint = m_gridPoints + gridPointIndex;
  pGridPoint->m_x = x;
  pGridPoint->m_y = y;
}

void CAmbiPiGrid::DumpCoordinates(void)
{
  unsigned int gridPointIndex = 0;
  SGridPoint* pGridPoint;

  while (gridPointIndex < m_numGridPoints) {

    pGridPoint = m_gridPoints + gridPointIndex;
    CLog::Log(LOGDEBUG, "%s - Coordinates, index: %d, x: %d, y: %d", __FUNCTION__, gridPointIndex, pGridPoint->m_x, pGridPoint->m_y);  

    gridPointIndex++;
  }  
}

void CAmbiPiGrid::UpdateSampleRectangles(unsigned int imageWidth, unsigned int imageHeight)
{

  unsigned int gridPointIndex = 0;
  SGridPoint* pGridPoint;

  unsigned int sampleWidth = imageWidth / m_width;
  unsigned int sampleHeight = imageHeight / m_height;

  CLog::Log(LOGDEBUG, "%s - Updating grid sample rectangles for image, width: %d, height: %d, sampleWidth: %d, sampleHeight: %d", __FUNCTION__, imageWidth, imageHeight, sampleWidth, sampleHeight);  

  while (gridPointIndex < m_numGridPoints) {

    pGridPoint = m_gridPoints + gridPointIndex;

    pGridPoint->sampleRect.SetRect(
      pGridPoint->m_x * sampleWidth,
      pGridPoint->m_y * sampleHeight,
      std::min(imageWidth, (pGridPoint->m_x * sampleWidth) + sampleWidth),
      std::min(imageHeight, (pGridPoint->m_y * sampleHeight) + sampleHeight)
    );

    CLog::Log(LOGDEBUG, "%s - updated rectangle, x: %d, y: %d, rect: (left: %f, top: %f, right: %f, bottom: %f)",
      __FUNCTION__, 
      pGridPoint->m_x,
      pGridPoint->m_y,
      pGridPoint->sampleRect.x1,
      pGridPoint->sampleRect.y1,
      pGridPoint->sampleRect.x2,
      pGridPoint->sampleRect.y2
    );  

    gridPointIndex++;
  }
}

