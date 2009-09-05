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
* \file preset.c
* Functions that operate on individual preset_list
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
* viz_preset_destroy(viz_preset_t p)
*
* Scope: PRIVATE (static)
*
* Description
*
* Destroy the preset structure pointed to by 'p' ad release
* it's storage. This should only be called by refmem_release().
*
* Return Value:
*
* None.
*/
static void
viz_preset_destroy(viz_preset_t p)
{
  refmem_dbg(REFMEM_DEBUG, "%s {\n", __FUNCTION__);
  if (!p) {
    refmem_dbg(REFMEM_DEBUG, "%s }!a\n", __FUNCTION__);
    return;
  }
  if (p->name) {
    refmem_release(p->name);
  }
  refmem_dbg(REFMEM_DEBUG, "%s }\n", __FUNCTION__);
}

/*
* viz_preset_create(void)
*
* Scope: PUBLIC
*
* Description:
*
* Create a new viz_preset structure and return pointer to the structure
*
* Return Value:
*
* Success: A non-NULL viz_preset_t
*
* Failure: A NULL viz_preset_t
*/
viz_preset_t
viz_preset_create(void)
{
  viz_preset_t ret = refmem_alloc(sizeof(*ret));

  refmem_dbg(REFMEM_DEBUG, "%s\n", __FUNCTION__);
  if(!ret) {
    return NULL;
  }
  refmem_set_destroy(ret, (refmem_destroy_t)viz_preset_destroy);

  ret->name = NULL;
  return ret;
}

/*
* viz_preset_name(viz_preset_t preset)
*
*
* Scope: PUBLIC
*
* Retrieves the 'name' field of a preset structure
*
* Return Value:
*
* Success: A ref counted char* to field 'name'
*
* Failure: NULL char*
*/
char *
viz_preset_name(viz_preset_t preset)
{
  if (!preset) {
    refmem_dbg(REFMEM_ERROR, "%s: NULL preset structure\n",
      __FUNCTION__);
    return NULL;
  }
  return preset->name;
}

/*
* viz_preset_set_name(viz_preset_t preset, viz_preset_type_t name)
*
*
* Scope: PUBLIC
*
* Modifies the 'name' field of a preset structure, with a duplicate
* of the string pointed to by 'name'
*
* Will release the previous element if non-NULL
*
* Return Value:
*
* Success: Any non-zero integer.
*
* Failure: 0
*/
int
viz_preset_set_type(viz_preset_t preset, char* name)
{
  if (!preset) {
    refmem_dbg(REFMEM_ERROR, "%s: NULL preset structure\n",
      __FUNCTION__);
    return 0;
  }

  if (preset->name) {
    refmem_release(preset->name);
  }

  preset->name = refmem_strdup(name);
  return (int) refmem_hold(preset->name);
}