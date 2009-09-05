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

/**
 * \file addon_local.h
 * Local definitions which are internal to libaddon
 */

#ifndef __ADDON_LOCAL_H_
#define __ADDON_LOCAL_H_

enum addon_log {
  REFMEM_DEBUG,
  REFMEM_INFO,
  REFMEM_NOTICE,
  REFMEM_ERROR
};

/* An individual setting */
struct addon_setting {
  addon_setting_type_t type;
  char *id;
  char *label;
  char *enable;
  char *lvalues;
  int valid;
};

/* A list of settings */
struct addon_settings {
  addon_setting_t *settings_list;
  long settings_count;
};

#endif /* __ADDON_LOCAL_H */
