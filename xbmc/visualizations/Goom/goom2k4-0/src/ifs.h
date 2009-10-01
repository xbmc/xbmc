/*
 * File created 11 april 2002 by JeKo <jeko@free.fr>
 */

#ifndef IFS_H
#define IFS_H

#include "goom_config.h"
#include "goom_graphic.h"
#include "goom_plugin_info.h"
#include "goom_visual_fx.h"

VisualFX ifs_visualfx_create(void);

/* init ifs for a (width)x(height) output. * /
void init_ifs (PluginInfo *goomInfo, int width, int height);

/ * draw an ifs on the buffer (which size is width * height)
   increment means that we draw 1/increment of the ifs's points * /
void ifs_update (PluginInfo *goomInfo, Pixel * buffer, Pixel * back, int width, int height, int increment);

/ * free all ifs's data. * /
void release_ifs (void);
*/


#endif
