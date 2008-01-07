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
#ifdef WIN32
#include "win32-dirent.h"
#else
#include <dirent.h>
#endif /** WIN32 */
#include <time.h>

#include "projectM.h"

#include "common.h"
#include "fatal.h"

#include "preset_types.h"
#include "preset.h"

#include "parser.h"

#include "expr_types.h"
#include "eval.h"

#include "splaytree_types.h"
#include "splaytree.h"
#include "tree_types.h"

#include "per_frame_eqn_types.h"
#include "per_frame_eqn.h"

#include "per_pixel_eqn_types.h"
#include "per_pixel_eqn.h"

#include "init_cond_types.h"
#include "init_cond.h"

#include "param_types.h"
#include "param.h"

#include "func_types.h"
#include "func.h"

#include "custom_wave_types.h"
#include "custom_wave.h"

#include "custom_shape_types.h"
#include "custom_shape.h"

#include "idle_preset.h"
#include "wipemalloc.h"

/* The maximum number of preset names loaded into buffer */
#define MAX_PRESETS_IN_DIR 50000
extern projectM_t *PM;

extern int per_frame_eqn_count;
extern int per_frame_init_eqn_count;
//extern int custom_per_frame_eqn_count;

extern splaytree_t * builtin_param_tree;

extern preset_t *active_preset;
extern preset_t *old_preset;
extern line_mode_t line_mode;

preset_t *preset_hack;


FILE * write_stream = NULL;


int preset_index = -1;

preset_t * load_preset(const char * pathname);
int is_valid_extension(const struct dirent* ent);
int load_preset_file(const char * pathname, preset_t * preset);
int close_preset(preset_t * preset);

int write_preset_name(FILE * fs);
int write_per_pixel_equations(FILE * fs);
int write_per_frame_equations(FILE * fs);
int write_per_frame_init_equations(FILE * fs);
int write_init_conditions(FILE * fs);
void load_init_cond(param_t * param);
void load_init_conditions( preset_t *preset);
void write_init(init_cond_t * init_cond);
int init_idle_preset();
int destroy_idle_preset();
void load_custom_wave_init_conditions(preset_t *preset);
void load_custom_wave_init(custom_wave_t * custom_wave);

void load_custom_shape_init_conditions(preset_t *preset);
void load_custom_shape_init(custom_shape_t * custom_shape);

/* loadPresetDir: opens the directory buffer
   denoted by 'dir' to load presets */
   
int loadPresetDir(char * dir) {
	/* we no longer do anything here and instead look in PM->presetURL in switchPreset
		this allows us to find new preset files on the fly */

  /* Start the prefix index right before the first entry, so next preset
     starts at the top of the list */
//#define PRESET_KLUDGE
#ifndef PRESET_KLUDGE
  preset_index = -1;
#else
  /** KLUDGE */
  preset_index = 30;
#endif

  /* Start the first preset */
  switchPreset(RANDOM_NEXT, HARD_CUT);
  
  return PROJECTM_SUCCESS;
}

/* closePresetDir: closes the current
   preset directory buffer */

int closePresetDir() {

	/* because we don't open we don't have to close ;) */
  destroyPresetLoader();

  return PROJECTM_SUCCESS;
}

/* switchPreset: loads the next preset from the directory stream.
   loadPresetDir() must be called first. This is a
   sequential load function */

int switchPreset(switch_mode_t switch_mode, int cut_type) {

  preset_t * new_preset = 0;
	
  int switch_index;
  int sindex = 0;
  int slen = 0;

#ifdef DEBUG
    if ( debugFile != NULL ) {
        fprintf( debugFile, "switchPreset(): in\n" );
        fflush( debugFile );
      }
#endif

	switch (switch_mode) {	  
	case ALPHA_NEXT:
		preset_index = switch_index = preset_index + 1;
		break;
	case ALPHA_PREVIOUS:
		preset_index = switch_index = preset_index - 1;
		break;  
	case RANDOM_NEXT:
		switch_index = rand();
		break;
	case RESTART_ACTIVE:
		switch_index = preset_index;
		break;
	default:
		return PROJECTM_FAILURE;
	}

	// iterate through the presetURL directory looking for the next entry 
	{
		struct dirent** entries;
		int dir_size = scandir(PM->presetURL, &entries, is_valid_extension, alphasort);
		if (dir_size > 0) {
			int i;
			
			switch_index %= dir_size;
			if (switch_index < 0) switch_index += dir_size;			
			
			for (i = 0; i < dir_size; ++i) {
				if (switch_index == i) {
					// matching entry
					const size_t len = strlen(PM->presetURL);
					char* path = (char *) malloc(len + strlen(entries[i]->d_name) + 2);
					if (path) {					
						strcpy(path, PM->presetURL);
						if (len && ((path[len - 1] != '/')||(path[len - 1] != '\\'))) {
							strcat(path + len, "/");
						}
						strcat(path + len, entries[i]->d_name);
						
						new_preset = load_preset(path);
                                                new_preset->index = switch_index;
						free(path);
						
						// we must keep iterating to free the remaining entries
					}
				}
				free(entries[i]);
			}
			free(entries);
		}
	}
	
	if (!new_preset) {
		switchToIdlePreset();
		return PROJECTM_ERROR;
	}


  /* Closes a preset currently loaded, if any */
  if ((active_preset != NULL) && (active_preset != idle_preset)) {
        close_preset(active_preset);
    }

  /* Sets global active_preset pointer */
  active_preset = new_preset;

#ifndef PANTS
  /** Split out the preset name from the path */
  slen = strlen( new_preset->file_path );
  sindex = slen;
  while ( new_preset->file_path[sindex] != WIN32_PATH_SEPARATOR && 
          new_preset->file_path[sindex] != UNIX_PATH_SEPARATOR && sindex > 0 ) {
    sindex--;
  }
  sindex++;
  if ( PM->presetName != NULL ) {
    free( PM->presetName );
    PM->presetName = NULL;
  }
  PM->presetName = (char *)wipemalloc( sizeof( char ) * (slen - sindex + 1) );
  strncpy( PM->presetName, new_preset->file_path + sindex, slen - sindex );
  PM->presetName[slen - sindex] = '\0';
#endif

  /* Reinitialize the engine variables to sane defaults */
  projectM_resetengine( PM );

  /* Add any missing initial conditions */
  load_init_conditions(active_preset);

  /* Add any missing initial conditions for each wave */
  load_custom_wave_init_conditions(active_preset);

/* Add any missing initial conditions for each shape */
  load_custom_shape_init_conditions(active_preset);

  /* Need to evaluate the initial conditions once */
  evalInitConditions(active_preset);
 evalCustomWaveInitConditions(active_preset);
  evalCustomShapeInitConditions(active_preset);
  //  evalInitPerFrameEquations();
  return PROJECTM_SUCCESS;
}

/* Loads a specific preset by absolute path */
int loadPresetByFile(char * filename) {

  preset_t * new_preset;
 
  /* Finally, load the preset using its actual path */
  if ((new_preset = load_preset(filename)) == NULL) {
#ifdef PRESET_DEBUG
	printf("loadPresetByFile: failed to load preset!\n");
#endif
	return PROJECTM_ERROR;	  
  }

  /* Closes a preset currently loaded, if any */
  if ((active_preset != NULL) && (active_preset != idle_preset))
    close_preset(active_preset); 

  /* Sets active preset global pointer */
  active_preset = new_preset;

  /* Reinitialize engine variables */
  projectM_resetengine( PM);

 
  /* Add any missing initial conditions for each wave */
  load_custom_wave_init_conditions(active_preset);

 /* Add any missing initial conditions for each wave */
  load_custom_shape_init_conditions(active_preset);

  /* Add any missing initial conditions */
  load_init_conditions(active_preset);
  
  /* Need to do this once for menu */
  evalInitConditions(active_preset);
  //  evalPerFrameInitEquations();
  return PROJECTM_SUCCESS;

}

int init_idle_preset() {

  preset_t * preset;
    /* Initialize idle preset struct */
  if ((preset = (preset_t*)wipemalloc(sizeof(preset_t))) == NULL)
    return PROJECTM_FAILURE;

  
  strncpy(preset->name, "idlepreset", strlen("idlepreset"));

  /* Initialize equation trees */
  preset->init_cond_tree = create_splaytree((int (*)(void*,void*))compare_string, (void*(*)(void*))copy_string, (void(*)(void*))free_string);
  preset->user_param_tree = create_splaytree((int (*)(void*,void*))compare_string,(void*(*)(void*)) copy_string, (void(*)(void*))free_string);
  preset->per_frame_eqn_tree = create_splaytree((int (*)(void*,void*))compare_int,(void*(*)(void*)) copy_int, (void(*)(void*))free_int);
  preset->per_pixel_eqn_tree = create_splaytree((int (*)(void*,void*))compare_int, (void*(*)(void*))copy_int, (void(*)(void*))free_int);
  preset->per_frame_init_eqn_tree = create_splaytree((int (*)(void*,void*))compare_string,(void*(*)(void*)) copy_string, (void(*)(void*))free_string);
  preset->custom_wave_tree = create_splaytree((int (*)(void*,void*))compare_int, (void*(*)(void*))copy_int, (void(*)(void*))free_int);
  preset->custom_shape_tree = create_splaytree((int (*)(void*,void*))compare_int,(void*(*)(void*)) copy_int, (void(*)(void*))free_int);
 
  /* Set file path to dummy name */  
  strncpy(preset->file_path, "IDLE PRESET", MAX_PATH_SIZE-1);
  
  /* Set initial index values */
  preset->per_pixel_eqn_string_index = 0;
  preset->per_frame_eqn_string_index = 0;
  preset->per_frame_init_eqn_string_index = 0;
  memset(preset->per_pixel_flag, 0, sizeof(int)*NUM_OPS);
  
  /* Clear string buffers */
  memset(preset->per_pixel_eqn_string_buffer, 0, STRING_BUFFER_SIZE);
  memset(preset->per_frame_eqn_string_buffer, 0, STRING_BUFFER_SIZE);
  memset(preset->per_frame_init_eqn_string_buffer, 0, STRING_BUFFER_SIZE);

  idle_preset = preset;
  
  return PROJECTM_SUCCESS;
}

int destroy_idle_preset() {

  return close_preset(idle_preset);
  
}

/* initPresetLoader: initializes the preset
   loading library. this should be done before
   any parsing */
int initPresetLoader(projectM_t *PM) {

  /* Initializes the builtin parameter database */
  init_builtin_param_db(PM);

  /* Initializes the builtin function database */
  init_builtin_func_db();
	
  /* Initializes all infix operators */
  init_infix_ops();

  /* Set the seed to the current time in seconds */
  srand(time(NULL));

  /* Initialize the 'idle' preset */
  init_idle_preset();

 

  projectM_resetengine( PM);

//  active_preset = idle_preset;
    PM->presetName = NULL;
    switchToIdlePreset();
  load_init_conditions(active_preset);

  /* Done */
#ifdef PRESET_DEBUG
    printf("initPresetLoader: finished\n");
#endif
  return PROJECTM_SUCCESS;
}

/* Sort of experimental code here. This switches
   to a hard coded preset. Useful if preset directory
   was not properly loaded, or a preset fails to parse */

void switchToIdlePreset() {

    if ( idle_preset == NULL ) {
        return;
      }

  /* Idle Preset already activated */
  if (active_preset == idle_preset)
    return;


  /* Close active preset */
  if (active_preset != NULL)
    close_preset(active_preset);

  /* Sets global active_preset pointer */
  active_preset = idle_preset;

    /** Stash the preset name */
    if ( PM->presetName != NULL ) {
        free( PM->presetName );
      }
    PM->presetName = (char *)wipemalloc( sizeof( char ) * 5 );
    strncpy( PM->presetName, "IDLE", 4 );
    PM->presetName[4] = '\0';

  /* Reinitialize the engine variables to sane defaults */
  projectM_resetengine( PM);

  /* Add any missing initial conditions */
  load_init_conditions(active_preset);

  /* Need to evaluate the initial conditions once */
  evalInitConditions(active_preset);

}

/* destroyPresetLoader: closes the preset
   loading library. This should be done when 
   projectM does cleanup */

int destroyPresetLoader() {
  
  if ((active_preset != NULL) && (active_preset != idle_preset)) {	
  	close_preset(active_preset);      
  }	

  active_preset = NULL;
  
  destroy_idle_preset();
  destroy_builtin_param_db();
  destroy_builtin_func_db();
  destroy_infix_ops();

  return PROJECTM_SUCCESS;

}

/* load_preset_file: private function that loads a specific preset denoted
   by the given pathname */
int load_preset_file(const char * pathname, preset_t * preset) { 
  FILE * fs;
  int retval;
    int lineno;

  if (pathname == NULL)
	  return PROJECTM_FAILURE;
  if (preset == NULL)
	  return PROJECTM_FAILURE;
  
  /* Open the file corresponding to pathname */
  if ((fs = fopen(pathname, "rb")) == 0) {
#if defined(PRESET_DEBUG) && defined(DEBUG)
    if ( debugFile != NULL ) {
        fprintf( debugFile,"load_preset_file: loading of file %s failed!\n", pathname);
      }
#endif
    return PROJECTM_ERROR;	
  }

#if defined(PRESET_DEBUG) && defined(DEBUG)
    if ( debugFile != NULL ) {
        fprintf( debugFile,"load_preset_file: file stream \"%s\" opened successfully\n", pathname);
      }
#endif

  /* Parse any comments */
  if (parse_top_comment(fs) < 0) {
#if defined(PRESET_DEBUG) && defined(DEBUG)
    if ( debugFile != NULL ) {
        fprintf(debugFile,"load_preset_file: no left bracket found...\n");
      }
#endif
    fclose(fs);
    return PROJECTM_FAILURE;
  }
  
  /* Parse the preset name and a left bracket */
  if (parse_preset_name(fs, preset->name) < 0) {
#if defined(PRESET_DEBUG) && defined(DEBUG)
    if ( debugFile != NULL ) {
        fprintf(debugFile,"load_preset_file: loading of preset name in file \"%s\" failed\n", pathname);
      }
#endif
    fclose(fs);
    return PROJECTM_ERROR;
  }
  
#if defined(PRESET_DEBUG) && defined(DEBUG)
    if ( debugFile != NULL ) {
        fprintf(debugFile,"load_preset_file: preset \"%s\" parsed\n", preset->name);
      }
#endif

  /* Parse each line until end of file */
    lineno = 0;
#if defined(PRESET_DEBUG) && defined(DEBUG)
    if ( debugFile != NULL ) {
        fprintf(debugFile,"load_preset_file: beginning line parsing...\n");
      }
#endif
  while ((retval = parse_line(fs, preset)) != EOF) {
    if (retval == PROJECTM_PARSE_ERROR) {
	line_mode = NORMAL_LINE_MODE;
#if defined(PRESET_DEBUG) && defined(DEBUG)
    if ( debugFile != NULL ) {
        fprintf(debugFile,"load_preset_file: parse error in file \"%s\": line %d\n", pathname,lineno);
      }
#endif
    }
    lineno++;
  }
  
#if defined(PRESET_DEBUG) && defined(DEBUG)
    if ( debugFile != NULL ) {
        fprintf(debugFile,"load_preset_file: finished line parsing successfully\n"); 
        fflush( debugFile );
      }
#endif

  /* Now the preset has been loaded.
     Evaluation calls can be made at appropiate
     times in the frame loop */
  
  fclose(fs);
   
#if defined(PRESET_DEBUG) && defined(DEBUG)
    if ( debugFile != NULL ) {
        fprintf(debugFile,"load_preset_file: file \"%s\" closed, preset ready\n", pathname);
      }
#endif
  return PROJECTM_SUCCESS;
  
}

void evalInitConditions(preset_t *preset) {
 splay_traverse((void (*)(void*))eval_init_cond, preset->per_frame_init_eqn_tree);
 
}

void evalPerFrameEquations(preset_t *preset) {
  splay_traverse((void (*)(void*))eval_init_cond, preset->init_cond_tree);
  splay_traverse((void (*)(void*))eval_per_frame_eqn, preset->per_frame_eqn_tree);
}

/* Returns nonzero if string 'name' contains .milk or
   (the better) .prjm extension. Not a very strong function currently */
int is_valid_extension(const struct dirent* ent) {
	const char* ext = 0;
	
	if (!ent) return FALSE;
	
	ext = strrchr(ent->d_name, '.');
	if (!ext) ext = ent->d_name;
	
	if (0 == strcasecmp(ext, MILKDROP_FILE_EXTENSION)) return TRUE;
	if (0 == strcasecmp(ext, PROJECTM_FILE_EXTENSION)) return TRUE;

	return FALSE;
}

/* Private function to close a preset file */
int close_preset(preset_t * preset) {

#if defined(PRESET_DEBUG) && defined(DEBUG)
    if ( debugFile != NULL ) {
        fprintf( debugFile, "close_preset(): in\n" );
        fflush( debugFile );
      }
#endif

  if (preset == NULL)
    return PROJECTM_FAILURE;

  splay_traverse((void (*)(void*))free_init_cond, preset->init_cond_tree);
  destroy_splaytree(preset->init_cond_tree);
  
  splay_traverse((void (*)(void*))free_init_cond, preset->per_frame_init_eqn_tree);
  destroy_splaytree(preset->per_frame_init_eqn_tree);
  
  splay_traverse((void (*)(void*))free_per_pixel_eqn, preset->per_pixel_eqn_tree);
  destroy_splaytree(preset->per_pixel_eqn_tree);
  
  splay_traverse((void (*)(void*))free_per_frame_eqn, preset->per_frame_eqn_tree);
  destroy_splaytree(preset->per_frame_eqn_tree);
  
  splay_traverse((void (*)(void*))free_param, preset->user_param_tree);
  destroy_splaytree(preset->user_param_tree);
  
  splay_traverse((void (*)(void*))free_custom_wave, preset->custom_wave_tree);
  destroy_splaytree(preset->custom_wave_tree);

  splay_traverse((void (*)(void*))free_custom_shape, preset->custom_shape_tree);
  destroy_splaytree(preset->custom_shape_tree);

  free(preset); 
  preset = NULL;

#if defined(PRESET_DEBUG) && defined(DEBUG)
    if ( debugFile != NULL ) {
        fprintf( debugFile, "close_preset(): out\n" );
        fflush( debugFile );
      }
#endif

  return PROJECTM_SUCCESS;

}

void reloadPerPixel(char *s, preset_t * preset) {
  
  int slen;

  if (s == NULL)
    return;

  if (preset == NULL)
    return;

  /* Clear previous per pixel equations */
  splay_traverse((void (*)(void*))free_per_pixel_eqn, preset->per_pixel_eqn_tree);
  destroy_splaytree(preset->per_pixel_eqn_tree);
  preset->per_pixel_eqn_tree = create_splaytree((int (*)(void*,void*))compare_int, (void* (*)(void*))copy_int, (void (*)(void*))free_int);

  /* Convert string to a stream */
#if !defined(MACOS) && !defined(WIN32)
	{
		FILE* fs = fmemopen (s, strlen(s), "r");
		char c;
		
		while ((c = fgetc(fs)) != EOF) {
			ungetc(c, fs);
			parse_per_pixel_eqn(fs, preset, 0);
		}

		fclose(fs);
	}
#else
printf( "reloadPerPixel()\n" );
#endif

  /* Clear string space */
  memset(preset->per_pixel_eqn_string_buffer, 0, STRING_BUFFER_SIZE);

  /* Compute length of string */
  slen = strlen(s);

  /* Copy new string into buffer */
  strncpy(preset->per_pixel_eqn_string_buffer, s, slen);

  /* Yet again no bounds checking */
  preset->per_pixel_eqn_string_index = slen;

  /* Finished */
 
  return;
}

/* Obviously unwritten */
void reloadPerFrameInit(char *s, preset_t * preset) {

}

void reloadPerFrame(char * s, preset_t * preset) {

  int slen;
  int eqn_count = 1;

  if (s == NULL)
    return;

  if (preset == NULL)
    return;

  /* Clear previous per frame equations */
  splay_traverse((void (*)(void*))free_per_frame_eqn, preset->per_frame_eqn_tree);
  destroy_splaytree(preset->per_frame_eqn_tree);
  preset->per_frame_eqn_tree = create_splaytree((int (*)(void*,void*))compare_int,(void* (*)(void*)) copy_int, (void (*)(void*))free_int);

  /* Convert string to a stream */
#if !defined(MACOS) && !defined(WIN32)
	{
		FILE* fs = fmemopen (s, strlen(s), "r");
		char c;
		
		while ((c = fgetc(fs)) != EOF) {
			per_frame_eqn_t * per_frame;
			ungetc(c, fs);
			if ((per_frame = parse_per_frame_eqn(fs, eqn_count, preset)) != NULL) {
				splay_insert(per_frame, &eqn_count, preset->per_frame_eqn_tree);
				eqn_count++;
			}
		}
		fclose(fs);
	}
#else
printf( "reloadPerFrame()\n" );
#endif

  /* Clear string space */
  memset(preset->per_frame_eqn_string_buffer, 0, STRING_BUFFER_SIZE);

  /* Compute length of string */
  slen = strlen(s);

  /* Copy new string into buffer */
  strncpy(preset->per_frame_eqn_string_buffer, s, slen);

  /* Yet again no bounds checking */
  preset->per_frame_eqn_string_index = slen;

  /* Finished */
  printf("reloadPerFrame: %d eqns parsed succesfully\n", eqn_count-1);
  return;

}

preset_t * load_preset(const char * pathname) {

  preset_t * preset;

  printf( "loading preset from '%s'\n", pathname );

  /* Initialize preset struct */
  if ((preset = (preset_t*)wipemalloc(sizeof(preset_t))) == NULL)
    return NULL;
   
  /* Initialize equation trees */
  preset->init_cond_tree = create_splaytree((int (*)(void*,void*))compare_string, (void* (*)(void*))copy_string, (void (*)(void*))free_string);
  preset->user_param_tree = create_splaytree((int (*)(void*,void*))compare_string,(void* (*)(void*)) copy_string,  (void (*)(void*))free_string);
  preset->per_frame_eqn_tree = create_splaytree((int (*)(void*,void*))compare_int,(void* (*)(void*)) copy_int, (void (*)(void*)) free_int);
  preset->per_pixel_eqn_tree = create_splaytree((int (*)(void*,void*))compare_int,(void* (*)(void*)) copy_int, (void (*)(void*)) free_int);
  preset->per_frame_init_eqn_tree = create_splaytree((int (*)(void*,void*))compare_string,(void* (*)(void*)) copy_string, (void (*)(void*)) free_string);
  preset->custom_wave_tree = create_splaytree((int (*)(void*,void*))compare_int, (void* (*)(void*))copy_int, (void (*)(void*)) free_int);
  preset->custom_shape_tree = create_splaytree((int (*)(void*,void*))compare_int, (void* (*)(void*))copy_int, (void (*)(void*)) free_int);

  memset(preset->per_pixel_flag, 0, sizeof(int)*NUM_OPS);

  /* Copy file path */  
  if ( pathname == NULL ) {
    close_preset( preset );
    return NULL;
  }
  strncpy(preset->file_path, pathname, MAX_PATH_SIZE-1);
  
  /* Set initial index values */
  preset->per_pixel_eqn_string_index = 0;
  preset->per_frame_eqn_string_index = 0;
  preset->per_frame_init_eqn_string_index = 0;
  
  
  /* Clear string buffers */
  memset(preset->per_pixel_eqn_string_buffer, 0, STRING_BUFFER_SIZE);
  memset(preset->per_frame_eqn_string_buffer, 0, STRING_BUFFER_SIZE);
  memset(preset->per_frame_init_eqn_string_buffer, 0, STRING_BUFFER_SIZE);
  
  
  if (load_preset_file(pathname, preset) < 0) {
#ifdef PRESET_DEBUG
	if (PRESET_DEBUG) printf("load_preset: failed to load file \"%s\"\n", pathname);
#endif
	close_preset(preset);
	return NULL;
  }

  /* It's kind of ugly to reset these values here. Should definitely be placed in the parser somewhere */
  per_frame_eqn_count = 0;
  per_frame_init_eqn_count = 0;

  /* Finished, return new preset */
  return preset;
}

void savePreset(char * filename) {

  FILE * fs;

  if (filename == NULL)
    return;
  
  /* Open the file corresponding to pathname */
  if ((fs = fopen(filename, "w+")) == 0) {
#ifdef PRESET_DEBUG
    if (PRESET_DEBUG) printf("savePreset: failed to create filename \"%s\"!\n", filename);
#endif
    return;	
  }

  write_stream = fs;

  if (write_preset_name(fs) < 0) {
    write_stream = NULL;
    fclose(fs);
    return;
  }

  if (write_init_conditions(fs) < 0) {
    write_stream = NULL;
    fclose(fs);
    return;
  }

  if (write_per_frame_init_equations(fs) < 0) {
    write_stream = NULL;
    fclose(fs);
    return;
  }

  if (write_per_frame_equations(fs) < 0) {
    write_stream = NULL;
    fclose(fs);
    return;
  }

  if (write_per_pixel_equations(fs) < 0) {
    write_stream = NULL;
    fclose(fs);
    return;
  }
 
  write_stream = NULL;
  fclose(fs);

}

int write_preset_name(FILE * fs) {

  char s[256];
  int len;

  memset(s, 0, 256);

  if (fs == NULL)
    return PROJECTM_FAILURE;

  /* Format the preset name in a string */
  sprintf(s, "[%s]\n", active_preset->name);

  len = strlen(s);

  /* Write preset name to file stream */
  if (fwrite(s, 1, len, fs) != len)
    return PROJECTM_FAILURE;

  return PROJECTM_SUCCESS;

}

int write_init_conditions(FILE * fs) {

  if (fs == NULL)
    return PROJECTM_FAILURE;
  if (active_preset == NULL)
    return PROJECTM_FAILURE;


  splay_traverse( (void (*)(void*))write_init, active_preset->init_cond_tree);
  
  return PROJECTM_SUCCESS;
}

void write_init(init_cond_t * init_cond) {

  char s[512];
  int len;

  if (write_stream == NULL)
    return;

  memset(s, 0, 512);

  if (init_cond->param->type == P_TYPE_BOOL)
    sprintf(s, "%s=%d\n", init_cond->param->name, init_cond->init_val.bool_val);

  else if (init_cond->param->type == P_TYPE_INT)    
    sprintf(s, "%s=%d\n", init_cond->param->name, init_cond->init_val.int_val);

  else if (init_cond->param->type == P_TYPE_DOUBLE)
    sprintf(s, "%s=%f\n", init_cond->param->name, init_cond->init_val.float_val);

  else { printf("write_init: unknown parameter type!\n"); return; }
  
  len = strlen(s);

  if ((fwrite(s, 1, len, write_stream)) != len)
    printf("write_init: failed writing to file stream! Out of disk space?\n");
  
}


int write_per_frame_init_equations(FILE * fs) {

  int len;

  if (fs == NULL)
    return PROJECTM_FAILURE;
  if (active_preset == NULL)
    return PROJECTM_FAILURE;
  
  len = strlen(active_preset->per_frame_init_eqn_string_buffer);

  if (fwrite(active_preset->per_frame_init_eqn_string_buffer, 1, len, fs) != len)
    return PROJECTM_FAILURE;

  return PROJECTM_SUCCESS;
}


int write_per_frame_equations(FILE * fs) {

  int len;

  if (fs == NULL)
    return PROJECTM_FAILURE;
  if (active_preset == NULL)
    return PROJECTM_FAILURE;

  len = strlen(active_preset->per_frame_eqn_string_buffer);

  if (fwrite(active_preset->per_frame_eqn_string_buffer, 1, len, fs) != len)
    return PROJECTM_FAILURE;

  return PROJECTM_SUCCESS;
}


int write_per_pixel_equations(FILE * fs) {

  int len;

  if (fs == NULL)
    return PROJECTM_FAILURE;
  if (active_preset == NULL)
    return PROJECTM_FAILURE;

  len = strlen(active_preset->per_pixel_eqn_string_buffer);

  if (fwrite(active_preset->per_pixel_eqn_string_buffer, 1, len, fs) != len)
    return PROJECTM_FAILURE;

  return PROJECTM_SUCCESS;
}


void load_init_conditions(preset_t *preset) {
  preset_hack=preset;
  splay_traverse( (void (*)(void*))load_init_cond, builtin_param_tree);

 
}

void load_init_cond(param_t * param) {

  init_cond_t * init_cond;
  value_t init_val;

  /* Don't count read only parameters as initial conditions */
  if (param->flags & P_FLAG_READONLY)
    return;

  /* If initial condition was not defined by the preset file, force a default one
     with the following code */
  if ((init_cond = (init_cond_t*)(splay_find(param->name, preset_hack->init_cond_tree))) == NULL) {
    
    /* Make sure initial condition does not exist in the set of per frame initial equations */
    if ((init_cond = (init_cond_t*)(splay_find(param->name, preset_hack->per_frame_init_eqn_tree))) != NULL)
      return;
    
    if (param->type == P_TYPE_BOOL)
      init_val.bool_val = 0;
    
    else if (param->type == P_TYPE_INT)
      init_val.int_val = *(int*)param->engine_val;

    else if (param->type == P_TYPE_DOUBLE)
      init_val.float_val = *(float*)param->engine_val;

    /* Create new initial condition */
    if ((init_cond = new_init_cond(param, init_val)) == NULL)
      return;
    
    /* Insert the initial condition into this presets tree */
    if (splay_insert(init_cond, init_cond->param->name, preset_hack->init_cond_tree) < 0) {
      free_init_cond(init_cond);
      return;
    }
    
  }
 
}

void load_custom_wave_init_conditions(preset_t *preset) {

  splay_traverse((void (*)(void*))load_custom_wave_init, preset->custom_wave_tree);

}

void load_custom_wave_init(custom_wave_t * custom_wave) {

  load_unspecified_init_conds(custom_wave);

}


void load_custom_shape_init_conditions(preset_t *preset) {

  splay_traverse((void (*)(void*))load_custom_shape_init, preset->custom_shape_tree);

}

void load_custom_shape_init(custom_shape_t * custom_shape) {
 
  load_unspecified_init_conds_shape(custom_shape);
 
}

preset_t* getActivePreset() {
  return active_preset;
}
