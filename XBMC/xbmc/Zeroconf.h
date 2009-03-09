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


#pragma once

#include <string>

/// this class provides support for zeroconf
/// currently it's able to publish xbmc's webserver
/// only OSX implementation currently available 
class CZeroconf
{
public:

    // publishs everything that is available
    void Start();
    
    //reads the port from guisettings and publishs the webserver
    void PublishWebserver();
    //removes published webserver
    void RemoveWebserver();
    //returns the prefix zeroconf should use to publish the webserver (e.g. XBMC)
    //the final name is $(GetWebserverPublishPrefix)@$(HOSTNAME)
    static const std::string& GetWebserverPublishPrefix();
    
    // unpublishs all services
    void Stop();
    
    // class methods
    // access to singleton; singleton gets created on call if not existent
    static CZeroconf* GetInstance();
    // release the singleton; (save to call multiple times)
    static void   ReleaseInstance();
    // returns false if ReleaseInstance() was called befores
    static bool   IsInstantiated() { return  GetInstance() != 0; }

protected:
    //methods to implement
    virtual void doPublishWebserver(int f_port) = 0;
    virtual void doRemoveWebserver() = 0;
    virtual void doStop() = 0;
    
protected:
    //singleton: we don't want to get instantiated nor copied or deleted from outside 
    CZeroconf();
    CZeroconf(const CZeroconf&);
    virtual ~CZeroconf();    
    
private:
    //internal access to singleton instance holder (aka meyer singleton)
    static CZeroconf*& GetrInternalRef();
};
