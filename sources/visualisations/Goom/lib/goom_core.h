#ifndef _GOOMCORE_H
#define _GOOMCORE_H

#include "config.h"
#ifdef __cplusplus
extern "C" {
#endif

/* typedef union {
   guint32 val;
   struct {
   guint8 r;
   guint8 g;
   guint8 b;
   guint32 a;
   } rgba;
   } Pixel ;
   
   typedef Pixel * GoomBuffer;
*/

#define NB_FX 10

void    goom_init (guint32 resx, guint32 resy, int cinemascope);
void    goom_set_resolution (guint32 resx, guint32 resy, int cinemascope);

/*
 * forceMode == 0 : do nothing
 * forceMode == -1 : lock the FX
 * forceMode == 1..NB_FX : force a switch to FX n°forceMode
 *
 * songTitle = pointer to the title of the song...
 *      - NULL if it is not the start of the song
 *      - only have a value at the start of the song
 */
guint32 *goom_update (gint16 data[2][512], int forceMode, float fps,
											char *songTitle, char *message);

void    goom_close ();

void    goom_set_font (int ***chars, int *width, int *height);
#ifdef __cplusplus
}
#endif

#endif
