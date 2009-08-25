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
 * \file settings.c
 * Functions that operate on lists of settings
 */
#include <sys/types.h>
#include <stdlib.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif
#include <errno.h>
#include <mvp_refmem.h>
#include <mvp_debug.h>
#include <libaddon.h>
#include <addon_local.h>

/*
 * addon_release(void* p)
 * calls refmem_release on struct pointed to by p
 */
void
addon_release(void* p)
{
  refmem_release(p);
}

/*
 * addon_settings_destroy(void)
 *
 * Scope: PRIVATE (static)
 *
 * Description:
 *
 * Destroy and free a settings list structure. This should only be called by refmem_release().
 *
 * Return Value:
 *
 * None.
 */
static void
addon_settings_destroy(addon_settings_t list)
{
  int i;

  if (!list) {
    return;
  }

  for (i=0; i < list->settings_count; ++i) {
    if (list->settings_list[i]) {
      refmem_release(list->settings_list[i]);
    }
    list->settings_list[i] = NULL;
  }

  if (list->settings_list) {
    free(list->settings_list);
  }
}

/*
 * addon_settings_create(void)
 *
 * Scope: PUBLIC
 *
 * Description:
 *
 * Create a settings list structure and return pointer to the structure
 *
 * Return Value:
 *
 * Success: A non-NULL addon_settings_t
 *
 * Failure: A NULL addon_settings_t
 */
addon_settings_t
addon_settings_create(void)
{
  addon_settings_t ret;

 printf("YADS");
 refmem_dbg(MVP_DBG_DEBUG, "%s\n", __FUNCTION__);
 ret = refmem_alloc(sizeof(*ret));
 if(!ret) {
   return(NULL);
 }
 refmem_set_destroy(ret, (refmem_destroy_t)addon_settings_destroy);

 ret->settings_list = NULL;
 ret->settings_count = 0;
 return ret;
}

/*
 * addon_settings_get_item(addon_settings_t list, int index)
 *
 * Scope: PUBLIC
 *
 * Description:
 * 
 * Retrieve the setting structure found at index 'index' in the list
 * in 'list'. Return the setting structure held. Before forgetting
 * the reference to this setting, caller must call refmem_release().
 *
 * Return Value:
 *
 * Success: A non-NULL addon_setting_t 
 *
 * Failure: A NULL addon_setting_t
 */
addon_setting_t
addon_settings_get_item(addon_settings_t list, int index)
{
  if (!list) {
    refmem_dbg(MVP_DBG_ERROR, "%s: NULL settings list\n",
      __FUNCTION__);
    return NULL;
  }
  if(!list->settings_list) {
    refmem_dbg(MVP_DBG_ERROR, "%s: NULL list\n",
      __FUNCTION__);
    return NULL;
  }
  if ((index < 0) || (index >= list->settings_count)) {
    refmem_dbg(MVP_DBG_ERROR, "%s: index %d out of range\n",
      __FUNCTION__, index);
    return NULL;
  }

  refmem_hold(list->settings_list[index]);
  return list->settings_list[index];
}

/*
 * addon_settings_add_item(addon_setting_t item)
 *
 * Scope: PUBLIC
 *
 * Description:
 *
 * Add the settings structure 'item' to the settings list 'list'
 *
 * Return Value:
 *
 * Success: NULL integer
 *
 * Failure: A non-NULL integer
 */
int
addon_settings_add_item(addon_settings_t list, addon_setting_t item)
{
  int c;

  if(!list || !item) {
    refmem_dbg(MVP_DBG_ERROR, "%s: NULL list or item \n", __FUNCTION__);
    return -1;
  }

  c = list->settings_count;

  list->settings_list = realloc(list->settings_list, (++c * sizeof(addon_setting_t)));
  if (list->settings_list) {
    list->settings_count = c;
  }
  else {
    refmem_dbg(MVP_DBG_ERROR, "%s: realloc failed for list\n", __FUNCTION__);
    return -1;
  }

  list->settings_list[c-1] = item;
  return 0;
}

/*
 * addon_settings_get_count(addon_settings_t list)
 *
 * Scope: PUBLIC
 *
 * Description:
 *
 * Retrieve the number of elements in the settings list structure
 * in 'list'.
 *
 * Return Value:
 *
 * Success: A number >= 0 indicating the number of items in 'list'
 *
 * Failure: -errno
 */
int
addon_settings_get_count(addon_settings_t list)
{
  if (!list) {
    refmem_dbg(MVP_DBG_ERROR, "%s: NULL settings list\n",
      __FUNCTION__);
    return -EINVAL;
  }
  return list->settings_count;
}

