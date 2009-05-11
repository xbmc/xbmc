/*
 *  Copyright (C) 2004-2006, Eric Lund
 *  http://www.mvpmc.org/
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/**
 * \file addon_local.h
 * Local definitions which are internal to libaddon
 */

#ifndef __ADDON_LOCAL_H_
#define __ADDON_LOCAL_H

#include "addon.h"

/* An individual setting */
struct addon_setting {
  addon_setting_type_t type;
  char *id;
  char *label;
  char *enable;
  char *lvalues;
};

/* A list of settings */
struct addon_settings {
  addon_setting_t *settings_list;
  long settings_count;
};


#endif /* __ADDON_LOCAL_H */
