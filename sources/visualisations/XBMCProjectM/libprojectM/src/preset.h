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

#ifndef _PRESET_H
#define _PRESET_H

//#define PRESET_DEBUG 2 /* 0 for no debugging, 1 for normal, 2 for insane */

#define HARD_CUT 0
#define SOFT_CUT 1

#include "preset_types.h"
#include "projectM.h"

void evalInitConditions(preset_t *preset);
void evalPerFrameEquations(preset_t *preset);

int switchPreset(switch_mode_t switch_mode, int cut_type);
void switchToIdlePreset();
int loadPresetDir(char * dir);
int closePresetDir();
int initPresetLoader(projectM_t *PM);
int destroyPresetLoader();
int loadPresetByFile(char * filename);
void reloadPerFrame(char * s, preset_t * preset);
void reloadPerFrameInit(char *s, preset_t * preset);
void reloadPerPixel(char *s, preset_t * preset);
void savePreset(char * name);
preset_t* getActivePreset();


#endif /** !_PRESET_H */
