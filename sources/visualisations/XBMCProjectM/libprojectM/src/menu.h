/**
 * projectM -- Milkdrop-esque visualisation SDK
 * Copyright (C)2003-2004 projectM Team
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * See 'LICENSE.txt' included within this release
 *
 */

#ifndef _MENU_H
#define _MENU_H

#include "event.h"
#include "common.h"
#include "param_types.h"

#define MENU_DEBUG 0

#define PARAM_ADJ_TYPE 0
#define MENU_LINK_TYPE 1
#define FUNCTION_MODE_TYPE 2

typedef struct MENU_LINK_T {
	char print_string[MAX_TOKEN_SIZE];
	struct MENU_T * sub_menu;
} menu_link_t;

typedef struct PARAM_ADJ_T {
	char print_string[MAX_TOKEN_SIZE];
	param_t * param;
} param_adj_t;	

typedef struct FUNCTION_MODE_T {
	char print_string[MAX_TOKEN_SIZE];
	int (*func_ptr)();
} function_mode_t;	

typedef union MENU_ENTRY_T {
  menu_link_t menu_link;
  param_adj_t param_adj;
  function_mode_t function_mode;
} menu_entry_t;


typedef struct MENU_ITEM_T {
  int menu_entry_type;
  menu_entry_t * menu_entry;
  struct MENU_ITEM_T * up;
  struct MENU_ITEM_T * down;
} menu_item_t;


typedef struct MENU_T {
  menu_item_t * selected_item;
  menu_item_t * start_item;
  menu_item_t * locked_item;
  struct MENU_T * top_menu;
} menu_t;

/* Direction types */
typedef enum {
	UP,
	DOWN,
	LEFT,
	RIGHT,
	PAGEUP,
	PAGEDOWN
} dir_t;

/* Adjustment types */
typedef enum {
	BIG_INC,
	BIG_DEC,
	SMALL_INC,
	SMALL_DEC,
	VERY_BIG_INC,
	VERY_BIG_DEC,
	VERY_SMALL_INC,
	VERY_SMALL_DEC
} adj_t;

typedef enum {
  SHOW,
  HIDE,
  SPECIAL
} display_state;



int switchMenuState(dir_t dir);

int initMenu();
int refreshMenu();
int clearMenu();
int showMenu();
int hideMenu();
void menu_key_handler( projectM_t *PM, projectMEvent event, projectMKeycode key );

#endif /** !_MENU_H */
