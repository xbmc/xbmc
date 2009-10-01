#ifndef _SDL_GOOM_H
#define _SDL_GOOM_H

#include <gtk/gtk.h>
#include "goom_config_param.h"
#include "goom_plugin_info.h"

typedef struct _SDL_GOOM {
  GtkWidget *config_win;

  int screen_width;
  int screen_height;
  int doublepix;
  int active;
  
  PluginInfo *plugin; /* infos about the plugin (see plugin_info.h) */

  PluginParameters screen; /* contains screen_size, pix_double, fps_limit */

  PluginParam screen_size;
  PluginParam pix_double;
  PluginParam fps_limit;
  PluginParam display_fps;
  PluginParam hide_cursor;

} SdlGoom;

void gtk_data_init(SdlGoom *sdlGoom);

#endif
