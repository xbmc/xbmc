#pragma once
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

#ifndef LIBADDON_HELPER_H
#define LIBADDON_HELPER_H

#include <vector>
#include <string>
#include "xbmc_addon_dll.h"

bool XBMC_register_me(void *hdl);
void XBMC_log(const addon_log_t loglevel, const char *format, ... );
void XBMC_status_callback(const addon_status_t status, const char* msg);
bool XBMC_get_setting(std::string settingName, void *settingValue);
char *XBMC_get_addon_directory();
char *XBMC_get_user_directory();


#endif /* LIBADDON_HELPER_H */

