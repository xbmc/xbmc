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
#include "stdafx.h"
#include "../guilib/system.h" //HAS_ZEROCONF define
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
  virtual void doStop(){}
};
#endif

CZeroconf::CZeroconf():m_started(false)
{
//  if(g_guiSettings.GetBool("servers.zeroconf")){
    Start();
//  }
}

CZeroconf::~CZeroconf()
{
}

bool CZeroconf::PublishService(const std::string& fcr_identifier,
                               const std::string& fcr_type,
                               const std::string& fcr_name,
                               unsigned int f_port)
{
  CZeroconf::PublishInfo info = {fcr_type, fcr_name, f_port};
  std::pair<tServiceMap::const_iterator, bool> ret = m_service_map.insert(std::make_pair(fcr_identifier, info));
  if(!ret.second) //identifier exists
    return false;
  if(m_started)
    return doPublishService(fcr_identifier, fcr_type, fcr_name, f_port);
  //not yet started, so its just queued
  return true;
}

bool CZeroconf::RemoveService(const std::string& fcr_identifier)
{
  tServiceMap::iterator it = m_service_map.find(fcr_identifier);
  if(it == m_service_map.end())
    return false;
  m_service_map.erase(it);
  if(m_started)
    return doRemoveService(fcr_identifier);
  else 
    return true;
}

bool CZeroconf::HasService(const std::string& fcr_identifier)
{
  return (m_service_map.find(fcr_identifier) != m_service_map.end());
}

void CZeroconf::Start()
{
  if(m_started)
    return;
  m_started = true;
  for(tServiceMap::const_iterator it = m_service_map.begin(); it != m_service_map.end(); ++it){
    doPublishService(it->first, it->second.type, it->second.name, it->second.port);
  }
}

void CZeroconf::Stop()
{
  if(!m_started)
    return;
  doStop();
  m_started = false;
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

