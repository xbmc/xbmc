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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "projectM.h"

#ifdef MACOS
#include <gl.h>
#else
#include <GL/gl.h>
#endif /** MACOS */
#include "common.h"
#include "fatal.h"
#include "param_types.h"
#include "param.h"
#include "interface_types.h"
#include "splaytree_types.h"
#include "splaytree.h"

#include "init_cond_types.h"
#include "init_cond.h"

#include "glConsole.h"

#include "preset_types.h"
#include "preset.h"
#include "editor.h"
#include "menu.h"
#include "wipemalloc.h"

#define HIGHLIGHT_COLOR 1
#define DEFAULT_COLOR 0
#define LOCKED_COLOR 2

extern preset_t *active_preset;
extern interface_t current_interface;

menu_t * load_waveform_menu();
menu_t * load_augmentations_menu();
menu_t * load_postprocessing_menu();
menu_t * load_motion_menu();
int load_main_menu();

int edit_per_frame_init();
int edit_per_frame_eqn();
int edit_per_pixel_eqn();
int run_bash_shell();


int print_menu_item(menu_item_t * menu_item);
int switch_bool_param(param_t * param);
int adj_float_param(param_t * param, adj_t adj);
int adj_int_param(param_t * param, adj_t adj);
int pursue_menu_link(menu_link_t * menu_link);

int append_menu_item(menu_t * menu, menu_item_t * new_menu_item);

menu_t * new_menu(menu_t * top_menu);
menu_item_t * new_menu_item(int menu_entry_type, menu_entry_t * menu_entry);
menu_entry_t * new_menu_link(char * print_string, menu_t * menu_ptr);


menu_entry_t * new_param_adj(char * print_string, param_t * param);

menu_entry_t * new_function_mode(char * print_string, int (*func_ptr)());

int free_menu_entry(menu_entry_t * menu_entry);
int free_menu_item(menu_item_t * menu_item);
int free_menu(menu_t * menu);

init_cond_t * get_init_cond(param_t * param);

int menu_lprint(char * string, int col);
int init_main_menu();
int destroy_main_menu();

int width, height;

float xmin,ymin,xmax,ymax;


value_t saved_init_val;  /* backups the value of an initial condition */

menu_t * main_menu = NULL; /* Global menu structure */

menu_t * active_menu = NULL; /* the currently loaded menu */

gl_console_t * menu_console = NULL;



display_state menu_display_state = HIDE;

int initMenu() {

  
  

 

  if (MENU_DEBUG) {
	printf("initMenu: loading menu:");
	fflush(stdout);	  
  }	  

  load_main_menu();
  if (MENU_DEBUG) printf(" [done]\n");
  active_menu = main_menu;
  return PROJECTM_SUCCESS;
} 

int showMenu() {

  if ((menu_console = glConsoleCreate(80,24, 81, 25, width*.05, height * .90,0)) == NULL)
    return PROJECTM_FAILURE;

  glConsoleSetFlags(CONSOLE_FLAG_CURSOR_HIDDEN|CONSOLE_FLAG_SCROLL_WRAP_DOWN|CONSOLE_FLAG_SCROLL_WRAP_RIGHT, menu_console);

  active_menu = main_menu;
  main_menu->start_item = main_menu->selected_item;
  menu_display_state = SHOW;
  return PROJECTM_SUCCESS;
}

int hideMenu() {
  menu_display_state = HIDE;
  active_menu = main_menu;
  active_menu->selected_item = active_menu->start_item;
	
  if (active_menu->locked_item != NULL) {
    //	if (active_menu->locked_item->menu_entry_type == PARAM_ADJ_TYPE)
    //	active_menu->locked_item->menu_entry->param_adj.param->init_val = saved_init_val;	  
  	active_menu->locked_item = NULL;  
  }
  
  glConsoleDestroy(menu_console);
  menu_console = NULL;

  return PROJECTM_SUCCESS;
}

int load_main_menu() {
	
	menu_t * waveform_menu;
	menu_t * augmentations_menu;
	menu_t * motion_menu;
	menu_t * postprocessing_menu;
	
	if ((main_menu = new_menu(NULL)) == NULL)
		return PROJECTM_OUTOFMEM_ERROR;
	
	if (MENU_DEBUG) { printf(" [waveform:"); fflush(stdout); } 
	waveform_menu = load_waveform_menu();
	
	if (waveform_menu == NULL) {
		free_menu(main_menu); 
		if (MENU_DEBUG) { printf(" failure]\n"); fflush(stdout); }
		return PROJECTM_FAILURE;
	}
	
	if (MENU_DEBUG) { printf(" success]"); fflush(stdout); }
	
	if (MENU_DEBUG) { printf(" [augmentations:"); fflush(stdout); } 
	augmentations_menu = load_augmentations_menu();
	
	if (augmentations_menu == NULL) {
		free_menu(main_menu);
		free_menu(waveform_menu);		
		if (MENU_DEBUG) { printf(" failure]\n"); fflush(stdout); }
		return PROJECTM_FAILURE;
	}
	
	if (MENU_DEBUG) { printf(" success]"); fflush(stdout); }
	
	if (MENU_DEBUG) { printf(" [postprocessing:"); fflush(stdout); } 
	postprocessing_menu = load_postprocessing_menu();
	
	if (postprocessing_menu == NULL) {
		free_menu(main_menu);
		free_menu(waveform_menu);
		free_menu(augmentations_menu);		
		if (MENU_DEBUG) { printf(" failure]\n"); fflush(stdout); }
		return PROJECTM_FAILURE;
	}
	
	if (MENU_DEBUG) { printf(" success]"); fflush(stdout); }
	
	if (MENU_DEBUG) { printf(" [motion:"); fflush(stdout); } 
	motion_menu = load_motion_menu();
	
	if (motion_menu == NULL) {
		free_menu(main_menu);
		free_menu(waveform_menu);
		free_menu(augmentations_menu);	
		free_menu(postprocessing_menu);		
		if (MENU_DEBUG) { printf(" failure]\n"); fflush(stdout); }
		return PROJECTM_FAILURE;
	}
	
	if (MENU_DEBUG) { printf(" success]"); fflush(stdout); }
	
	append_menu_item(main_menu, new_menu_item(MENU_LINK_TYPE, new_menu_link("--waveform", waveform_menu)));
	append_menu_item(main_menu, new_menu_item(MENU_LINK_TYPE, new_menu_link("--augmentations", augmentations_menu)));
	append_menu_item(main_menu, new_menu_item(MENU_LINK_TYPE, new_menu_link("--motion", motion_menu)));
	append_menu_item(main_menu, new_menu_item(MENU_LINK_TYPE, new_menu_link("--post processing, global effects", postprocessing_menu)));

	append_menu_item(main_menu, new_menu_item(FUNCTION_MODE_TYPE, new_function_mode("edit per_frame equations", edit_per_frame_eqn)));
	append_menu_item(main_menu, new_menu_item(FUNCTION_MODE_TYPE, new_function_mode("edit per_pixel equations", edit_per_pixel_eqn)));
	append_menu_item(main_menu, new_menu_item(FUNCTION_MODE_TYPE, new_function_mode("edit preset initialization code", edit_per_frame_init)));	
	//append_menu_item(main_menu, new_menu_item(FUNCTION_MODE_TYPE, new_function_mode("bash shell", run_bash_shell)));
		
	main_menu->selected_item = main_menu->start_item;	
	
	return PROJECTM_SUCCESS;
} 


menu_t * load_waveform_menu() {
	menu_t * waveform_menu;
	
	if ((waveform_menu = new_menu(main_menu)) == NULL)
		return NULL;
	append_menu_item(waveform_menu, new_menu_item(PARAM_ADJ_TYPE, new_param_adj("wave type", find_param("nWaveMode", active_preset, P_CREATE))));
	append_menu_item(waveform_menu, new_menu_item(PARAM_ADJ_TYPE, new_param_adj("size", find_param("fWaveScale", active_preset, P_CREATE))));
	append_menu_item(waveform_menu, new_menu_item(PARAM_ADJ_TYPE, new_param_adj("smoothing", find_param("fWaveSmoothing", active_preset, P_CREATE))));
    
	append_menu_item(waveform_menu, new_menu_item(PARAM_ADJ_TYPE, new_param_adj("mystery parameter", find_param("wave_mystery", active_preset, P_CREATE))));
	append_menu_item(waveform_menu, new_menu_item(PARAM_ADJ_TYPE, new_param_adj("opacity", find_param("fWaveAlpha", active_preset, P_CREATE))));
	append_menu_item(waveform_menu, new_menu_item(PARAM_ADJ_TYPE, new_param_adj("position (x)", find_param("wave_x", active_preset, P_CREATE))));
	append_menu_item(waveform_menu, new_menu_item(PARAM_ADJ_TYPE, new_param_adj("position (y)", find_param("wave_y", active_preset, P_CREATE))));
	append_menu_item(waveform_menu, new_menu_item(PARAM_ADJ_TYPE, new_param_adj("color (red)", find_param("wave_r", active_preset, P_CREATE))));
	append_menu_item(waveform_menu, new_menu_item(PARAM_ADJ_TYPE, new_param_adj("color (green)", find_param("wave_g", active_preset, P_CREATE))));
	append_menu_item(waveform_menu, new_menu_item(PARAM_ADJ_TYPE, new_param_adj("color (blue)", find_param("wave_b", active_preset, P_CREATE))));
	
	append_menu_item(waveform_menu, new_menu_item(PARAM_ADJ_TYPE, new_param_adj("use dots", find_param("bWaveDots", active_preset, P_CREATE))));
	
	append_menu_item(waveform_menu, new_menu_item(PARAM_ADJ_TYPE, new_param_adj("draw thick", find_param("bWaveThick", active_preset, P_CREATE))));

	append_menu_item(waveform_menu, new_menu_item(PARAM_ADJ_TYPE, new_param_adj("modulate opacity by volume", find_param("bModWaveAlphaByVolume", active_preset, P_CREATE))));
	append_menu_item(waveform_menu, new_menu_item(PARAM_ADJ_TYPE, new_param_adj("mod. lower threshold", find_param("fModWaveAlphaStart", active_preset, P_CREATE))));
	append_menu_item(waveform_menu, new_menu_item(PARAM_ADJ_TYPE, new_param_adj("mod. uppper threshold", find_param("fModWaveAlphaEnd", active_preset, P_CREATE))));
	
	append_menu_item(waveform_menu, new_menu_item(PARAM_ADJ_TYPE, new_param_adj("additive drawing", find_param("bAdditiveWaves", active_preset, P_CREATE))));
	append_menu_item(waveform_menu, new_menu_item(PARAM_ADJ_TYPE, new_param_adj("color brightening", find_param("bMaximizeWaveColor", active_preset, P_CREATE))));
	waveform_menu->selected_item = waveform_menu->start_item;	
	
	return waveform_menu;
} 


menu_t * load_augmentations_menu() {
	menu_t * augmentations_menu;
	
	if ((augmentations_menu = new_menu(main_menu)) == NULL)
		return NULL;
	
	append_menu_item(augmentations_menu, new_menu_item(PARAM_ADJ_TYPE, new_param_adj("outer border thickness", find_param("ob_size", active_preset, P_CREATE))));	
	append_menu_item(augmentations_menu, new_menu_item(PARAM_ADJ_TYPE, new_param_adj(" color (red)", find_param("ob_r", active_preset, P_CREATE))));
	append_menu_item(augmentations_menu, new_menu_item(PARAM_ADJ_TYPE, new_param_adj(" color (green)", find_param("ob_g", active_preset, P_CREATE))));
	append_menu_item(augmentations_menu, new_menu_item(PARAM_ADJ_TYPE, new_param_adj(" color (blue)", find_param("ob_b", active_preset, P_CREATE))));
	append_menu_item(augmentations_menu, new_menu_item(PARAM_ADJ_TYPE, new_param_adj(" opacity", find_param("ob_a", active_preset, P_CREATE))));

	
	append_menu_item(augmentations_menu, new_menu_item(PARAM_ADJ_TYPE, new_param_adj("inner border thickness", find_param("ib_size", active_preset, P_CREATE))));	
	append_menu_item(augmentations_menu, new_menu_item(PARAM_ADJ_TYPE, new_param_adj(" color (red)", find_param("ib_r", active_preset, P_CREATE))));
	append_menu_item(augmentations_menu, new_menu_item(PARAM_ADJ_TYPE, new_param_adj(" color (green)", find_param("ib_g", active_preset, P_CREATE))));
	append_menu_item(augmentations_menu, new_menu_item(PARAM_ADJ_TYPE, new_param_adj(" color (blue)", find_param("ib_b", active_preset, P_CREATE))));
	append_menu_item(augmentations_menu, new_menu_item(PARAM_ADJ_TYPE, new_param_adj(" opacity", find_param("ib_a", active_preset, P_CREATE))));
	
	append_menu_item(augmentations_menu, new_menu_item(PARAM_ADJ_TYPE, new_param_adj("motion vector opacity", find_param("mv_a", active_preset, P_CREATE))));
	append_menu_item(augmentations_menu, new_menu_item(PARAM_ADJ_TYPE, new_param_adj("num. mot. vector (X)", find_param("mv_x", active_preset, P_CREATE))));
	append_menu_item(augmentations_menu, new_menu_item(PARAM_ADJ_TYPE, new_param_adj("num. mot. vector (Y)", find_param("mv_y", active_preset, P_CREATE))));
	append_menu_item(augmentations_menu, new_menu_item(PARAM_ADJ_TYPE, new_param_adj("offset (X)", find_param("mv_dx", active_preset, P_CREATE))));
	append_menu_item(augmentations_menu, new_menu_item(PARAM_ADJ_TYPE, new_param_adj("offset (Y)", find_param("mv_dy", active_preset, P_CREATE))));
	append_menu_item(augmentations_menu, new_menu_item(PARAM_ADJ_TYPE, new_param_adj("trail length", find_param("mv_l", active_preset, P_CREATE))));
	append_menu_item(augmentations_menu, new_menu_item(PARAM_ADJ_TYPE, new_param_adj("color (red)", find_param("mv_r", active_preset, P_CREATE))));
	append_menu_item(augmentations_menu, new_menu_item(PARAM_ADJ_TYPE, new_param_adj("color (green)", find_param("mv_g", active_preset, P_CREATE))));
	append_menu_item(augmentations_menu, new_menu_item(PARAM_ADJ_TYPE, new_param_adj("color (blue)", find_param("mv_b", active_preset, P_CREATE))));


	augmentations_menu->selected_item = augmentations_menu->start_item;	
	
	return augmentations_menu;
}

/* Loads the motion menu, a sub menu of the main menu */
menu_t * load_motion_menu() {
	menu_t * motion_menu;
	
	if ((motion_menu = new_menu(main_menu)) == NULL)
		return NULL;
	
	  
	append_menu_item(motion_menu, new_menu_item(PARAM_ADJ_TYPE, new_param_adj("zoom amount", find_param("zoom", active_preset, P_CREATE))));
	append_menu_item(motion_menu, new_menu_item(PARAM_ADJ_TYPE, new_param_adj("zoom exponent", find_param("zoomexp", active_preset, P_CREATE))));
	append_menu_item(motion_menu, new_menu_item(PARAM_ADJ_TYPE, new_param_adj("rotation amount", find_param("rot", active_preset, P_CREATE))));
	append_menu_item(motion_menu, new_menu_item(PARAM_ADJ_TYPE, new_param_adj("rot., center of (X)", find_param("cx", active_preset, P_CREATE))));
	append_menu_item(motion_menu, new_menu_item(PARAM_ADJ_TYPE, new_param_adj("rot., center of (Y)", find_param("cy", active_preset, P_CREATE))));
	append_menu_item(motion_menu, new_menu_item(PARAM_ADJ_TYPE, new_param_adj("translation (X)", find_param("dx", active_preset, P_CREATE))));
	append_menu_item(motion_menu, new_menu_item(PARAM_ADJ_TYPE, new_param_adj("translation (Y)", find_param("dy", active_preset, P_CREATE))));
	append_menu_item(motion_menu, new_menu_item(PARAM_ADJ_TYPE, new_param_adj("scaling (X)", find_param("sx", active_preset, P_CREATE))));
	append_menu_item(motion_menu, new_menu_item(PARAM_ADJ_TYPE, new_param_adj("scaling (Y)", find_param("sy", active_preset, P_CREATE))));
	
	motion_menu->selected_item = motion_menu->start_item;
	
	return motion_menu;
} 

/* Loads the post processing menu, a sub menu of the main, menu */
menu_t * load_postprocessing_menu() {
	menu_t * postprocessing_menu;
	if ((postprocessing_menu = new_menu(main_menu)) == NULL)
		return NULL;
	
	append_menu_item(postprocessing_menu, new_menu_item(PARAM_ADJ_TYPE, new_param_adj("sustain level", find_param("fDecay", active_preset, P_CREATE))));

    append_menu_item(postprocessing_menu, new_menu_item(PARAM_ADJ_TYPE, new_param_adj("darken center", find_param("bDarkenCenter", active_preset, P_CREATE))));

	append_menu_item(postprocessing_menu, new_menu_item(PARAM_ADJ_TYPE, new_param_adj("gamma adjustment", find_param("fDecay", active_preset, P_CREATE))));
	append_menu_item(postprocessing_menu, new_menu_item(PARAM_ADJ_TYPE, new_param_adj("video echo: alpha", find_param("fVideoEchoAlpha", active_preset, P_CREATE))));
	append_menu_item(postprocessing_menu, new_menu_item(PARAM_ADJ_TYPE, new_param_adj("video echo: scale", find_param("fVideoEchoZoom", active_preset, P_CREATE))));
	append_menu_item(postprocessing_menu, new_menu_item(PARAM_ADJ_TYPE, new_param_adj("video echo: orientation", find_param("nVideoEchoOrientation", active_preset, P_CREATE))));
	append_menu_item(postprocessing_menu, new_menu_item(PARAM_ADJ_TYPE, new_param_adj("texture wrapping", find_param("bTexWrap", active_preset, P_CREATE)))); 
	append_menu_item(postprocessing_menu, new_menu_item(PARAM_ADJ_TYPE, new_param_adj("darken filter", find_param("bDarken", active_preset, P_CREATE))));
 append_menu_item(postprocessing_menu, new_menu_item(PARAM_ADJ_TYPE, new_param_adj("brighten filter", find_param("bBrighten", active_preset, P_CREATE))));
 append_menu_item(postprocessing_menu, new_menu_item(PARAM_ADJ_TYPE, new_param_adj("solarize filter", find_param("bSolarize", active_preset, P_CREATE))));
 append_menu_item(postprocessing_menu, new_menu_item(PARAM_ADJ_TYPE, new_param_adj("invert filter", find_param("bInvert", active_preset, P_CREATE))));

	
	postprocessing_menu->selected_item = postprocessing_menu->start_item;	
	
	return postprocessing_menu;
} 

int refreshMenu() {
	
	menu_item_t * menu_item;

	if (menu_display_state == HIDE) {
	  return PROJECTM_SUCCESS;
	}

	if (menu_display_state == SPECIAL) {
	  glConsoleDraw(menu_console);
	  glConsoleClearBuffer(menu_console);
	  return PROJECTM_SUCCESS;
	}
	  
	menu_item = active_menu->start_item;	
	
	glConsoleClearBuffer(menu_console);

	while (menu_item) {
	  print_menu_item(menu_item);
	  menu_item = menu_item->down;
	}

	glConsoleDraw(menu_console);
	return PROJECTM_SUCCESS;
}

int destroyMenu() {
 
  if (main_menu != NULL)
	  free_menu(main_menu);
  
  main_menu = NULL;
  return PROJECTM_SUCCESS;
}

/* Prints a line to the opengl screen, second entry is the color, work in progress */
int menu_lprint(char * string, int col) {
	
  if (active_menu == NULL)
    return PROJECTM_FAILURE;
  
  if (string == NULL)
    return PROJECTM_FAILURE;
  
  if (col == HIGHLIGHT_COLOR) {
    glConsoleSetBGColor(CONSOLE_WHITE, menu_console);
    glConsoleSetFGColor(CONSOLE_BLUE, menu_console);
  }
  else if (col == LOCKED_COLOR) {
    glConsoleSetBGColor(CONSOLE_WHITE, menu_console);
    glConsoleSetFGColor(CONSOLE_RED, menu_console);
  }
  else {
    glConsoleSetBGColor(CONSOLE_BLACK, menu_console);
    glConsoleSetFGColor(CONSOLE_WHITE, menu_console);
  }

  glConsolePrintString(string, menu_console); 
  glConsolePrintString("\n", menu_console);
  return PROJECTM_SUCCESS;
}



int print_menu_item(menu_item_t * menu_item) {

	param_adj_t param_adj;
	char string[MAX_TOKEN_SIZE];
	int col;
	init_cond_t * init_cond;

	if (menu_item == NULL)
		return PROJECTM_FAILURE;
	if (active_menu == NULL)
		return PROJECTM_FAILURE;
	
	if (active_menu->locked_item == menu_item)
	  col = LOCKED_COLOR;
	else if (active_menu->selected_item == menu_item) 
	  col = HIGHLIGHT_COLOR;			
	else
	  col = DEFAULT_COLOR;
	switch (menu_item->menu_entry_type) {
	
		case PARAM_ADJ_TYPE:
		  param_adj = menu_item->menu_entry->param_adj;

		  switch(param_adj.param->type) {

		  case P_TYPE_BOOL:

		    if ((init_cond = get_init_cond(param_adj.param)) == NULL)
		      sprintf(string, "%s ?", param_adj.print_string);
		    else if (init_cond->init_val.bool_val)
		      sprintf(string, "%s [ON]", param_adj.print_string);
		    else
		      sprintf(string, "%s [OFF]", param_adj.print_string);
		    break;
		  case P_TYPE_INT:
		    if ((init_cond = get_init_cond(param_adj.param)) == NULL)
		      sprintf(string, "%s ?", param_adj.print_string);
		    else
		      sprintf(string, "%s %d", param_adj.print_string, init_cond->init_val.int_val);
		    break;
		  case P_TYPE_DOUBLE:
		    if ((init_cond = get_init_cond(param_adj.param)) == NULL)
		      sprintf(string, "%s ?", param_adj.print_string);
		    else
		      sprintf(string, "%s %f", param_adj.print_string, init_cond->init_val.float_val);
		    break;

		  default: /* unknown paramater type */
		    return PROJECTM_FAILURE;
		  }
		  return menu_lprint(string, col);
		case FUNCTION_MODE_TYPE: 
			return menu_lprint(menu_item->menu_entry->function_mode.print_string, col);
		case MENU_LINK_TYPE:
			return menu_lprint(menu_item->menu_entry->menu_link.print_string, col);
		default:
			if (MENU_DEBUG) printf("print_menu_item: invalid menu entry type, val = %d\n", 
										menu_item->menu_entry_type);		
		return PROJECTM_FAILURE; 
	}

	return PROJECTM_SUCCESS;
}	

/* switchMenuState: changes to another menu item according to the passed direction */
int switchMenuState(dir_t dir) {
  
  menu_item_t * new_menu_item;
  param_adj_t param_adj;
  init_cond_t * init_cond;

  if (active_menu == NULL)
	  return PROJECTM_FAILURE;
  
  /* Case on the direction of the key press */	
  switch (dir) {
    
  case PAGEUP:
    if(active_menu->locked_item != NULL) {
      if (active_menu->locked_item->menu_entry_type == PARAM_ADJ_TYPE)
	return adj_float_param(active_menu->locked_item->menu_entry->param_adj.param, BIG_INC);
    }   
    
    /* Force to top of menu */
    active_menu->selected_item = active_menu->start_item;
    return PROJECTM_SUCCESS;
  case PAGEDOWN:
    if(active_menu->locked_item != NULL) {
      if (active_menu->locked_item->menu_entry_type == PARAM_ADJ_TYPE)
	return adj_float_param(active_menu->locked_item->menu_entry->param_adj.param, BIG_DEC);
    }   
    
	  /* Force to bottom of menu */
    while((new_menu_item = active_menu->selected_item->down) != NULL)
      active_menu->selected_item = new_menu_item;
    return PROJECTM_SUCCESS;
    
  case UP:
    if(active_menu->locked_item != NULL) {
      if (active_menu->locked_item->menu_entry_type == PARAM_ADJ_TYPE)
	return adj_float_param(active_menu->locked_item->menu_entry->param_adj.param, SMALL_INC);
    }   
    
    /* Is this the first item of the menu ? */
    if ((new_menu_item = active_menu->selected_item->up) == NULL)
      return PROJECTM_SUCCESS;
    active_menu->selected_item = new_menu_item;
    return PROJECTM_SUCCESS;
  case DOWN:
	  if(active_menu->locked_item != NULL) {
	    if (active_menu->locked_item->menu_entry_type == PARAM_ADJ_TYPE)
	      return adj_float_param(active_menu->locked_item->menu_entry->param_adj.param, SMALL_DEC);
	  }
	  
	  /* Is this the last item of the menu ? */
	  if ((new_menu_item = active_menu->selected_item->down) == NULL)
	    return PROJECTM_SUCCESS;
	  active_menu->selected_item = new_menu_item;
	  return PROJECTM_SUCCESS;
  case LEFT:
    /* Left key unlocks the highlighted item */
    if (active_menu->locked_item != NULL) {
      /* The left arrow cancels the initial condition adjustment */
      if (active_menu->locked_item->menu_entry_type == PARAM_ADJ_TYPE) {
	//active_menu->locked_item->menu_entry->param_adj.param->init_val = saved_init_val;
      }			
      active_menu->locked_item = NULL;
      return PROJECTM_SUCCESS;
    }
    /* Otherwise go up a menu if it isn't the top menu */
    if (active_menu->top_menu == NULL)
	    return PROJECTM_SUCCESS;
    active_menu = active_menu->top_menu;
    return PROJECTM_SUCCESS;
  case RIGHT:
    switch (active_menu->selected_item->menu_entry_type) {
    case MENU_LINK_TYPE:
      return pursue_menu_link(&active_menu->selected_item->menu_entry->menu_link);				
    case PARAM_ADJ_TYPE:
      param_adj = active_menu->selected_item->menu_entry->param_adj;
    
	
   	
	if (active_menu->locked_item == NULL) {

        if ((init_cond = get_init_cond(param_adj.param)) == NULL)
           return PROJECTM_FAILURE;


	  /* Save previous initial condition value */
	  //saved_init_val = init_cond->init_val;

	  /* If a bool parameter, just switch its value and quit */	  
	  if (param_adj.param->type == P_TYPE_BOOL) {
	    init_cond->init_val.bool_val = !init_cond->init_val.bool_val;
	    return PROJECTM_SUCCESS;
	  }

	  /* Otherwise, lock this parameter so the user can adjust it */
	  active_menu->locked_item = active_menu->selected_item;

       }
	
	/* Unlock the item, but keep the changed initial value intact */
	else {
	  active_menu->locked_item = NULL;			  
	}
       
	break;
  case FUNCTION_MODE_TYPE:
    return active_menu->selected_item->menu_entry->function_mode.func_ptr();
  default:
    return PROJECTM_FAILURE;
  }			
 default:
   return PROJECTM_FAILURE;		
}

return PROJECTM_SUCCESS;
}

/* Creates a new menu */
menu_t * new_menu(menu_t * top_menu) {
	
	menu_t * menu;
	
	if ((menu = (menu_t*)wipemalloc(sizeof(menu_t))) == NULL)
		return NULL;
	
	menu->top_menu = top_menu;
	menu->start_item = menu->selected_item = NULL;
	menu->locked_item = NULL;
	return menu;
}

/* Frees a menu */
int free_menu(menu_t * menu) {
	menu_item_t * menu_item, * backup;
	
	if (menu == NULL)
		return PROJECTM_SUCCESS;
	
	menu_item = menu->start_item;

	/* Now we free every entry in this menu */
	while (menu_item) {
		backup = menu_item->down;
		
		/* If a menu link, free the sub menu it points to */
		if (menu_item->menu_entry_type == MENU_LINK_TYPE) {
			free_menu(menu_item->menu_entry->menu_link.sub_menu);						
		}		
		
		/* Now free the menu item */
		free_menu_item(menu_item);
		
		/* Set menu item to the next item in the list */
		menu_item = backup;
	}
	
	/* Finally, we free the menu struct itself */
	free(menu);
        menu = NULL;
	
	/* Finished, return success */
	return PROJECTM_SUCCESS;
} 

/* Creates a new menu link type */
menu_entry_t * new_menu_link(char * print_string, menu_t * menu_ptr) {

	menu_entry_t * menu_entry;
	menu_link_t menu_link;
	
	/* Argument Checks */
	if (print_string == NULL)
		return NULL;
	if (menu_ptr == NULL)
		return NULL;

	
	/* Allocate memory for the menu entry */
	if ((menu_entry = (menu_entry_t*)wipemalloc(sizeof(menu_entry_t))) == NULL)
		return NULL;
	
	
	/* Copy the string parameter */
	memset(menu_link.print_string, 0, MAX_TOKEN_SIZE);
	strncpy(menu_link.print_string, print_string, MAX_TOKEN_SIZE);
	
	menu_link.sub_menu = menu_ptr;
	
	menu_entry->menu_link = menu_link;
	
	return menu_entry;
}


/* Creates a new parameter adjustment entry */
menu_entry_t * new_param_adj(char * print_string, param_t * param) {

	menu_entry_t * menu_entry;
	param_adj_t param_adj;
	
	/* Argument Checks */
	if (print_string == NULL)
		return NULL;

	if (param == NULL) {
		if (MENU_DEBUG) printf("new_param_adj: passed a null parameter!\n");
		return NULL;
	}
	
	/* Allocate memory for the menu entry */
	if ((menu_entry = (menu_entry_t*)wipemalloc(sizeof(menu_entry_t))) == NULL)
		return NULL;
		
	/* Copy the string parameter */
	memset(param_adj.print_string, 0, MAX_TOKEN_SIZE);
	strncpy(param_adj.print_string, print_string, MAX_TOKEN_SIZE);
	
	param_adj.param = param;
	menu_entry->param_adj = param_adj;
	
	return menu_entry;
}

/* Appends a menu item */
int append_menu_item(menu_t * menu, menu_item_t * new_menu_item) {
	
	menu_item_t * menu_item;
	
	/* Argument checks */
	if (menu == NULL)
		return PROJECTM_FAILURE;
	
	if (new_menu_item == NULL)
		return PROJECTM_FAILURE;

       
	/* Menu is empty, insert here */
	if (menu->start_item == NULL) {
		menu->start_item = new_menu_item;
		new_menu_item->up = NULL;
		return PROJECTM_SUCCESS;
	}
      
	menu_item = menu->start_item;
	
	/* Traverse down the menu */
	while (menu_item->down) {
		menu_item = menu_item->down;
	}
	
	/* Insert the item at the end */
	menu_item->down = new_menu_item;
	new_menu_item->up = menu_item;
	
	return PROJECTM_SUCCESS;

}

/* Creates a new menu item, up and down entries are null by default */
menu_item_t * new_menu_item(int menu_entry_type, menu_entry_t * menu_entry) {

	menu_item_t * menu_item;					
	
 	/* Argument check */						
	if (menu_entry == NULL)
		return NULL;

	/* Out of memory check */
	if ((menu_item = (menu_item_t*)wipemalloc(sizeof(menu_item_t))) == NULL) {
		return NULL;
	}
	
	menu_item->menu_entry_type = menu_entry_type;
	menu_item->menu_entry = menu_entry;
	menu_item->up = NULL;
	menu_item->down = NULL;

	return menu_item;	
}

/* Frees a menu item */
int free_menu_item(menu_item_t * menu_item) {

	if (menu_item == NULL)
		return PROJECTM_OK;
	
	free(menu_item->menu_entry);
	free(menu_item);

        menu_item = NULL;
	
	return PROJECTM_SUCCESS;
}	


/* Creates a new editor mode */
menu_entry_t * new_function_mode(char * print_string, int (*func_ptr)()) {

	menu_entry_t * menu_entry;
	function_mode_t function_mode;
	
	/* Argument Checks */
	if (print_string == NULL)
		return NULL;

	/* Allocate memory for the menu entry */
	if ((menu_entry = (menu_entry_t*)wipemalloc(sizeof(menu_entry_t))) == NULL)
		return NULL;
	
	
	/* Copy the string parameter */
	memset(function_mode.print_string, 0, MAX_TOKEN_SIZE);
	strncpy(function_mode.print_string, print_string, MAX_TOKEN_SIZE);
	
	function_mode.func_ptr = func_ptr;
	menu_entry->function_mode = function_mode;
	
	return menu_entry;
}	

 
/* Pursues a link in a menu */
int pursue_menu_link(menu_link_t * menu_link) {

	if (menu_link == NULL)
			return PROJECTM_FAILURE;
	
	active_menu = menu_link->sub_menu;
	
	return PROJECTM_SUCCESS;
}


int edit_per_pixel_eqn() {
  hideMenu();
  current_interface = EDITOR_INTERFACE;
  loadEditor(active_preset->per_pixel_eqn_string_buffer,(void (*)()) reloadPerPixel, 80, 24, 140, 60, 0, 0);
  return PROJECTM_SUCCESS;
}

int edit_per_frame_eqn() {
  hideMenu();
  current_interface = EDITOR_INTERFACE;
  loadEditor(active_preset->per_frame_eqn_string_buffer, (void (*)())reloadPerFrame,80,24,140,60,0,0);
  return PROJECTM_SUCCESS;
}

int edit_per_frame_init() {
  hideMenu();
  current_interface = EDITOR_INTERFACE;
  loadEditor(active_preset->per_frame_init_eqn_string_buffer,(void (*)()) reloadPerFrameInit,80,24,140,60,0,0);
  return PROJECTM_SUCCESS;
}


int run_bash_shell() {

  printf("setting menu state to special\n");
  menu_display_state = SPECIAL;
  //  glConsoleStartShell(menu_console);

  //menu_display_state = SHOW;

  return PROJECTM_SUCCESS;
}



/* Adjust a float parameter */
int adj_float_param(param_t * param, adj_t adj) {

  float inc_val;
  init_cond_t * init_cond = NULL;

  if (param == NULL)
    return PROJECTM_FAILURE;

  if (param->type == P_TYPE_INT)
    return (adj_int_param(param, adj));

  if ((init_cond = (init_cond_t*)splay_find(param->name, active_preset->init_cond_tree)) == NULL)
    return PROJECTM_FAILURE;
    
  switch (adj) {
    
  case VERY_SMALL_INC:
    inc_val = .001;
    break;
  case SMALL_INC:
    inc_val = .01;
    break;
  case BIG_INC:
    inc_val = .1;
    break;
  case VERY_BIG_INC:
    inc_val = 1.0;
    break;
  case VERY_SMALL_DEC:
    inc_val = -.001;
    break;
  case SMALL_DEC:
    inc_val = -.01;
    break;
  case BIG_DEC:
    inc_val = -0.1;
    break;
  case VERY_BIG_DEC:
    inc_val = -1.0;
    break;

  default:
    return PROJECTM_FAILURE;
  }

  /* Beyond upper bounds, normalize to maximum value */
  if ((init_cond->init_val.float_val + inc_val) > param->upper_bound.float_val)
    init_cond->init_val.float_val = param->upper_bound.float_val;

  else if ((init_cond->init_val.float_val + inc_val) < param->lower_bound.float_val)
    init_cond->init_val.float_val = param->lower_bound.float_val;

  else
    init_cond->init_val.float_val += inc_val;

  return PROJECTM_SUCCESS;
}

/* Adjust an integer parameter */
int adj_int_param(param_t * param, adj_t adj) {
    
  int inc_val;
  init_cond_t * init_cond = NULL;
  
  if (param == NULL)
    return PROJECTM_FAILURE;
  
  if ((init_cond = (init_cond_t*)splay_find(param->name, active_preset->init_cond_tree)) == NULL) 
   return PROJECTM_FAILURE;
  
 switch (adj) {
   
   
 case VERY_SMALL_INC:
   inc_val = 1;
   break;
  case SMALL_INC:
    inc_val = 1;
    break;
  case BIG_INC:
    inc_val = 1;
    break;
  case VERY_BIG_INC:
    inc_val = 5;
    break;

  case VERY_SMALL_DEC:
    inc_val = -1;
    break;
  case SMALL_DEC:
    inc_val = -1;
    break;
  case BIG_DEC:
    inc_val = -1;
    break;
  case VERY_BIG_DEC:
    inc_val = -5;
    break;

  default:
    return PROJECTM_FAILURE;
  }

  /* Beyond upper bounds, normalize to maximum value */
  if ((init_cond->init_val.int_val + inc_val) > param->upper_bound.int_val)
    init_cond->init_val.int_val = param->upper_bound.int_val;

  /* Below lower bounds, normalize to lowest value */
  else if ((init_cond->init_val.int_val + inc_val) < param->lower_bound.int_val)
    init_cond->init_val.int_val = param->lower_bound.int_val;

  else
    init_cond->init_val.int_val += inc_val;

  return PROJECTM_SUCCESS;
}

void menu_key_handler( projectM_t *PM, projectMEvent event, projectMKeycode key ) {
  
	switch( event ) {
	case PROJECTM_KEYDOWN:	 		    
	  switch(key)
	    {
	    case PROJECTM_K_f:
	      
	      if (PM->fullscreen==1)
    {PM->vw=PM->wvw;PM->vh=PM->wvh;PM->fullscreen=0;}
  else{PM->vw=PM->fvw; PM->vh=PM->fvh;PM->fullscreen=1;}

//  init_display(PM->vw,PM->vh,PM->fullscreen);


	      break;
            case PROJECTM_K_n:
	      switchPreset(ALPHA_NEXT, HARD_CUT);
	      break;
	    case PROJECTM_K_r:
	      switchPreset(RANDOM_NEXT, HARD_CUT);
	      break;
	    case PROJECTM_K_p:
	      switchPreset(ALPHA_PREVIOUS, HARD_CUT);
	      break;
	    case PROJECTM_K_a:      	      	    
	      break;
	    case PROJECTM_K_z:      	     	      	     
	      break;
	    case PROJECTM_K_0:      
	      //nWaveMode=0;
	      break;
	    case PROJECTM_K_6:
	      //nWaveMode=6;
	      break;
	    case PROJECTM_K_7:
	      //nWaveMode=7;
	      break;
	    case PROJECTM_K_UP:
	      switchMenuState(UP);
	      break;
	    case PROJECTM_K_RETURN:
	    case PROJECTM_K_RIGHT:
	      switchMenuState(RIGHT);
	      break;
	    case PROJECTM_K_LEFT:
	      switchMenuState(LEFT);
	      break;
	    case PROJECTM_K_DOWN:
	      switchMenuState(DOWN);
	      break;
	    case PROJECTM_K_PAGEUP:
	      switchMenuState(PAGEUP);
	      break;
	    case PROJECTM_K_PAGEDOWN:
	      switchMenuState(PAGEDOWN);
	      break;
	    case PROJECTM_K_ESCAPE:
	    case PROJECTM_K_m:
	      hideMenu();
	      current_interface = DEFAULT_INTERFACE;
	      break;	     
	    case PROJECTM_K_t:
	      break;
	    default:
	      break;
	    }
	}
   



}

/* Finds / Creates (if necessary) initial condition associated with passed parameter */
init_cond_t * get_init_cond(param_t * param) {

  init_cond_t * init_cond;
  value_t init_val;

  if (param == NULL)
    return NULL;

  if ((init_cond = (init_cond_t*)(splay_find(param->name, active_preset->init_cond_tree))) == NULL) {

    if (param->type == P_TYPE_BOOL)
      init_val.bool_val = 0;
    
    else if (param->type == P_TYPE_INT)
      init_val.int_val = *(int*)param->engine_val;

    else if (param->type == P_TYPE_DOUBLE)
      init_val.float_val = *(float*)param->engine_val;

    /* Create new initial condition */
    if ((init_cond = new_init_cond(param, init_val)) == NULL)
      return NULL;
    
    /* Insert the initial condition into this presets tree */
    if (splay_insert(init_cond, init_cond->param->name, active_preset->init_cond_tree) < 0) {
      free_init_cond(init_cond);
      return NULL;
    }
    
  }

  return init_cond;

}
