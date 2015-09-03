/*
 *      Copyright (C) 2005-2013 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "system.h"
#if defined(HAVE_LIBCEC)
#include "PeripheralBusCEC.h"
#include "peripherals/Peripherals.h"
#include "DynamicDll.h"

#include <libcec/cec.h>

using namespace PERIPHERALS;
using namespace CEC;
using namespace std;

class DllLibCECInterface
{
public:
  virtual ~DllLibCECInterface() {}
  virtual ICECAdapter* CECInitialise(libcec_configuration *configuration)=0;
  virtual void*        CECDestroy(ICECAdapter *adapter)=0;
};

class PERIPHERALS::DllLibCEC : public DllDynamic, DllLibCECInterface
{
  DECLARE_DLL_WRAPPER(DllLibCEC, DLL_PATH_LIBCEC)

  DEFINE_METHOD1(ICECAdapter*, CECInitialise, (libcec_configuration *p1))
  DEFINE_METHOD1(void*       , CECDestroy,    (ICECAdapter *p1))

  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD_RENAME(CECInitialise,  CECInitialise)
    RESOLVE_METHOD_RENAME(CECDestroy, CECDestroy)
  END_METHOD_RESOLVE()
};

CPeripheralBusCEC::CPeripheralBusCEC(CPeripherals *manager) :
    CPeripheralBus("PeripBusCEC", manager, PERIPHERAL_BUS_CEC),
    m_dll(new DllLibCEC),
    m_cecAdapter(NULL)
{
  m_iRescanTime = 5000;
  if (!m_dll->Load() || !m_dll->IsLoaded())
  {
    delete m_dll;
    m_dll = NULL;
  }
  else
  {
    m_cecAdapter = m_dll->CECInitialise(&m_configuration);
  }
}

CPeripheralBusCEC::~CPeripheralBusCEC(void)
{
  if (m_dll && m_cecAdapter)
    m_dll->CECDestroy(m_cecAdapter);
  delete m_dll;
}

bool CPeripheralBusCEC::PerformDeviceScan(PeripheralScanResults &results)
{
  if (!m_dll || !m_cecAdapter)
    return false;

  cec_adapter_descriptor deviceList[10];
  int8_t iFound = m_cecAdapter->DetectAdapters(deviceList, 10, NULL, true);

  for (uint8_t iDevicePtr = 0; iDevicePtr < iFound; iDevicePtr++)
  {
    PeripheralScanResult result(m_type);
    result.m_iVendorId   = deviceList[iDevicePtr].iVendorId;
    result.m_iProductId  = deviceList[iDevicePtr].iProductId;
    result.m_strLocation = deviceList[iDevicePtr].strComName;
    result.m_type        = PERIPHERAL_CEC;

    // override the bus type, so users don't have to reconfigure their adapters
    switch(deviceList[iDevicePtr].adapterType)
    {
    case ADAPTERTYPE_P8_EXTERNAL:
    case ADAPTERTYPE_P8_DAUGHTERBOARD:
      result.m_mappedBusType = PERIPHERAL_BUS_USB;
      break;
    case ADAPTERTYPE_RPI:
      result.m_mappedBusType = PERIPHERAL_BUS_RPI;
      /** the Pi's adapter cannot be removed, no need to rescan */
      m_bNeedsPolling = false;
      break;
    default:
      break;
    }

    result.m_iSequence = GetNumberOfPeripheralsWithId(result.m_iVendorId, result.m_iProductId);
    if (!results.ContainsResult(result))
      results.m_results.push_back(result);
  }
  return true;
}

#endif
