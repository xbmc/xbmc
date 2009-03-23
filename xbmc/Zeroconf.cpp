/*
 *      Copyright (C) 2005-2009 Team XBMC
 *      http://www.xbmc.org
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
 
#include "../guilib/system.h" //HAS_ZEROCONF define

#include "stdafx.h"

#include "Zeroconf.h"
#include "Settings.h"

#ifdef _LINUX
#ifndef __APPLE__
#include "linux/ZeroconfAvahi.h"
#else
//on osx use the native implementation
#include "osx/ZeroconfOSX.h"
#endif
#endif


#ifndef HAS_ZEROCONF
//dummy implementation used if no zeroconf is present
//should be optimized away
class CZeroconfDummy : public CZeroconf
{
  virtual bool doPublishService(const std::string&, const std::string&, const std::string&, unsigned int)
  {
    return false;
  }
  virtual bool doRemoveService(const std::string& fcr_ident){return false;}
  virtual bool doHasService(const std::string& fcr_ident){return false;}
  virtual void doStop(){}
};
#endif

CZeroconf::CZeroconf()
{
}

CZeroconf::~CZeroconf()
{
}

bool CZeroconf::PublishService(const std::string& fcr_identifier,
                               const std::string& fcr_type,
                               const std::string& fcr_name,
                               unsigned int f_port)
{
  return doPublishService(fcr_identifier, fcr_type, fcr_name, f_port);
}

bool CZeroconf::RemoveService(const std::string& fcr_identifier)
{
  return doRemoveService(fcr_identifier);
}

bool CZeroconf::HasService(const std::string& fcr_identifier)
{
  return doHasService(fcr_identifier);
}


void CZeroconf::Stop()
{
  doStop();
}

//
// below is singleton handling stuff
// 
CZeroconf*& CZeroconf::GetrInternalRef()
{
  //use pseudo-meyer singleton to be able to do manual intantiation
  //and to not get bitten by static initialization order effects
  static CZeroconf* slp_instance = 0;
  return slp_instance;
}

CZeroconf*  CZeroconf::GetInstance()
{
  if(GetrInternalRef() == 0)
  {
#ifndef HAS_ZEROCONF
    GetrInternalRef() = new CZeroconfDummy;
#else
#ifdef __APPLE__
    GetrInternalRef() = new CZeroconfOSX;
#elif defined(_LINUX)
    GetrInternalRef() = new CZeroconfAvahi;
#endif
#endif
  }
  return GetrInternalRef();
}

void CZeroconf::ReleaseInstance()
{
  delete GetrInternalRef();
  GetrInternalRef() = 0;
}

