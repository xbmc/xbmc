/*
** Nofrendo (c) 1998-2000 Matthew Conte (matt@conte.com)
**
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of version 2 of the GNU Library General 
** Public License as published by the Free Software Foundation.
**
** This program is distributed in the hope that it will be useful, 
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
** Library General Public License for more details.  To obtain a 
** copy of the GNU Library General Public License, write to the Free 
** Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** Any permitted reproduction of these routines, in whole or in part,
** must bear this legend.
**
**
** osd.h
**
** O/S dependent routine defintions (must be customized)
** $Id: osd.h,v 1.7 2000/07/04 04:45:33 matt Exp $
*/

#ifndef _OSD_H_
#define _OSD_H_


#ifdef __GNUC__
#define  __PACKED__  __attribute__ ((packed))
#define  PATH_SEP    '/'
#ifdef __DJGPP__
#include <dpmi.h>
#include "dos_ints.h"
#endif
#elif defined(WIN32)
#define  __PACKED__
#define  PATH_SEP    '\\'
#else /* crapintosh? */
#define  __PACKED__
#define  PATH_SEP    ':'
#endif

extern void osd_loginit(void);
extern void osd_logshutdown(void);
extern void osd_logprint(const char *string);

extern int osd_startsound(void (*playfunc)(void *buffer, int size));
extern int osd_getsoundbps(void);
extern int osd_getsamplerate(void);


#ifndef NSF_PLAYER
#include "rgb.h"
#include "bitmap.h"

extern bitmap_t *osd_getvidbuf(void);
typedef void (*blitproc_t)(bitmap_t *bmp, int x_pos, int y_pos, int width, int height);
extern blitproc_t osd_blit;
extern void osd_copytoscreen(void);

extern void osd_showusage(char *filename);
extern void osd_fullname(char *fullname, const char *shortname);
extern char *osd_newextension(char *string, char *ext);

extern void osd_setpalette(rgb_t *pal);
extern void osd_restorepalette(void);

extern void osd_getinput(void);
extern int osd_gethostinput(void);
extern void osd_getmouse(int *x, int *y, int *button);

extern int osd_init(void);
extern void osd_shutdown(void);
#endif /* !NSF_PLAYER */

#endif /* _OSD_H_ */

/*
** $Log: osd.h,v $
** Revision 1.7  2000/07/04 04:45:33  matt
** moved INLINE define into types.h
**
** Revision 1.6  2000/06/29 16:06:18  neil
** Wrapped DOS-specific headers in an ifdef
**
** Revision 1.5  2000/06/09 15:12:25  matt
** initial revision
**
*/