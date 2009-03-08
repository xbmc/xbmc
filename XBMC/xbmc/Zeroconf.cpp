/*
 * Zeroconf Support for XBMC
 * Copyright (c) 2009 TeamXBMC
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "stdafx.h"

#include "Zeroconf.h"
#include "Settings.h"

#ifdef __APPLE__
//on osx use the native implementation
#include "osx/ZeroconfOSX.h"
#endif

CZeroconf::CZeroconf(){
  
}

CZeroconf::~CZeroconf(){
}

void CZeroconf::Start(){
    PublishWebserver();
}

void CZeroconf::PublishWebserver(){
#ifdef HAS_WEB_SERVER
    if(g_guiSettings.GetBool("servers.webserver")) {
        int port = atoi(g_guiSettings.GetString("servers.webserverport"));
        doPublishWebserver(port);
    }
#endif    
}

void CZeroconf::RemoveWebserver(){
#ifdef HAS_WEB_SERVER
    doRemoveWebserver();
#endif    
}

const std::string& CZeroconf::GetWebserverPublishPrefix(){
    static std::string name = "XBMC";
    return name;
}

void CZeroconf::Stop(){
    doStop();
}

//
// below is singleton handling stuff
// 
CZeroconf*& CZeroconf::GetrInternalRef(){
    //use pseudo-meyer singleton to be able to do manual intantiation
    //and to not get bitten by static initialization order effects
    static CZeroconf* slp_instance = 0;
    return slp_instance;
}

CZeroconf*  CZeroconf::GetInstance(){
    if(GetrInternalRef() == 0){
#ifdef __APPLE__
        GetrInternalRef() = new CZeroconfOSX;
#else
#error 
#endif  
    }
    return GetrInternalRef();

};

void CZeroconf::ReleaseInstance(){
    delete GetrInternalRef();
    GetrInternalRef() = 0;
};
