#ifndef _VISUAL_FX_H
#define _VISUAL_FX_H

/**
 * File created on 2003-05-21 by Jeko.
 * (c)2003, JC Hoelt for iOS-software.
 *
 * LGPL Licence.
 * If you use this file on a visual program,
 *    please make my name being visible on it.
 */

#include "goom_config_param.h"
#include "goom_graphic.h"
#include "goom_typedefs.h"

struct _VISUAL_FX {
  void (*init) (struct _VISUAL_FX *_this, PluginInfo *info);
  void (*free) (struct _VISUAL_FX *_this);
  void (*apply) (struct _VISUAL_FX *_this, Pixel *src, Pixel *dest, PluginInfo *info);
  void *fx_data;

  PluginParameters *params;
};

#endif
