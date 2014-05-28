#pragma once
/*
 *      Copyright (C) 2012-2013 Team XBMC
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

#ifndef ADDONS_DLLADDON_H_INCLUDED
#define ADDONS_DLLADDON_H_INCLUDED
#include "DllAddon.h"
#endif

#ifndef ADDONS_INCLUDE_XBMC_PVR_TYPES_H_INCLUDED
#define ADDONS_INCLUDE_XBMC_PVR_TYPES_H_INCLUDED
#include "include/xbmc_pvr_types.h"
#endif


class DllPVRClient : public DllAddon<PVRClient, PVR_PROPERTIES>
{
  // this is populated via Macro calls in DllAddon.h
};

