/*
 * video_out_x11.c
 * Copyright (C) 2000-2003 Michel Lespinasse <walken@zoy.org>
 * Copyright (C) 2003      Regis Duchesne <hpreg@zoy.org>
 * Copyright (C) 1999-2000 Aaron Holtzman <aholtzma@ess.engr.uvic.ca>
 *
 * This file is part of mpeg2dec, a free MPEG-2 video stream decoder.
 * See http://libmpeg2.sourceforge.net/ for updates.
 *
 * mpeg2dec is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * mpeg2dec is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#ifdef LIBVO_X11

#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/extensions/XShm.h>
#include <inttypes.h>
#include <unistd.h>
/* since it doesn't seem to be defined on some platforms */
int XShmGetEventBase (Display *);

#ifdef LIBVO_XV
#include <string.h>	/* strcmp */
#include <X11/extensions/Xvlib.h>
#define FOURCC_YV12 0x32315659
#define FOURCC_UYVY 0x59565955
#endif

#include "mpeg2.h"
#include "video_out.h"
#include "vo_internal.h"
#include "mpeg2convert.h"

typedef struct {
    void * data;
    int wait_completion;
    XImage * ximage;
#ifdef LIBVO_XV
    XvImage * xvimage;
#endif
} x11_frame_t;

typedef struct x11_instance_s {
    vo_instance_t vo;
    x11_frame_t frame[3];
    int index;
    int width;
    int height;
    Display * display;
    Window window;
    GC gc;
    XVisualInfo vinfo;
    XShmSegmentInfo shminfo;
    int xshm_extension;
    int completion_type;
    int xshm;
#ifdef LIBVO_XV
    unsigned int adaptors;
    XvAdaptorInfo * adaptorInfo;
    XvPortID port;
    int xv;
#endif
    void (* teardown) (struct x11_instance_s * instance);
} x11_instance_t;

static int open_display (x11_instance_t * instance, int width, int height)
{
    int major;
    int minor;
    Bool pixmaps;
    XVisualInfo visualTemplate;
    XVisualInfo * XvisualInfoTable;
    XVisualInfo * XvisualInfo;
    int number;
    int i;
    XSetWindowAttributes attr;
    XGCValues gcValues;

    instance->display = XOpenDisplay (NULL);
    if (! (instance->display)) {
	fprintf (stderr, "Can not open display\n");
	return 1;
    }

    instance->xshm_extension = 0;
    if (XShmQueryVersion (instance->display, &major, &minor,
			  &pixmaps) != 0 &&
	(major > 1 || (major == 1 && minor >= 1))) {
	instance->xshm_extension = 1;
	instance->completion_type =
	    XShmGetEventBase (instance->display) + ShmCompletion;
    } else
	fprintf (stderr, "No xshm extension\n");

    /* list truecolor visuals for the default screen */
#ifdef __cplusplus
    visualTemplate.c_class = TrueColor;
#else
    visualTemplate.class = TrueColor;
#endif
    visualTemplate.screen = DefaultScreen (instance->display);
    XvisualInfoTable = XGetVisualInfo (instance->display,
				       VisualScreenMask | VisualClassMask,
				       &visualTemplate, &number);
    if (XvisualInfoTable == NULL) {
	fprintf (stderr, "No truecolor visual\n");
	return 1;
    }

    /* find the visual with the highest depth */
    XvisualInfo = XvisualInfoTable;
    for (i = 1; i < number; i++)
	if (XvisualInfoTable[i].depth > XvisualInfo->depth)
	    XvisualInfo = XvisualInfoTable + i;

    instance->vinfo = *XvisualInfo;
    XFree (XvisualInfoTable);

    attr.background_pixmap = None;
    attr.backing_store = NotUseful;
    attr.border_pixel = 0;
    attr.event_mask = 0;
    /* fucking sun blows me - you have to create a colormap there... */
    attr.colormap = XCreateColormap (instance->display,
				     RootWindow (instance->display,
						 instance->vinfo.screen),
				     instance->vinfo.visual, AllocNone);
    instance->window =
	XCreateWindow (instance->display,
		       DefaultRootWindow (instance->display),
		       0 /* x */, 0 /* y */, width, height,
		       0 /* border_width */, instance->vinfo.depth,
		       InputOutput, instance->vinfo.visual,
		       (CWBackPixmap | CWBackingStore | CWBorderPixel |
			CWEventMask | CWColormap), &attr);

    instance->gc = XCreateGC (instance->display, instance->window, 0,
			      &gcValues);

#ifdef LIBVO_XV
    instance->adaptors = 0;
    instance->adaptorInfo = NULL;
#endif

    return 0;
}

static int shmerror = 0;

static int handle_error (Display * display, XErrorEvent * error)
{
    shmerror = 1;
    return 0;
}

static void * create_shm (x11_instance_t * instance, int size)
{
    instance->shminfo.shmid = shmget (IPC_PRIVATE, size, IPC_CREAT | 0777);
    if (instance->shminfo.shmid == -1)
	goto error;

    instance->shminfo.shmaddr = (char *) shmat (instance->shminfo.shmid, 0, 0);
    if (instance->shminfo.shmaddr == (char *)-1)
	goto error;

    /* on linux the IPC_RMID only kicks off once everyone detaches the shm */
    /* doing this early avoids shm leaks when we are interrupted. */
    /* this would break the solaris port though :-/ */
    /* shmctl (instance->shminfo.shmid, IPC_RMID, 0); */

    /* XShmAttach fails on remote displays, so we have to catch this event */

    XSync (instance->display, False);
    XSetErrorHandler (handle_error);

    instance->shminfo.readOnly = True;
    if (! (XShmAttach (instance->display, &(instance->shminfo))))
	shmerror = 1;

    XSync (instance->display, False);
    XSetErrorHandler (NULL);
    if (shmerror) {
    error:
	fprintf (stderr, "cannot create shared memory\n");
	if (instance->shminfo.shmid != -1) {
	    shmdt (instance->shminfo.shmaddr);
	    shmctl (instance->shminfo.shmid, IPC_RMID, 0);
	}
	return NULL;
    }

    return instance->shminfo.shmaddr;
}

static void destroy_shm (x11_instance_t * instance)
{
    XShmDetach (instance->display, &(instance->shminfo));
    shmdt (instance->shminfo.shmaddr);
    shmctl (instance->shminfo.shmid, IPC_RMID, 0);
}

static void x11_event (x11_instance_t * instance)
{
    XEvent event;
    char * addr;
    int i;

    XNextEvent (instance->display, &event);
    if (event.type == instance->completion_type) {
	addr = (instance->shminfo.shmaddr +
		((XShmCompletionEvent *)&event)->offset);
	for (i = 0; i < 3; i++)
	    if (addr == instance->frame[i].data)
		instance->frame[i].wait_completion = 0;
    }
}

static void x11_start_fbuf (vo_instance_t * _instance,
			    uint8_t * const * buf, void * id)
{
    x11_instance_t * instance = (x11_instance_t *) _instance;
    x11_frame_t * frame = (x11_frame_t *) id;

    while (frame->wait_completion)
	x11_event (instance);
}

static void x11_setup_fbuf (vo_instance_t * _instance,
			    uint8_t ** buf, void ** id)
{
    x11_instance_t * instance = (x11_instance_t *) _instance;

    buf[0] = (uint8_t *) instance->frame[instance->index].data;
    buf[1] = buf[2] = NULL;
    *id = instance->frame + instance->index++;
}

static void x11_draw_frame (vo_instance_t * _instance,
			    uint8_t * const * buf, void * id)
{
    x11_frame_t * frame;
    x11_instance_t * instance;

    frame = (x11_frame_t *) id;
    instance = (x11_instance_t *) _instance;
    if (instance->xshm)
	XShmPutImage (instance->display, instance->window, instance->gc,
		      frame->ximage, 0, 0, 0, 0,
		      instance->width, instance->height, True);
    else
	XPutImage (instance->display, instance->window, instance->gc,
		   frame->ximage, 0, 0, 0, 0,
		   instance->width, instance->height);
    XFlush (instance->display);
    frame->wait_completion = instance->xshm;
}

static int x11_alloc_frames (x11_instance_t * instance, int xshm)
{
    int size;
    char * alloc;
    int i = 0;

    if (xshm && !instance->xshm_extension)
	return 1;

    size = 0;
    alloc = NULL;
    while (i < 3) {
	instance->frame[i].wait_completion = 0;
	instance->frame[i].ximage = xshm ?
	    XShmCreateImage (instance->display, instance->vinfo.visual,
			     instance->vinfo.depth, ZPixmap, NULL /* data */,
			     &(instance->shminfo),
			     instance->width, instance->height) :
	    XCreateImage(instance->display, instance->vinfo.visual,
			 instance->vinfo.depth, ZPixmap, 0, NULL /* data */,
			 instance->width, instance->height, 8, 0);
	if (instance->frame[i].ximage == NULL) {
	    fprintf (stderr, "Cannot create ximage\n");
	    return 1;
	} else if (xshm) {
	    if (i == 0) {
		size = (instance->frame[0].ximage->bytes_per_line *
			instance->frame[0].ximage->height);
		alloc = (char *) create_shm (instance, 3 * size);
	    } else if (size != (instance->frame[i].ximage->bytes_per_line *
				instance->frame[i].ximage->height)) {
		fprintf (stderr, "unexpected ximage data size\n");
		return 1;
	    }
	} else
	    alloc =
		(char *) malloc (instance->frame[i].ximage->bytes_per_line *
				 instance->frame[i].ximage->height);
	instance->frame[i].data = instance->frame[i].ximage->data = alloc;
	i++;
	if (alloc == NULL) {
	    while (--i >= 0)
		XDestroyImage (instance->frame[i].ximage);
	    return 1;
	}
	alloc += size;
    }

    instance->xshm = xshm;
    return 0;
}

static void x11_teardown (x11_instance_t * instance)
{
    int i;

    for (i = 0; i < 3; i++) {
	while (instance->frame[i].wait_completion)
	    x11_event (instance);
	XDestroyImage (instance->frame[i].ximage);
    }
    if (instance->xshm)
	destroy_shm (instance);
}

static void x11_close (vo_instance_t * _instance)
{
    x11_instance_t * instance = (x11_instance_t *) _instance;

    if (instance->teardown != NULL)
	instance->teardown (instance);
    XFreeGC (instance->display, instance->gc);
    XDestroyWindow (instance->display, instance->window);
#ifdef LIBVO_XV
    if (instance->adaptors)
	XvFreeAdaptorInfo (instance->adaptorInfo);
#endif
    XCloseDisplay (instance->display);
    free (instance);
}

#ifdef LIBVO_XV
static void xv_setup_fbuf (vo_instance_t * _instance,
			   uint8_t ** buf, void ** id)
{
    x11_instance_t * instance = (x11_instance_t *) _instance;
    uint8_t * data;

    data = (uint8_t *) instance->frame[instance->index].xvimage->data;
    buf[0] = data + instance->frame[instance->index].xvimage->offsets[0];
    buf[1] = data + instance->frame[instance->index].xvimage->offsets[2];
    buf[2] = data + instance->frame[instance->index].xvimage->offsets[1];
    *id = instance->frame + instance->index++;
}

static void xv_draw_frame (vo_instance_t * _instance,
			   uint8_t * const * buf, void * id)
{
    x11_frame_t * frame = (x11_frame_t *) id;
    x11_instance_t * instance = (x11_instance_t *) _instance;

    if (instance->xshm)
	XvShmPutImage (instance->display, instance->port, instance->window,
		       instance->gc, frame->xvimage, 0, 0,
		       instance->width, instance->height, 0, 0,
		       instance->width, instance->height, True);
    else
	XvPutImage (instance->display, instance->port, instance->window,
		    instance->gc, frame->xvimage, 0, 0,
		    instance->width, instance->height, 0, 0,
		    instance->width, instance->height);
    XFlush (instance->display);
    frame->wait_completion = instance->xshm;
}

static int xv_check_fourcc (x11_instance_t * instance, XvPortID port,
			    int fourcc, const char * fourcc_str)
{
    XvImageFormatValues * formatValues;
    int formats;
    int i;

    formatValues = XvListImageFormats (instance->display, port, &formats);
    for (i = 0; i < formats; i++)
	if ((formatValues[i].id == fourcc) &&
	    (! (strcmp (formatValues[i].guid, fourcc_str)))) {
	    XFree (formatValues);
	    return 0;
	}
    XFree (formatValues);
    return 1;
}

static int xv_check_extension (x11_instance_t * instance,
			       int fourcc, const char * fourcc_str)
{
    unsigned int i;
    unsigned long j;

    if (!instance->adaptorInfo) {
	unsigned int version;
	unsigned int release;
	unsigned int dummy;

	if ((XvQueryExtension (instance->display, &version, &release,
			       &dummy, &dummy, &dummy) != Success) ||
	    (version < 2) || ((version == 2) && (release < 2))) {
	    fprintf (stderr, "No xv extension\n");
	    return 1;
	}

	XvQueryAdaptors (instance->display, instance->window,
			 &instance->adaptors, &instance->adaptorInfo);
    }

    for (i = 0; i < instance->adaptors; i++)
	if (instance->adaptorInfo[i].type & XvImageMask)
	    for (j = 0; j < instance->adaptorInfo[i].num_ports; j++)
		if ((! (xv_check_fourcc (instance,
					 instance->adaptorInfo[i].base_id + j,
					 fourcc, fourcc_str))) &&
		    (XvGrabPort (instance->display,
				 instance->adaptorInfo[i].base_id + j,
				 0) == Success)) {
		    instance->port = instance->adaptorInfo[i].base_id + j;
		    return 0;
		}

    fprintf (stderr, "Cannot find xv %s port\n", fourcc_str);
    return 1;
}

static int xv_alloc_frames (x11_instance_t * instance, int size,
			    int fourcc)
{
    char * alloc;
    int i = 0;

    instance->xshm = 1;
    alloc = instance->xshm_extension  ?
	(char *) create_shm (instance, 3 * size) : NULL;
    if (alloc == NULL) {
	instance->xshm = 0;
	alloc = (char *) malloc (3 * size);
	if (alloc == NULL)
	    return 1;
    }

    while (i < 3) {
	instance->frame[i].wait_completion = 0;
	instance->frame[i].xvimage = instance->xshm ?
	    XvShmCreateImage (instance->display, instance->port, fourcc,
			      alloc, instance->width, instance->height,
			      &(instance->shminfo)) :
	    XvCreateImage (instance->display, instance->port, fourcc,
			   alloc, instance->width, instance->height);
	instance->frame[i].data = alloc;
	alloc += size;
	if ((instance->frame[i].xvimage == NULL) ||
	    (instance->frame[i++].xvimage->data_size != size)) {
	    while (--i >= 0)
		XFree (instance->frame[i].xvimage);
	    if (instance->xshm)
		destroy_shm (instance);
	    else
		free (instance->frame[0].data);
	    return 1;
	}
    }

    return 0;
}

static void xv_teardown (x11_instance_t * instance)
{
    int i;

    for (i = 0; i < 3; i++) {
	while (instance->frame[i].wait_completion)
	    x11_event (instance);
	XFree (instance->frame[i].xvimage);
    }
    if (instance->xshm)
	destroy_shm (instance);
    else
	free (instance->frame[0].data);
    XvUngrabPort (instance->display, instance->port, 0);
}
#endif

static int common_setup (vo_instance_t * _instance, unsigned int width,
			 unsigned int height, unsigned int chroma_width,
			 unsigned int chroma_height,
			 vo_setup_result_t * result)
{
    x11_instance_t * instance = (x11_instance_t *) _instance;

    if (instance->display != NULL) {
	/* Already setup, just adjust to the new size */
	if (instance->teardown != NULL)
	    instance->teardown (instance);
        XResizeWindow (instance->display, instance->window, width, height);
    } else {
	/* Not setup yet, do the full monty */
        if (open_display (instance, width, height))
            return 1;
        XMapWindow (instance->display, instance->window);
    }
    instance->vo.setup_fbuf = NULL;
    instance->vo.start_fbuf = NULL;
    instance->vo.set_fbuf = NULL;
    instance->vo.draw = NULL;
    instance->vo.discard = NULL;
    instance->vo.close = x11_close;
    instance->width = width;
    instance->height = height;
    instance->index = 0;
    instance->teardown = NULL;
    result->convert = NULL;

#ifdef LIBVO_XV
    if (instance->xv == 1 &&
	(chroma_width == width >> 1) && (chroma_height == height >> 1) &&
	!xv_check_extension (instance, FOURCC_YV12, "YV12") &&
	!xv_alloc_frames (instance, 3 * width * height / 2, FOURCC_YV12)) {
	instance->vo.setup_fbuf = xv_setup_fbuf;
	instance->vo.start_fbuf = x11_start_fbuf;
	instance->vo.draw = xv_draw_frame;
	instance->teardown = xv_teardown;
    } else if (instance->xv && (chroma_width == width >> 1) &&
	       !xv_check_extension (instance, FOURCC_UYVY, "UYVY") &&
	       !xv_alloc_frames (instance, 2 * width * height, FOURCC_UYVY)) {
	instance->vo.setup_fbuf = x11_setup_fbuf;
	instance->vo.start_fbuf = x11_start_fbuf;
	instance->vo.draw = xv_draw_frame;
	instance->teardown = xv_teardown;
	result->convert = mpeg2convert_uyvy;
    } else
#endif
    if (!x11_alloc_frames (instance, 1) || !x11_alloc_frames (instance, 0)) {
	int bpp;

	instance->vo.setup_fbuf = x11_setup_fbuf;
	instance->vo.start_fbuf = x11_start_fbuf;
	instance->vo.draw = x11_draw_frame;
	instance->teardown = x11_teardown;

#ifdef WORDS_BIGENDIAN
	if (instance->frame[0].ximage->byte_order != MSBFirst) {
	    fprintf (stderr, "No support for non-native byte order\n");
	    return 1;
	}
#else
	if (instance->frame[0].ximage->byte_order != LSBFirst) {
	    fprintf (stderr, "No support for non-native byte order\n");
	    return 1;
	}
#endif

	/*
	 * depth in X11 terminology land is the number of bits used to
	 * actually represent the colour.
	 *
	 * bpp in X11 land means how many bits in the frame buffer per
	 * pixel.
	 *
	 * ex. 15 bit color is 15 bit depth and 16 bpp. Also 24 bit
	 *     color is 24 bit depth, but can be 24 bpp or 32 bpp.
	 *
	 * If we have blue in the lowest bit then "obviously" RGB
	 * (the guy who wrote this convention never heard of endianness ?)
	 */

	bpp = ((instance->vinfo.depth == 24) ?
	       instance->frame[0].ximage->bits_per_pixel :
	       instance->vinfo.depth);
	result->convert =
	    mpeg2convert_rgb (((instance->frame[0].ximage->blue_mask & 1) ?
			       MPEG2CONVERT_RGB : MPEG2CONVERT_BGR), bpp);
	if (result->convert == NULL) {
	    fprintf (stderr, "%dbpp not supported\n", bpp);
	    return 1;
	}
    }

    return 0;
}

static vo_instance_t * common_open (int xv)
{
    x11_instance_t * instance;

    instance = (x11_instance_t *) malloc (sizeof (x11_instance_t));
    if (instance == NULL)
	return NULL;

    instance->vo.setup = common_setup;
    instance->vo.close = (void (*) (vo_instance_t *)) free;
    instance->display = NULL;
#ifdef LIBVO_XV
    instance->xv = xv;
#endif
    return (vo_instance_t *) instance;
}

vo_instance_t * vo_x11_open (void)
{
    return common_open (0);
}

#ifdef LIBVO_XV
vo_instance_t * vo_xv_open (void)
{
    return common_open (1);
}

vo_instance_t * vo_xv2_open (void)
{
    return common_open (2);
}
#endif
#endif
