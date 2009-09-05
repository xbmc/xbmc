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
#include <libvisualisation.h>
#include <viz_local.h>



/*
 * viz_release(void *p)
 * calls refmem_release on struct pointed to by p
 */
void
viz_release(void *p)
{
  refmem_release(p);
}

/*
* viz_preset_destroy(void)
*
* Scope: PRIVATE (static)
*
* Description:
*
* Destroy and free a preset list structure. This should only be called by refmem_release().
*
* Return Value:
*
* None.
*/
static void
viz_preset_list_destroy(viz_preset_list_t list)
{
  int i;

  if (!list) {
    return;
  }

  for (i=0; i < list->count; ++i) {
    if (list->list[i]) {
      refmem_release(list->list[i]);
    }
    list->list[i] = NULL;
  }

  if (list->list) {
    free(list->list);
  }
}

/*
* viz_preset_list_create(void)
*
* Scope: PUBLIC
*
* Description:
*
* Create a preset list structure and return pointer to the structure
*
* Return Value:
*
* Success: A non-NULL viz_preset_list_t
*
* Failure: A NULL viz_preset_list_t
*/
viz_preset_list_t
viz_preset_list_create(void)
{
  viz_preset_list_t ret;

  refmem_dbg(REFMEM_DEBUG, "%s\n", __FUNCTION__);
  ret = refmem_alloc(sizeof(*ret));
  if(!ret) {
    return(NULL);
  }
  refmem_set_destroy(ret, (refmem_destroy_t)viz_preset_list_destroy);

  ret->list = NULL;
  ret->count = 0;
  return ret;
}

/*
* viz_preset_list_get_item(viz_preset_list_t list, int index)
*
* Scope: PUBLIC
*
* Description:
* 
* Retrieve the preset structure found at position 'index' in the list
* 'list'. Return the preset structure held. Before forgetting
* the reference to this preset, caller must call viz_release().
*
* Return Value:
*
* Success: A non-NULL viz_preset_t 
*
* Failure: A NULL viz_preset_t
*/
viz_preset_t
viz_preset_list_get_item(viz_preset_list_t list, int index)
{
  if (!list) {
    refmem_dbg(REFMEM_ERROR, "%s: NULL preset list\n",
      __FUNCTION__);
    return NULL;
  }
  if(!list->list) {
    refmem_dbg(REFMEM_ERROR, "%s: NULL list\n",
      __FUNCTION__);
    return NULL;
  }
  if ((index < 0) || (index >= list->count)) {
    refmem_dbg(REFMEM_ERROR, "%s: index %d out of range\n",
      __FUNCTION__, index);
    return NULL;
  }

  refmem_hold(list->list[index]);
  return list->list[index];
}

/*
* viz_preset_list_add_item(viz_preset_t item)
*
* Scope: PUBLIC
*
* Description:
*
* Add the preset_list structure 'item' to the preset_list list 'list'
*
* Return Value:
*
* Success: Any non-zero integer.
*
* Failure: 0
*/
int
viz_preset_list_add_item(viz_preset_list_t list, viz_preset_t item)
{
  int c;

  if(!list || !item) {
    refmem_dbg(REFMEM_ERROR, "%s: NULL list or item \n", __FUNCTION__);
    return 0;
  }

  c = list->count;

  list->list = realloc(list->list, (++c * sizeof(viz_preset_t)));
  if (list->list) {
    list->count = c;
  }
  else {
    refmem_dbg(REFMEM_ERROR, "%s: realloc failed for list\n", __FUNCTION__);
    return -1;
  }

  list->list[c-1] = item;
  return 1;
}

/*
* viz_preset_list_get_count(viz_preset_list_t list)
*
* Scope: PUBLIC
*
* Description:
*
* Retrieve the number of elements in the preset_list list structure
* in 'list'.
*
* Return Value:
*
* Success: A number >= 0 indicating the number of items in 'list'
*
* Failure: A number < 0 indicating an error
*/
int
viz_preset_list_get_count(viz_preset_list_t list)
{
  if (!list) {
    refmem_dbg(REFMEM_ERROR, "%s: NULL preset_list list\n",
      __FUNCTION__);
    return -EINVAL;
  }
  return list->count;
}
