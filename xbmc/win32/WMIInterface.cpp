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

#include "stdafx.h"
#include "WMIInterface.h"
#include "../Util.h"

using namespace std;



CWIN32Wmi::CWIN32Wmi(void)
{
  bconnected = false;
  pclsObj = NULL;
  Connect();
}

CWIN32Wmi::~CWIN32Wmi(void)
{
  Release();
}

bool CWIN32Wmi::Connect()
{
  // Initialize COM. ------------------------------------------

  hres =  CoInitializeEx(0, COINIT_MULTITHREADED);
  if (FAILED(hres))
  {
      return false;                  // Program has failed.
  }

	hres =  CoInitializeSecurity(
        NULL,
        -1,                          // COM authentication
        NULL,                        // Authentication services
        NULL,                        // Reserved
        RPC_C_AUTHN_LEVEL_DEFAULT,   // Default authentication
        RPC_C_IMP_LEVEL_IMPERSONATE, // Default Impersonation
        NULL,                        // Authentication info
        EOAC_NONE,                   // Additional capabilities
        NULL                         // Reserved
        );

	if (FAILED(hres))
  {
      return false;                    // Program has failed.
  }

	pLoc = NULL;

  hres = CoCreateInstance(
      CLSID_WbemLocator,
      0,
      CLSCTX_INPROC_SERVER,
      IID_IWbemLocator, (LPVOID *) &pLoc);

  if (FAILED(hres))
  {
      return false;                 // Program has failed.
  }

	pSvc = NULL;

  // Connect to the root\cimv2 namespace with
  // the current user and obtain pointer pSvc
  // to make IWbemServices calls.
  hres = pLoc->ConnectServer(
       _bstr_t(L"ROOT\\CIMV2"), // Object path of WMI namespace
       NULL,                    // User name. NULL = current user
       NULL,                    // User password. NULL = current
       0,                       // Locale. NULL indicates current
       NULL,                    // Security flags.
       0,                       // Authority (e.g. Kerberos)
       0,                       // Context object
       &pSvc                    // pointer to IWbemServices proxy
       );

  if (FAILED(hres))
  {
      pLoc->Release();
      CoUninitialize();
      return false;                // Program has failed.
  }

	hres = CoSetProxyBlanket(
       pSvc,                        // Indicates the proxy to set
       RPC_C_AUTHN_WINNT,           // RPC_C_AUTHN_xxx
       RPC_C_AUTHZ_NONE,            // RPC_C_AUTHZ_xxx
       NULL,                        // Server principal name
       RPC_C_AUTHN_LEVEL_CALL,      // RPC_C_AUTHN_LEVEL_xxx
       RPC_C_IMP_LEVEL_IMPERSONATE, // RPC_C_IMP_LEVEL_xxx
       NULL,                        // client identity
       EOAC_NONE                    // proxy capabilities
    );

    if (FAILED(hres))
    {
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return false;               // Program has failed.
    }

		pEnumerator = NULL;

    bconnected = true;
    return true;
}

void CWIN32Wmi::Release()
{
  if(pSvc != NULL)
    pSvc->Release();
  if(pLoc != NULL)
    pLoc->Release();
  if(pEnumerator != NULL)
    pEnumerator->Release();
  if(pclsObj != NULL)
    pclsObj->Release();

  CoUninitialize();

  bconnected = false;
  pSvc = NULL;
  pLoc = NULL;
  pEnumerator = NULL;
  pclsObj = NULL;
}

void CWIN32Wmi::testquery()
{
  hres = pSvc->ExecQuery(
      bstr_t("WQL"),
      //bstr_t("SELECT * FROM Win32_NetworkAdapterConfiguration WHERE IPEnabled=TRUE"),
      //bstr_t("SELECT * FROM Win32_NetworkAdapter WHERE PhysicalAdapter=TRUE"),
      bstr_t("SELECT * FROM Win32_NetworkAdapter WHERE Description='Atheros AR5008X Wireless Network Adapter'"),
      WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
      NULL,
      &pEnumerator);

  if (FAILED(hres))
  {
      return;               // Program has failed.
  }
  ULONG uReturn = 0;

  while (pEnumerator)
  {
      HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1,
          &pclsObj, &uReturn);

      if(0 == uReturn)
      {
          break;
      }

      VARIANT vtProp;
      VariantInit(&vtProp);

      // Get the value of the Name property
      //hr = pclsObj->Get(L"Description", 0, &vtProp, 0, 0);

      vtProp.bstrVal = bstr_t("192.168.1.209");
      hr = pclsObj->Put(L"IPAddress",0,&vtProp,0);
      VariantClear(&vtProp);
			//iCpu++;
  }
	pclsObj->Release();
  pclsObj = NULL;
}

std::vector<std::string> CWIN32Wmi::GetWMIStrVector(std::string& strQuery, std::wstring& strProperty)
{
  std::vector<std::string> strResult;
  pEnumerator = NULL;
  pclsObj = NULL;

  if(!bconnected)
    return strResult;

  hres = pSvc->ExecQuery(
      bstr_t("WQL"),
      bstr_t(strQuery.c_str()),
      WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
      NULL,
      &pEnumerator);

  if (FAILED(hres))
  {
      return strResult;               // Program has failed.
  }
  ULONG uReturn = 0;

  while (pEnumerator)
  {
      HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1,
          &pclsObj, &uReturn);

      if(0 == uReturn)
      {
          break;
      }

      VARIANT vtProp;
      VariantInit(&vtProp);

      hr = pclsObj->Get(strProperty.c_str(), 0, &vtProp, 0, 0);
      strResult.push_back(vtProp.bstrVal);
      VariantClear(&vtProp);
  }
  if(pEnumerator != NULL)
    pEnumerator->Release();
  pEnumerator = NULL;
  if(pclsObj != NULL)
	  pclsObj->Release();
  pclsObj = NULL;
  return strResult;
}

std::string CWIN32Wmi::GetWMIString(std::string& strQuery, std::wstring& strProperty)
{
  return GetWMIStrVector(strQuery, strProperty)[0];
}
