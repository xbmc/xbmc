#ifndef _GOOMCORE_H
#define _GOOMCORE_H

#include "goom_config.h"
#include "goom_plugin_info.h"
#include "goomsl.h"

#define NB_FX 10

PluginInfo *goom_init (guint32 resx, guint32 resy);
void goom_set_resolution (PluginInfo *goomInfo, guint32 resx, guint32 resy);

/*
 * forceMode == 0 : do nothing
 * forceMode == -1 : lock the FX
 * forceMode == 1..NB_FX : force a switch to FX n# forceMode
 *
 * songTitle = pointer to the title of the song...
 *      - NULL if it is not the start of the song
 *      - only have a value at the start of the song
 */
guint32 *goom_update (PluginInfo *goomInfo, gint16 data[2][512], int forceMode, float fps,
                      char *songTitle, char *message);

/* returns 0 if the buffer wasn't accepted */
int goom_set_screenbuffer(PluginInfo *goomInfo, void *buffer);

void goom_close (PluginInfo *goomInfo);

#endif
