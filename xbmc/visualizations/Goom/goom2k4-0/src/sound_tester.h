#ifndef _SOUND_TESTER_H
#define _SOUND_TESTER_H

#include "goom_plugin_info.h"
#include "goom_config.h"

/** change les donnees du SoundInfo */
void evaluate_sound(gint16 data[2][512], SoundInfo *sndInfo);

#endif

