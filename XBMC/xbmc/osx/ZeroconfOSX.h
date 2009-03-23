#pragma once
/*
 *      Copyright (C) 2005-2008 Team XBMC
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

#include <memory>
#include "Zeroconf.h"

class CZeroconfOSX : public CZeroconf
{
public:
  CZeroconfOSX();
  ~CZeroconfOSX();
protected:
  //implement base CZeroConf interface
  bool doPublishService(const std::string& fcr_identifier,
                        const std::string& fcr_type,
                        const std::string& fcr_name,
                        unsigned int f_port);

  bool doRemoveService(const std::string& fcr_ident);

  //doHas is ugly ...
  bool doHasService(const std::string& fcr_ident);

  virtual void doStop();

private:
  //another indirection with pimpl
  //CZeroconfOSXData stores the actual (objective-c-) data
  class CZeroconfOSXData;
  std::auto_ptr<CZeroconfOSXData> mp_data;
};
