/* 
 * Copyright © 2001 Keith Packard, member of The XFree86 Project, Inc.
 * Copyright © 2002 Hewlett Packard Company, Inc.
 * Copyright © 2006 Intel Corporation
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation, and
 * that the name of the copyright holders not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  The copyright holders make no representations
 * about the suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 *
 * Thanks to Jim Gettys who wrote most of the client side code,
 * and part of the server code for randr.
 */

#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xlibint.h>
#include <X11/Xproto.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xrandr.h>
#include <X11/extensions/Xrender.h>	/* we share subpixel information */
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>

#if RANDR_MAJOR > 1 || (RANDR_MAJOR == 1 && RANDR_MINOR >= 2)
#define HAS_RANDR_1_2 1
#endif

static char	*program_name;
static Display	*dpy;
static Window	root;
static int	screen = -1;
static Bool	verbose = False;
static Bool	automatic = False;
static Bool	properties = False;

static char *direction[5] = {
    "normal", 
    "left", 
    "inverted", 
    "right",
    "\n"};

static char *reflections[5] = {
    "normal", 
    "x", 
    "y", 
    "xy",
    "\n"};

/* subpixel order */
static char *order[6] = {
    "unknown",
    "horizontal rgb",
    "horizontal bgr",
    "vertical rgb",
    "vertical bgr",
    "no subpixels"};

static const struct {
    char	    *string;
    unsigned long   flag;
} mode_flags[] = {
    { "+HSync", RR_HSyncPositive },
    { "-HSync", RR_HSyncNegative },
    { "+VSync", RR_VSyncPositive },
    { "-VSync", RR_VSyncNegative },
    { "Interlace", RR_Interlace },
    { "DoubleScan", RR_DoubleScan },
    { "CSync",	    RR_CSync },
    { "+CSync",	    RR_CSyncPositive },
    { "-CSync",	    RR_CSyncNegative },
    { NULL,	    0 }
};

static void
usage(void)
{
    fprintf(stderr, "usage: %s [options]\n", program_name);
    fprintf(stderr, "  where options are:\n");
    fprintf(stderr, "  -display <display> or -d <display>\n");
    fprintf(stderr, "  -help\n");
    fprintf(stderr, "  -o <normal,inverted,left,right,0,1,2,3>\n");
    fprintf(stderr, "            or --orientation <normal,inverted,left,right,0,1,2,3>\n");
    fprintf(stderr, "  -q        or --query\n");
    fprintf(stderr, "  -s <size>/<width>x<height> or --size <size>/<width>x<height>\n");
    fprintf(stderr, "  -r <rate> or --rate <rate> or --refresh <rate>\n");
    fprintf(stderr, "  -v        or --version\n");
    fprintf(stderr, "  -x        (reflect in x)\n");
    fprintf(stderr, "  -y        (reflect in y)\n");
    fprintf(stderr, "  --screen <screen>\n");
    fprintf(stderr, "  --verbose\n");
    fprintf(stderr, "  --dryrun\n");
#if HAS_RANDR_1_2
    fprintf(stderr, "  --prop or --properties\n");
    fprintf(stderr, "  --fb <width>x<height>\n");
    fprintf(stderr, "  --fbmm <width>x<height>\n");
    fprintf(stderr, "  --dpi <dpi>/<output>\n");
#if 0
    fprintf(stderr, "  --clone\n");
    fprintf(stderr, "  --extend\n");
#endif
    fprintf(stderr, "  --output <output>\n");
    fprintf(stderr, "      --auto\n");
    fprintf(stderr, "      --mode <mode>\n");
    fprintf(stderr, "      --preferred\n");
    fprintf(stderr, "      --pos <x>x<y>\n");
    fprintf(stderr, "      --rate <rate> or --refresh <rate>\n");
    fprintf(stderr, "      --reflect normal,x,y,xy\n");
    fprintf(stderr, "      --rotate normal,inverted,left,right\n");
    fprintf(stderr, "      --left-of <output>\n");
    fprintf(stderr, "      --right-of <output>\n");
    fprintf(stderr, "      --above <output>\n");
    fprintf(stderr, "      --below <output>\n");
    fprintf(stderr, "      --same-as <output>\n");
    fprintf(stderr, "      --set <property> <value>\n");
    fprintf(stderr, "      --off\n");
    fprintf(stderr, "      --crtc <crtc>\n");
    fprintf(stderr, "  --newmode <name> <clock MHz>\n");
    fprintf(stderr, "            <hdisp> <hsync-start> <hsync-end> <htotal>\n");
    fprintf(stderr, "            <vdisp> <vsync-start> <vsync-end> <vtotal>\n");
    fprintf(stderr, "            [+HSync] [-HSync] [+VSync] [-VSync]\n");
    fprintf(stderr, "  --rmmode <name>\n");
    fprintf(stderr, "  --addmode <output> <name>\n");
    fprintf(stderr, "  --delmode <output> <name>\n");
#endif

    exit(1);
    /*NOTREACHED*/
}

static void
fatal (const char *format, ...)
{
    va_list ap;
    
    va_start (ap, format);
    fprintf (stderr, "%s: ", program_name);
    vfprintf (stderr, format, ap);
    va_end (ap);
    exit (1);
    /*NOTREACHED*/
}

static char *
rotation_name (Rotation rotation)
{
    int	i;

    if ((rotation & 0xf) == 0)
	return "normal";
    for (i = 0; i < 4; i++)
	if (rotation & (1 << i))
	    return direction[i];
    return "invalid rotation";
}

static char *
reflection_name (Rotation rotation)
{
    rotation &= (RR_Reflect_X|RR_Reflect_Y);
    switch (rotation) {
    case 0:
	return "none";
    case RR_Reflect_X:
	return "X axis";
    case RR_Reflect_Y:
	return "Y axis";
    case RR_Reflect_X|RR_Reflect_Y:
	return "X and Y axis";
    }
    return "invalid reflection";
}

#if HAS_RANDR_1_2
typedef enum _policy {
    clone, extend
} policy_t;

typedef enum _relation {
    left_of, right_of, above, below, same_as,
} relation_t;

typedef enum _changes {
    changes_none = 0,
    changes_crtc = (1 << 0),
    changes_mode = (1 << 1),
    changes_relation = (1 << 2),
    changes_position = (1 << 3),
    changes_rotation = (1 << 4),
    changes_reflection = (1 << 5),
    changes_automatic = (1 << 6),
    changes_refresh = (1 << 7),
    changes_property = (1 << 8),
} changes_t;

typedef enum _name_kind {
    name_none = 0,
    name_string = (1 << 0),
    name_xid = (1 << 1),
    name_index = (1 << 2),
    name_preferred = (1 << 3),
} name_kind_t;

typedef struct {
    name_kind_t	    kind;
    char    	    *string;
    XID	    	    xid;
    int		    index;
} name_t;

typedef struct _crtc crtc_t;
typedef struct _output	output_t;
typedef struct _umode	umode_t;
typedef struct _output_prop output_prop_t;

struct _crtc {
    name_t	    crtc;
    Bool	    changing;
    XRRCrtcInfo	    *crtc_info;

    XRRModeInfo	    *mode_info;
    int		    x;
    int		    y;
    Rotation	    rotation;
    output_t	    **outputs;
    int		    noutput;
};

struct _output_prop {
    struct _output_prop	*next;
    char		*name;
    char		*value;
};

struct _output {
    struct _output   *next;
    
    changes_t	    changes;
    
    output_prop_t   *props;

    name_t	    output;
    XRROutputInfo   *output_info;
    
    name_t	    crtc;
    crtc_t	    *crtc_info;
    crtc_t	    *current_crtc_info;
    
    name_t	    mode;
    float	    refresh;
    XRRModeInfo	    *mode_info;
    
    name_t	    addmode;

    relation_t	    relation;
    char	    *relative_to;

    int		    x, y;
    Rotation	    rotation;
    
    Bool    	    automatic;
};

typedef enum _umode_action {
    umode_create, umode_destroy, umode_add, umode_delete
} umode_action_t;


struct _umode {
    struct _umode   *next;
    
    umode_action_t  action;
    XRRModeInfo	    mode;
    name_t	    output;
    name_t	    name;
};

/*

static char *connection[3] = {
    "connected",
    "disconnected",
    "unknown connection"};

*/

static char *connection[3] = {
    "true",
    "false",
    "unknown"};

#define OUTPUT_NAME 1

#define CRTC_OFF    2
#define CRTC_UNSET  3
#define CRTC_INDEX  0x40000000

#define MODE_NAME   1
#define MODE_OFF    2
#define MODE_UNSET  3
#define MODE_PREF   4

#define POS_UNSET   -1

static output_t	*outputs = NULL;
static output_t	**outputs_tail = &outputs;
static crtc_t	*crtcs;
static umode_t	*umodes;
static int	num_crtcs;
static XRRScreenResources  *res;
static int	fb_width = 0, fb_height = 0;
static int	fb_width_mm = 0, fb_height_mm = 0;
static float	dpi = 0;
static char	*dpi_output = NULL;
static Bool	dryrun = False;
static int	minWidth, maxWidth, minHeight, maxHeight;
static Bool    	has_1_2 = False;

static int
mode_height (XRRModeInfo *mode_info, Rotation rotation)
{
    switch (rotation & 0xf) {
    case RR_Rotate_0:
    case RR_Rotate_180:
	return mode_info->height;
    case RR_Rotate_90:
    case RR_Rotate_270:
	return mode_info->width;
    default:
	return 0;
    }
}

static int
mode_width (XRRModeInfo *mode_info, Rotation rotation)
{
    switch (rotation & 0xf) {
    case RR_Rotate_0:
    case RR_Rotate_180:
	return mode_info->width;
    case RR_Rotate_90:
    case RR_Rotate_270:
	return mode_info->height;
    default:
	return 0;
    }
}

/* v refresh frequency in Hz */
static float
mode_refresh (XRRModeInfo *mode_info)
{
    float rate;
    
    if (mode_info->hTotal && mode_info->vTotal)
	rate = ((float) mode_info->dotClock / 
		((float) mode_info->hTotal * (float) mode_info->vTotal));
    else
    	rate = 0;
    return rate;
}

/* h sync frequency in Hz */
static float
mode_hsync (XRRModeInfo *mode_info)
{
    float rate;
    
    if (mode_info->hTotal)
	rate = (float) mode_info->dotClock / (float) mode_info->hTotal;
    else
    	rate = 0;
    return rate;
}

static void
init_name (name_t *name)
{
    name->kind = name_none;
}

static void
set_name_string (name_t *name, char *string)
{
    name->kind |= name_string;
    name->string = string;
}

static void
set_name_xid (name_t *name, XID xid)
{
    name->kind |= name_xid;
    name->xid = xid;
}

static void
set_name_index (name_t *name, int index)
{
    name->kind |= name_index;
    name->index = index;
}

static void
set_name_preferred (name_t *name)
{
    name->kind |= name_preferred;
}

static void
set_name_all (name_t *name, name_t *old)
{
    if (old->kind & name_xid)
	name->xid = old->xid;
    if (old->kind & name_string)
	name->string = old->string;
    if (old->kind & name_index)
	name->index = old->index;
    name->kind |= old->kind;
}

static void
set_name (name_t *name, char *string, name_kind_t valid)
{
    XID	xid;
    int index;

    if ((valid & name_xid) && sscanf (string, "0x%x", &xid) == 1)
	set_name_xid (name, xid);
    else if ((valid & name_index) && sscanf (string, "%d", &index) == 1)
	set_name_index (name, index);
    else if (valid & name_string)
	set_name_string (name, string);
    else
	usage ();
}

static output_t *
add_output (void)
{
    output_t *output = calloc (1, sizeof (output_t));

    if (!output)
	fatal ("out of memory");
    output->next = NULL;
    *outputs_tail = output;
    outputs_tail = &output->next;
    return output;
}

static output_t *
find_output (name_t *name)
{
    output_t *output;

    for (output = outputs; output; output = output->next)
    {
	name_kind_t common = name->kind & output->output.kind;
	
	if ((common & name_xid) && name->xid == output->output.xid)
	    break;
	if ((common & name_string) && !strcmp (name->string, output->output.string))
	    break;
	if ((common & name_index) && name->index == output->output.index)
	    break;
    }
    return output;
}

static output_t *
find_output_by_xid (RROutput output)
{
    name_t  output_name;

    init_name (&output_name);
    set_name_xid (&output_name, output);
    return find_output (&output_name);
}

static output_t *
find_output_by_name (char *name)
{
    name_t  output_name;

    init_name (&output_name);
    set_name_string (&output_name, name);
    return find_output (&output_name);
}

static crtc_t *
find_crtc (name_t *name)
{
    int	    c;
    crtc_t  *crtc = NULL;

    for (c = 0; c < num_crtcs; c++)
    {
	name_kind_t common;
	
	crtc = &crtcs[c];
	common = name->kind & crtc->crtc.kind;
	
	if ((common & name_xid) && name->xid == crtc->crtc.xid)
	    break;
	if ((common & name_string) && !strcmp (name->string, crtc->crtc.string))
	    break;
	if ((common & name_index) && name->index == crtc->crtc.index)
	    break;
	crtc = NULL;
    }
    return crtc;
}

static crtc_t *
find_crtc_by_xid (RRCrtc crtc)
{
    name_t  crtc_name;

    init_name (&crtc_name);
    set_name_xid (&crtc_name, crtc);
    return find_crtc (&crtc_name);
}

static XRRModeInfo *
find_mode (name_t *name, float refresh)
{
    int		m;
    XRRModeInfo	*best = NULL;
    float	bestDist = 0;

    for (m = 0; m < res->nmode; m++)
    {
	XRRModeInfo *mode = &res->modes[m];
	if ((name->kind & name_xid) && name->xid == mode->id)
	{
	    best = mode;
	    break;
	}
	if ((name->kind & name_string) && !strcmp (name->string, mode->name))
	{
	    float   dist;
	    
	    if (refresh)
		dist = fabs (mode_refresh (mode) - refresh);
	    else
		dist = 0;
	    if (!best || dist < bestDist)
	    {
		bestDist = dist;
		best = mode;
	    }
	    break;
	}
    }
    return best;
}

static XRRModeInfo *
find_mode_by_xid (RRMode mode)
{
    name_t  mode_name;

    init_name (&mode_name);
    set_name_xid (&mode_name, mode);
    return find_mode (&mode_name, 0);
}

static XRRModeInfo *
find_mode_by_name (char *name)
{
    name_t  mode_name;
    init_name (&mode_name);
    set_name_string (&mode_name, name);
    return find_mode (&mode_name, 0);
}

static
XRRModeInfo *
find_mode_for_output (output_t *output, name_t *name)
{
    XRROutputInfo   *output_info = output->output_info;
    int		    m;
    XRRModeInfo	    *best = NULL;
    float	    bestDist = 0;

    for (m = 0; m < output_info->nmode; m++)
    {
	XRRModeInfo	    *mode;
	
	mode = find_mode_by_xid (output_info->modes[m]);
	if (!mode) continue;
	if ((name->kind & name_xid) && name->xid == mode->id)
	{
	    best = mode;
	    break;
	}
	if ((name->kind & name_string) && !strcmp (name->string, mode->name))
	{
	    float   dist;
	    
	    if (output->refresh)
		dist = fabs (mode_refresh (mode) - output->refresh);
	    else
		dist = 0;
	    if (!best || dist < bestDist)
	    {
		bestDist = dist;
		best = mode;
	    }
	}
    }
    return best;
}

XRRModeInfo *
preferred_mode (output_t *output)
{
    XRROutputInfo   *output_info = output->output_info;
    int		    m;
    XRRModeInfo	    *best;
    int		    bestDist;
    
    best = NULL;
    bestDist = 0;
    for (m = 0; m < output_info->nmode; m++)
    {
	XRRModeInfo *mode_info = find_mode_by_xid (output_info->modes[m]);
	int	    dist;
	
	if (m < output_info->npreferred)
	    dist = 0;
	else if (output_info->mm_height)
	    dist = (1000 * DisplayHeight(dpy, screen) / DisplayHeightMM(dpy, screen) -
		    1000 * mode_info->height / output_info->mm_height);
	else
	    dist = DisplayHeight(dpy, screen) - mode_info->height;

        if (dist < 0) dist = -dist;
	if (!best || dist < bestDist)
	{
	    best = mode_info;
	    bestDist = dist;
	}
    }
    return best;
}

static Bool
output_can_use_crtc (output_t *output, crtc_t *crtc)
{
    XRROutputInfo   *output_info = output->output_info;
    int		    c;

    for (c = 0; c < output_info->ncrtc; c++)
	if (output_info->crtcs[c] == crtc->crtc.xid)
	    return True;
    return False;
}

static Bool
output_can_use_mode (output_t *output, XRRModeInfo *mode)
{
    XRROutputInfo   *output_info = output->output_info;
    int		    m;

    for (m = 0; m < output_info->nmode; m++)
	if (output_info->modes[m] == mode->id)
	    return True;
    return False;
}

static Bool
crtc_can_use_rotation (crtc_t *crtc, Rotation rotation)
{
    Rotation	rotations = crtc->crtc_info->rotations;
    Rotation	dir = rotation & (RR_Rotate_0|RR_Rotate_90|RR_Rotate_180|RR_Rotate_270);
    Rotation	reflect = rotation & (RR_Reflect_X|RR_Reflect_Y);
    if (((rotations & dir) != 0) && ((rotations & reflect) == reflect))
	return True;
    return False;
}

/*
 * Report only rotations that are supported by all crtcs
 */
static Rotation
output_rotations (output_t *output)
{
    Bool	    found = False;
    Rotation	    rotation = RR_Rotate_0;
    XRROutputInfo   *output_info = output->output_info;
    int		    c;
    
    for (c = 0; c < output_info->ncrtc; c++)
    {
	crtc_t	*crtc = find_crtc_by_xid (output_info->crtcs[c]);
	if (crtc)
	{
	    if (!found) {
		rotation = crtc->crtc_info->rotations;
		found = True;
	    } else
		rotation &= crtc->crtc_info->rotations;
	}
    }
    return rotation;
}

static Bool
output_can_use_rotation (output_t *output, Rotation rotation)
{
    XRROutputInfo   *output_info = output->output_info;
    int		    c;

    /* make sure all of the crtcs can use this rotation.
     * yes, this is not strictly necessary, but it is 
     * simpler,and we expect most drivers to either
     * support rotation everywhere or nowhere
     */
    for (c = 0; c < output_info->ncrtc; c++)
    {
	crtc_t	*crtc = find_crtc_by_xid (output_info->crtcs[c]);
	if (crtc && !crtc_can_use_rotation (crtc, rotation))
	    return False;
    }
    return True;
}

static void
set_output_info (output_t *output, RROutput xid, XRROutputInfo *output_info)
{
    /* sanity check output info */
    if (output_info->connection != RR_Disconnected && !output_info->nmode)
	fatal ("Output %s is not disconnected but has no modes\n",
	       output_info->name);
    
    /* set output name and info */
    if (!(output->output.kind & name_xid))
	set_name_xid (&output->output, xid);
    if (!(output->output.kind & name_string))
	set_name_string (&output->output, output_info->name);
    output->output_info = output_info;
    
    /* set crtc name and info */
    if (!(output->changes & changes_crtc))
	set_name_xid (&output->crtc, output_info->crtc);
    
    if (output->crtc.kind == name_xid && output->crtc.xid == None)
	output->crtc_info = NULL;
    else
    {
	output->crtc_info = find_crtc (&output->crtc);
	if (!output->crtc_info)
	{
	    if (output->crtc.kind & name_xid)
		fatal ("cannot find crtc 0x%x\n", output->crtc.xid);
	    if (output->crtc.kind & name_index)
		fatal ("cannot find crtc %d\n", output->crtc.index);
	}
	if (!output_can_use_crtc (output, output->crtc_info))
	    fatal ("output %s cannot use crtc 0x%x\n", output->output.string,
		   output->crtc_info->crtc.xid);
    }

    /* set mode name and info */
    if (!(output->changes & changes_mode))
    {
	if (output->crtc_info)
	    set_name_xid (&output->mode, output->crtc_info->crtc_info->mode);
	else
	    set_name_xid (&output->mode, None);
	if (output->mode.xid)
	{
	    output->mode_info = find_mode_by_xid (output->mode.xid);
	    if (!output->mode_info)
		fatal ("server did not report mode 0x%x for output %s\n",
		       output->mode.xid, output->output.string);
	}
	else
	    output->mode_info = NULL;
    }
    else if (output->mode.kind == name_xid && output->mode.xid == None)
	output->mode_info = NULL;
    else
    {
	if (output->mode.kind == name_preferred)
	    output->mode_info = preferred_mode (output);
	else
	    output->mode_info = find_mode_for_output (output, &output->mode);
	if (!output->mode_info)
	{
	    if (output->mode.kind & name_preferred)
		fatal ("cannot find preferred mode\n");
	    if (output->mode.kind & name_string)
		fatal ("cannot find mode %s\n", output->mode.string);
	    if (output->mode.kind & name_xid)
		fatal ("cannot find mode 0x%x\n", output->mode.xid);
	}
	if (!output_can_use_mode (output, output->mode_info))
	    fatal ("output %s cannot use mode %s\n", output->output.string,
		   output->mode_info->name);
    }

    /* set position */
    if (!(output->changes & changes_position))
    {
	if (output->crtc_info)
	{
	    output->x = output->crtc_info->crtc_info->x;
	    output->y = output->crtc_info->crtc_info->y;
	}
	else
	{
	    output->x = 0;
	    output->y = 0;
	}
    }

    /* set rotation */
    if (!(output->changes & changes_rotation))
    {
	output->rotation &= ~0xf;
	if (output->crtc_info)
	    output->rotation |= (output->crtc_info->crtc_info->rotation & 0xf);
	else
	    output->rotation = RR_Rotate_0;
    }
    if (!(output->changes & changes_reflection))
    {
	output->rotation &= ~(RR_Reflect_X|RR_Reflect_Y);
	if (output->crtc_info)
	    output->rotation |= (output->crtc_info->crtc_info->rotation &
				 (RR_Reflect_X|RR_Reflect_Y));
    }
    if (!output_can_use_rotation (output, output->rotation))
	fatal ("output %s cannot use rotation \"%s\" reflection \"%s\"\n",
	       output->output.string,
	       rotation_name (output->rotation),
	       reflection_name (output->rotation));
}
    
static void
get_screen (void)
{
    if (!has_1_2)
        fatal ("Server RandR version before 1.2\n");
    
    XRRGetScreenSizeRange (dpy, root, &minWidth, &minHeight,
			   &maxWidth, &maxHeight);
    
    res = XRRGetScreenResources (dpy, root);
    if (!res) fatal ("could not get screen resources");
}

static void
get_crtcs (void)
{
    int		c;

    num_crtcs = res->ncrtc;
    crtcs = calloc (num_crtcs, sizeof (crtc_t));
    if (!crtcs) fatal ("out of memory");
    
    for (c = 0; c < res->ncrtc; c++)
    {
	XRRCrtcInfo *crtc_info = XRRGetCrtcInfo (dpy, res, res->crtcs[c]);
	set_name_xid (&crtcs[c].crtc, res->crtcs[c]);
	set_name_index (&crtcs[c].crtc, c);
	if (!crtc_info) fatal ("could not get crtc 0x%x information", res->crtcs[c]);
	crtcs[c].crtc_info = crtc_info;
	if (crtc_info->mode == None)
	{
	    crtcs[c].mode_info = NULL;
	    crtcs[c].x = 0;
	    crtcs[c].y = 0;
	    crtcs[c].rotation = RR_Rotate_0;
	}
    }
}

static void
crtc_add_output (crtc_t *crtc, output_t *output)
{
    if (crtc->outputs)
	crtc->outputs = realloc (crtc->outputs, (crtc->noutput + 1) * sizeof (output_t *));
    else
    {
	crtc->outputs = malloc (sizeof (output_t *));
	crtc->x = output->x;
	crtc->y = output->y;
	crtc->rotation = output->rotation;
	crtc->mode_info = output->mode_info;
    }
    if (!crtc->outputs) fatal ("out of memory");
    crtc->outputs[crtc->noutput++] = output;
}

static void
set_crtcs (void)
{
    output_t	*output;

    for (output = outputs; output; output = output->next)
    {
	if (!output->mode_info) continue;
	crtc_add_output (output->crtc_info, output);
    }
}

static Status
crtc_disable (crtc_t *crtc)
{
    if (verbose)
    	printf ("crtc %d: disable\n", crtc->crtc.index);
	
    if (dryrun)
	return RRSetConfigSuccess;
    return XRRSetCrtcConfig (dpy, res, crtc->crtc.xid, CurrentTime,
			     0, 0, None, RR_Rotate_0, NULL, 0);
}

static Status
crtc_revert (crtc_t *crtc)
{
    XRRCrtcInfo	*crtc_info = crtc->crtc_info;
    
    if (verbose)
    	printf ("crtc %d: revert\n", crtc->crtc.index);
	
    if (dryrun)
	return RRSetConfigSuccess;
    return XRRSetCrtcConfig (dpy, res, crtc->crtc.xid, CurrentTime,
			    crtc_info->x, crtc_info->y,
			    crtc_info->mode, crtc_info->rotation,
			    crtc_info->outputs, crtc_info->noutput);
}

static Status
crtc_apply (crtc_t *crtc)
{
    RROutput	*rr_outputs;
    int		o;
    Status	s;
    RRMode	mode = None;

    if (!crtc->changing || !crtc->mode_info)
	return RRSetConfigSuccess;

    rr_outputs = calloc (crtc->noutput, sizeof (RROutput));
    if (!rr_outputs)
	return BadAlloc;
    for (o = 0; o < crtc->noutput; o++)
	rr_outputs[o] = crtc->outputs[o]->output.xid;
    mode = crtc->mode_info->id;
    if (verbose) {
	printf ("crtc %d: %12s %6.1f +%d+%d", crtc->crtc.index,
		crtc->mode_info->name, mode_refresh (crtc->mode_info),
		crtc->x, crtc->y);
	for (o = 0; o < crtc->noutput; o++)
	    printf (" \"%s\"", crtc->outputs[o]->output.string);
	printf ("\n");
    }
    
    if (dryrun)
	s = RRSetConfigSuccess;
    else
	s = XRRSetCrtcConfig (dpy, res, crtc->crtc.xid, CurrentTime,
			      crtc->x, crtc->y, mode, crtc->rotation,
			      rr_outputs, crtc->noutput);
    free (rr_outputs);
    return s;
}

static void
screen_revert (void)
{
    if (verbose)
	printf ("screen %d: revert\n", screen);

    if (dryrun)
	return;
    XRRSetScreenSize (dpy, root,
		      DisplayWidth (dpy, screen),
		      DisplayHeight (dpy, screen),
		      DisplayWidthMM (dpy, screen),
		      DisplayHeightMM (dpy, screen));
}

static void
screen_apply (void)
{
    if (fb_width == DisplayWidth (dpy, screen) &&
	fb_height == DisplayHeight (dpy, screen) &&
	fb_width_mm == DisplayWidthMM (dpy, screen) &&
	fb_height_mm == DisplayHeightMM (dpy, screen))
    {
	return;
    }
    if (verbose)
	printf ("screen %d: %dx%d %dx%d mm %6.2fdpi\n", screen,
		fb_width, fb_height, fb_width_mm, fb_height_mm, dpi);
    if (dryrun)
	return;
    XRRSetScreenSize (dpy, root, fb_width, fb_height,
		      fb_width_mm, fb_height_mm);
}

static void
revert (void)
{
    int	c;

    /* first disable all crtcs */
    for (c = 0; c < res->ncrtc; c++)
	crtc_disable (&crtcs[c]);
    /* next reset screen size */
    screen_revert ();
    /* now restore all crtcs */
    for (c = 0; c < res->ncrtc; c++)
	crtc_revert (&crtcs[c]);
}

/*
 * uh-oh, something bad happened in the middle of changing
 * the configuration. Revert to the previous configuration
 * and bail
 */
static void
panic (Status s, crtc_t *crtc)
{
    int	    c = crtc->crtc.index;
    char    *message;
    
    switch (s) {
    case RRSetConfigSuccess:		message = "succeeded";		    break;
    case BadAlloc:			message = "out of memory";	    break;
    case RRSetConfigFailed:		message = "failed";		    break;
    case RRSetConfigInvalidConfigTime:	message = "invalid config time";    break;
    case RRSetConfigInvalidTime:	message = "invalid time";	    break;
    default:				message = "unknown failure";	    break;
    }
    
    fprintf (stderr, "%s: Configure crtc %d %s\n", program_name, c, message);
    revert ();
    exit (1);
}

void
apply (void)
{
    Status  s;
    int	    c;
    
    /*
     * Turn off any crtcs which are to be disabled or which are
     * larger than the target size
     */
    for (c = 0; c < res->ncrtc; c++)
    {
	crtc_t	    *crtc = &crtcs[c];
	XRRCrtcInfo *crtc_info = crtc->crtc_info;

	/* if this crtc is already disabled, skip it */
	if (crtc_info->mode == None) 
	    continue;
	
	/* 
	 * If this crtc is to be left enabled, make
	 * sure the old size fits then new screen
	 */
	if (crtc->mode_info) 
	{
	    XRRModeInfo	*old_mode = find_mode_by_xid (crtc_info->mode);
	    int x, y, w, h;

	    if (!old_mode) 
		panic (RRSetConfigFailed, crtc);
	    
	    /* old position and size information */
	    x = crtc_info->x;
	    y = crtc_info->y;
	    w = mode_width (old_mode, crtc_info->rotation);
	    h = mode_height (old_mode, crtc_info->rotation);
	    
	    /* if it fits, skip it */
	    if (x + w <= fb_width && y + h <= fb_height) 
		continue;
	    crtc->changing = True;
	}
	s = crtc_disable (crtc);
	if (s != RRSetConfigSuccess)
	    panic (s, crtc);
    }

    /*
     * Hold the server grabbed while messing with
     * the screen so that apps which notice the resize
     * event and ask for xinerama information from the server
     * receive up-to-date information
     */
    XGrabServer (dpy);
    
    /*
     * Set the screen size
     */
    screen_apply ();
    
    /*
     * Set crtcs
     */

    for (c = 0; c < res->ncrtc; c++)
    {
	crtc_t	*crtc = &crtcs[c];
	
	s = crtc_apply (crtc);
	if (s != RRSetConfigSuccess)
	    panic (s, crtc);
    }
    /*
     * Release the server grab and let all clients
     * respond to the updated state
     */
    XUngrabServer (dpy);
}

/*
 * Use current output state to complete the output list
 */
void
get_outputs (void)
{
    int		o;
    
    for (o = 0; o < res->noutput; o++)
    {
	XRROutputInfo	*output_info = XRRGetOutputInfo (dpy, res, res->outputs[o]);
	output_t	*output;
	name_t		output_name;
	if (!output_info) fatal ("could not get output 0x%x information", res->outputs[o]);
	set_name_xid (&output_name, res->outputs[o]);
	set_name_index (&output_name, o);
	set_name_string (&output_name, output_info->name);
	output = find_output (&output_name);
	if (!output)
	{
	    output = add_output ();
	    set_name_all (&output->output, &output_name);
	    /*
	     * When global --automatic mode is set, turn on connected but off
	     * outputs, turn off disconnected but on outputs
	     */
	    if (automatic)
	    {
		switch (output_info->connection) {
		case RR_Connected:
		    if (!output_info->crtc) {
			output->changes |= changes_automatic;
			output->automatic = True;
		    }
		    break;
		case RR_Disconnected:
		    if (output_info->crtc)
		    {
			output->changes |= changes_automatic;
			output->automatic = True;
		    }
		    break;
		}
	    }
	}

	/*
	 * Automatic mode -- track connection state and enable/disable outputs
	 * as necessary
	 */
	if (output->automatic)
	{
	    switch (output_info->connection) {
	    case RR_Connected:
	    case RR_UnknownConnection:
		if ((!(output->changes & changes_mode)))
		{
		    set_name_preferred (&output->mode);
		    output->changes |= changes_mode;
		}
		break;
	    case RR_Disconnected:
		if ((!(output->changes & changes_mode)))
		{
		    set_name_xid (&output->mode, None);
		    set_name_xid (&output->crtc, None);
		    output->changes |= changes_mode;
		    output->changes |= changes_crtc;
		}
		break;
	    }
	}

	set_output_info (output, res->outputs[o], output_info);
    }
}

void
mark_changing_crtcs (void)
{
    int	c;

    for (c = 0; c < num_crtcs; c++)
    {
	crtc_t	    *crtc = &crtcs[c];
	int	    o;
	output_t    *output;

	/* walk old output list (to catch disables) */
	for (o = 0; o < crtc->crtc_info->noutput; o++)
	{
	    output = find_output_by_xid (crtc->crtc_info->outputs[o]);
	    if (!output) fatal ("cannot find output 0x%x\n",
				crtc->crtc_info->outputs[o]);
	    if (output->changes)
		crtc->changing = True;
	}
	/* walk new output list */
	for (o = 0; o < crtc->noutput; o++)
	{
	    output = crtc->outputs[o];
	    if (output->changes)
		crtc->changing = True;
	}
    }
}

/*
 * Test whether 'crtc' can be used for 'output'
 */
Bool
check_crtc_for_output (crtc_t *crtc, output_t *output)
{
    int		c;
    int		l;
    output_t    *other;
    
    for (c = 0; c < output->output_info->ncrtc; c++)
	if (output->output_info->crtcs[c] == crtc->crtc.xid)
	    break;
    if (c == output->output_info->ncrtc)
	return False;
    for (other = outputs; other; other = other->next)
    {
	if (other == output)
	    continue;

	if (other->mode_info == NULL)
	    continue;

	if (other->crtc_info != crtc)
	    continue;

	/* see if the output connected to the crtc can clone to this output */
	for (l = 0; l < output->output_info->nclone; l++)
	    if (output->output_info->clones[l] == other->output.xid)
		break;
	/* not on the list, can't clone */
	if (l == output->output_info->nclone) 
	    return False;
    }

    if (crtc->noutput)
    {
	/* make sure the state matches */
	if (crtc->mode_info != output->mode_info)
	    return False;
	if (crtc->x != output->x)
	    return False;
	if (crtc->y != output->y)
	    return False;
	if (crtc->rotation != output->rotation)
	    return False;
    }
    return True;
}

crtc_t *
find_crtc_for_output (output_t *output)
{
    int	    c;

    for (c = 0; c < output->output_info->ncrtc; c++)
    {
	crtc_t	    *crtc;

	crtc = find_crtc_by_xid (output->output_info->crtcs[c]);
	if (!crtc) fatal ("cannot find crtc 0x%x\n", output->output_info->crtcs[c]);

	if (check_crtc_for_output (crtc, output))
	    return crtc;
    }
    return NULL;
}

static void
set_positions (void)
{
    output_t	*output;
    Bool	keep_going;
    Bool	any_set;
    int		min_x, min_y;

    for (;;)
    {
	any_set = False;
	keep_going = False;
	for (output = outputs; output; output = output->next)
	{
	    output_t    *relation;
	    name_t	relation_name;

	    if (!(output->changes & changes_relation)) continue;
	    
	    if (output->mode_info == NULL) continue;

	    init_name (&relation_name);
	    set_name_string (&relation_name, output->relative_to);
	    relation = find_output (&relation_name);
	    if (!relation) fatal ("cannot find output \"%s\"\n", output->relative_to);
	    
	    if (relation->mode_info == NULL) 
	    {
		output->x = 0;
		output->y = 0;
		output->changes |= changes_position;
		any_set = True;
		continue;
	    }
	    /*
	     * Make sure the dependent object has been set in place
	     */
	    if ((relation->changes & changes_relation) && 
		!(relation->changes & changes_position))
	    {
		keep_going = True;
		continue;
	    }
	    
	    switch (output->relation) {
	    case left_of:
		output->y = relation->y;
		output->x = relation->x - mode_width (output->mode_info, output->rotation);
		break;
	    case right_of:
		output->y = relation->y;
		output->x = relation->x + mode_width (relation->mode_info, relation->rotation);
		break;
	    case above:
		output->x = relation->x;
		output->y = relation->y - mode_height (output->mode_info, output->rotation);
		break;
	    case below:
		output->x = relation->x;
		output->y = relation->y + mode_height (relation->mode_info, relation->rotation);
		break;
	    case same_as:
		output->x = relation->x;
		output->y = relation->y;
	    }
	    output->changes |= changes_position;
	    any_set = True;
	}
	if (!keep_going)
	    break;
	if (!any_set)
	    fatal ("loop in relative position specifications\n");
    }

    /*
     * Now normalize positions so the upper left corner of all outputs is at 0,0
     */
    min_x = 32768;
    min_y = 32768;
    for (output = outputs; output; output = output->next)
    {
	if (output->mode_info == NULL) continue;
	
	if (output->x < min_x) min_x = output->x;
	if (output->y < min_y) min_y = output->y;
    }
    if (min_x || min_y)
    {
	/* move all outputs */
	for (output = outputs; output; output = output->next)
	{
	    if (output->mode_info == NULL) continue;

	    output->x -= min_x;
	    output->y -= min_y;
	    output->changes |= changes_position;
	}
    }
}

static void
set_screen_size (void)
{
    output_t	*output;
    Bool	fb_specified = fb_width != 0 && fb_height != 0;
    
    for (output = outputs; output; output = output->next)
    {
	XRRModeInfo *mode_info = output->mode_info;
	int	    x, y, w, h;
	
	if (!mode_info) continue;
	
	x = output->x;
	y = output->y;
	w = mode_width (mode_info, output->rotation);
	h = mode_height (mode_info, output->rotation);
	/* make sure output fits in specified size */
	if (fb_specified)
	{
	    if (x + w > fb_width || y + h > fb_height)
		fatal ("specified screen %dx%d not large enough for output %s (%dx%d+%d+%d)\n",
		       fb_width, fb_height, output->output.string, w, h, x, y);
	}
	/* fit fb to output */
	else
	{
	    if (x + w > fb_width) fb_width = x + w;
	    if (y + h > fb_height) fb_height = y + h;
	}
    }	

    if (fb_width > maxWidth || fb_height > maxHeight)
        fatal ("screen cannot be larger than %dx%d (desired size %dx%d)\n",
	       maxWidth, maxHeight, fb_width, fb_height);
    if (fb_specified)
    {
	if (fb_width < minWidth || fb_height < minHeight)
	    fatal ("screen must be at least %dx%d\n", minWidth, minHeight);
    }
    else
    {
	if (fb_width < minWidth) fb_width = minWidth;
	if (fb_height < minHeight) fb_height = minHeight;
    }
}
    
#endif
    
void
disable_outputs (output_t *outputs)
{
    while (outputs)
    {
	outputs->crtc_info = NULL;
	outputs = outputs->next;
    }
}

/*
 * find the best mapping from output to crtc available
 */
int
pick_crtcs_score (output_t *outputs)
{
    output_t	*output;
    int		best_score;
    int		my_score;
    int		score;
    crtc_t	*best_crtc;
    int		c;
    
    if (!outputs)
	return 0;
    
    output = outputs;
    outputs = outputs->next;
    /*
     * Score with this output disabled
     */
    output->crtc_info = NULL;
    best_score = pick_crtcs_score (outputs);
    if (output->mode_info == NULL)
	return best_score;

    best_crtc = NULL;
    /* 
     * Now score with this output any valid crtc
     */
    for (c = 0; c < output->output_info->ncrtc; c++)
    {
	crtc_t	    *crtc;

	crtc = find_crtc_by_xid (output->output_info->crtcs[c]);
	if (!crtc)
	    fatal ("cannot find crtc 0x%x\n", output->output_info->crtcs[c]);
	
	/* reset crtc allocation for following outputs */
	disable_outputs (outputs);
	if (!check_crtc_for_output (crtc, output))
	    continue;
	
	my_score = 1000;
	/* slight preference for existing connections */
	if (crtc == output->current_crtc_info)
	    my_score++;

	output->crtc_info = crtc;
	score = my_score + pick_crtcs_score (outputs);
	if (score > best_score)
	{
	    best_crtc = crtc;
	    best_score = score;
	}
    }
    /*
     * Reset other outputs based on this one using the best crtc
     */
    if (output->crtc_info != best_crtc)
    {
	output->crtc_info = best_crtc;
	(void) pick_crtcs_score (outputs);
    }
    return best_score;
}

/*
 * Pick crtcs for any changing outputs that don't have one
 */
void
pick_crtcs (void)
{
    output_t	*output;

    /*
     * First try to match up newly enabled outputs with spare crtcs
     */
    for (output = outputs; output; output = output->next)
    {
	if (output->changes && output->mode_info && !output->crtc_info)
	{
	    output->crtc_info = find_crtc_for_output (output);
	    if (!output->crtc_info)
		break;
	}
    }
    /*
     * Everyone is happy
     */
    if (!output)
	return;
    /*
     * When the simple way fails, see if there is a way
     * to swap crtcs around and make things work
     */
    for (output = outputs; output; output = output->next)
	output->current_crtc_info = output->crtc_info;
    pick_crtcs_score (outputs);
    for (output = outputs; output; output = output->next)
    {
	if (output->mode_info && !output->crtc_info)
	    fatal ("cannot find crtc for output %s\n", output->output.string);
	if (!output->changes && output->crtc_info != output->current_crtc_info)
	    output->changes |= changes_crtc;
    }
}

int
main (int argc, char **argv)
{
    XRRScreenSize *sizes;
    XRRScreenConfiguration *sc;
    int		nsize;
    int		nrate;
    short		*rates;
    Status	status = RRSetConfigFailed;
    int		rot = -1;
    int		query = 0;
    Rotation	rotation, current_rotation, rotations;
    XEvent	event;
    XRRScreenChangeNotifyEvent *sce;    
    char          *display_name = NULL;
    int 		i, j;
    SizeID	current_size;
    short	current_rate;
    float    	rate = -1;
    int		size = -1;
    int		dirind = 0;
    Bool	setit = False;
    Bool    	version = False;
    int		event_base, error_base;
    int		reflection = 0;
    int		width = 0, height = 0;
    Bool    	have_pixel_size = False;
    int		ret = 0;
#if HAS_RANDR_1_2
    output_t	*output = NULL;
    policy_t	policy = clone;
    Bool    	setit_1_2 = False;
    Bool    	query_1_2 = False;
    Bool	modeit = False;
    Bool	propit = False;
    Bool	query_1 = False;
    int		major, minor;
#endif

    program_name = argv[0];
    if (argc == 1) query = True;
    for (i = 1; i < argc; i++) {
	if (!strcmp ("-display", argv[i]) || !strcmp ("-d", argv[i])) {
	    if (++i>=argc) usage ();
	    display_name = argv[i];
	    continue;
	}
	if (!strcmp("-help", argv[i])) {
	    usage();
	    continue;
	}
	if (!strcmp ("--verbose", argv[i])) {
	    verbose = True;
	    continue;
	}
	if (!strcmp ("--dryrun", argv[i])) {
	    dryrun = True;
	    verbose = True;
	    continue;
	}

	if (!strcmp ("-s", argv[i]) || !strcmp ("--size", argv[i])) {
	    if (++i>=argc) usage ();
	    if (sscanf (argv[i], "%dx%d", &width, &height) == 2)
		have_pixel_size = True;
	    else {
		size = atoi (argv[i]);
		if (size < 0) usage();
	    }
	    setit = True;
	    continue;
	}

	if (!strcmp ("-r", argv[i]) ||
	    !strcmp ("--rate", argv[i]) ||
	    !strcmp ("--refresh", argv[i]))
	{
	    if (++i>=argc) usage ();
	    if (sscanf (argv[i], "%f", &rate) != 1)
		usage ();
	    setit = True;
#if HAS_RANDR_1_2
	    if (output)
	    {
		output->refresh = rate;
		output->changes |= changes_refresh;
		setit_1_2 = True;
	    }
#endif
	    continue;
	}

	if (!strcmp ("-v", argv[i]) || !strcmp ("--version", argv[i])) {
	    version = True;
	    continue;
	}

	if (!strcmp ("-x", argv[i])) {
	    reflection |= RR_Reflect_X;
	    setit = True;
	    continue;
	}
	if (!strcmp ("-y", argv[i])) {
	    reflection |= RR_Reflect_Y;
	    setit = True;
	    continue;
	}
	if (!strcmp ("--screen", argv[i])) {
	    if (++i>=argc) usage ();
	    screen = atoi (argv[i]);
	    if (screen < 0) usage();
	    continue;
	}
	if (!strcmp ("-q", argv[i]) || !strcmp ("--query", argv[i])) {
	    query = True;
	    continue;
	}
	if (!strcmp ("-o", argv[i]) || !strcmp ("--orientation", argv[i])) {
	    char *endptr;
	    if (++i>=argc) usage ();
	    dirind = strtol(argv[i], &endptr, 0);
	    if (*endptr != '\0') {
		for (dirind = 0; dirind < 4; dirind++) {
		    if (strcmp (direction[dirind], argv[i]) == 0) break;
		}
		if ((dirind < 0) || (dirind > 3))  usage();
	    }
	    rot = dirind;
	    setit = True;
	    continue;
	}
#if HAS_RANDR_1_2
	if (!strcmp ("--prop", argv[i]) || !strcmp ("--properties", argv[i]))
	{
	    query_1_2 = True;
	    properties = True;
	    continue;
	}
	if (!strcmp ("--output", argv[i])) {
	    if (++i >= argc) usage();
	    output = add_output ();

	    set_name (&output->output, argv[i], name_string|name_xid);
	    
	    setit_1_2 = True;
	    continue;
	}
	if (!strcmp ("--crtc", argv[i])) {
	    if (++i >= argc) usage();
	    if (!output) usage();
	    set_name (&output->crtc, argv[i], name_xid|name_index);
	    output->changes |= changes_crtc;
	    continue;
	}
	if (!strcmp ("--mode", argv[i])) {
	    if (++i >= argc) usage();
	    if (!output) usage();
	    set_name (&output->mode, argv[i], name_string|name_xid);
	    output->changes |= changes_mode;
	    continue;
	}
	if (!strcmp ("--preferred", argv[i])) {
	    if (!output) usage();
	    set_name_preferred (&output->mode);
	    output->changes |= changes_mode;
	    continue;
	}
	if (!strcmp ("--pos", argv[i])) {
	    if (++i>=argc) usage ();
	    if (!output) usage();
	    if (sscanf (argv[i], "%dx%d",
			&output->x, &output->y) != 2)
		usage ();
	    output->changes |= changes_position;
	    continue;
	}
	if (!strcmp ("--rotation", argv[i]) || !strcmp ("--rotate", argv[i])) {
	    if (++i>=argc) usage ();
	    if (!output) usage();
	    for (dirind = 0; dirind < 4; dirind++) {
		if (strcmp (direction[dirind], argv[i]) == 0) break;
	    }
	    if (dirind == 4)
		usage ();
	    output->rotation &= ~0xf;
	    output->rotation |= 1 << dirind;
	    output->changes |= changes_rotation;
	    continue;
	}
	if (!strcmp ("--reflect", argv[i]) || !strcmp ("--reflection", argv[i])) {
	    if (++i>=argc) usage ();
	    if (!output) usage();
	    for (dirind = 0; dirind < 4; dirind++) {
		if (strcmp (reflections[dirind], argv[i]) == 0) break;
	    }
	    if (dirind == 4)
		usage ();
	    output->rotation &= ~(RR_Reflect_X|RR_Reflect_Y);
	    output->rotation |= dirind * RR_Reflect_X;
	    output->changes |= changes_reflection;
	    continue;
	}
	if (!strcmp ("--left-of", argv[i])) {
	    if (++i>=argc) usage ();
	    if (!output) usage();
	    output->relation = left_of;
	    output->relative_to = argv[i];
	    output->changes |= changes_relation;
	    continue;
	}
	if (!strcmp ("--right-of", argv[i])) {
	    if (++i>=argc) usage ();
	    if (!output) usage();
	    output->relation = right_of;
	    output->relative_to = argv[i];
	    output->changes |= changes_relation;
	    continue;
	}
	if (!strcmp ("--above", argv[i])) {
	    if (++i>=argc) usage ();
	    if (!output) usage();
	    output->relation = above;
	    output->relative_to = argv[i];
	    output->changes |= changes_relation;
	    continue;
	}
	if (!strcmp ("--below", argv[i])) {
	    if (++i>=argc) usage ();
	    if (!output) usage();
	    output->relation = below;
	    output->relative_to = argv[i];
	    output->changes |= changes_relation;
	    continue;
	}
	if (!strcmp ("--same-as", argv[i])) {
	    if (++i>=argc) usage ();
	    if (!output) usage();
	    output->relation = same_as;
	    output->relative_to = argv[i];
	    output->changes |= changes_relation;
	    continue;
	}
	if (!strcmp ("--set", argv[i])) {
	    output_prop_t   *prop;
	    if (!output) usage();
	    prop = malloc (sizeof (output_prop_t));
	    prop->next = output->props;
	    output->props = prop;
	    if (++i>=argc) usage ();
	    prop->name = argv[i];
	    if (++i>=argc) usage ();
	    prop->value = argv[i];
	    propit = True;
	    output->changes |= changes_property;
	    setit_1_2 = True;
	    continue;
	}
	if (!strcmp ("--off", argv[i])) {
	    if (!output) usage();
	    set_name_xid (&output->mode, None);
	    set_name_xid (&output->crtc, None);
	    output->changes |= changes_mode;
	    continue;
	}
	if (!strcmp ("--fb", argv[i])) {
	    if (++i>=argc) usage ();
	    if (sscanf (argv[i], "%dx%d",
			&fb_width, &fb_height) != 2)
		usage ();
	    setit_1_2 = True;
	    continue;
	}
	if (!strcmp ("--fbmm", argv[i])) {
	    if (++i>=argc) usage ();
	    if (sscanf (argv[i], "%dx%d",
			&fb_width_mm, &fb_height_mm) != 2)
		usage ();
	    setit_1_2 = True;
	    continue;
	}
	if (!strcmp ("--dpi", argv[i])) {
	    if (++i>=argc) usage ();
	    if (sscanf (argv[i], "%f", &dpi) != 1)
	    {
		dpi = 0.0;
		dpi_output = argv[i];
	    }
	    setit_1_2 = True;
	    continue;
	}
	if (!strcmp ("--clone", argv[i])) {
	    policy = clone;
	    setit_1_2 = True;
	    continue;
	}
	if (!strcmp ("--extend", argv[i])) {
	    policy = extend;
	    setit_1_2 = True;
	    continue;
	}
	if (!strcmp ("--auto", argv[i])) {
	    if (output)
	    {
		output->automatic = True;
		output->changes |= changes_automatic;
	    }
	    else
		automatic = True;
	    setit_1_2 = True;
	    continue;
	}
	if (!strcmp ("--q12", argv[i]))
	{
	    query_1_2 = True;
	    continue;
	}
	if (!strcmp ("--q1", argv[i]))
	{
	    query_1 = True;
	    continue;
	}
	if (!strcmp ("--newmode", argv[i]))
	{
	    umode_t  *m = malloc (sizeof (umode_t));
	    float   clock;
	    
	    ++i;
	    if (i + 9 >= argc) usage ();
	    m->mode.name = argv[i];
	    m->mode.nameLength = strlen (argv[i]);
	    i++;
	    if (sscanf (argv[i++], "%f", &clock) != 1)
		usage ();
	    m->mode.dotClock = clock * 1e6;

	    if (sscanf (argv[i++], "%d", &m->mode.width) != 1) usage();
	    if (sscanf (argv[i++], "%d", &m->mode.hSyncStart) != 1) usage();
	    if (sscanf (argv[i++], "%d", &m->mode.hSyncEnd) != 1) usage();
	    if (sscanf (argv[i++], "%d", &m->mode.hTotal) != 1) usage();
	    if (sscanf (argv[i++], "%d", &m->mode.height) != 1) usage();
	    if (sscanf (argv[i++], "%d", &m->mode.vSyncStart) != 1) usage();
	    if (sscanf (argv[i++], "%d", &m->mode.vSyncEnd) != 1) usage();
	    if (sscanf (argv[i++], "%d", &m->mode.vTotal) != 1) usage();
	    m->mode.modeFlags = 0;
	    while (i < argc) {
		int f;
		
		for (f = 0; mode_flags[f].string; f++)
		    if (!strcasecmp (mode_flags[f].string, argv[i]))
			break;
		
		if (!mode_flags[f].string)
		    break;
    		m->mode.modeFlags |= mode_flags[f].flag;
    		i++;
	    }
	    m->next = umodes;
	    m->action = umode_create;
	    umodes = m;
	    modeit = True;
	    continue;
	}
	if (!strcmp ("--rmmode", argv[i]))
	{
	    umode_t  *m = malloc (sizeof (umode_t));

	    if (++i>=argc) usage ();
	    set_name (&m->name, argv[i], name_string|name_xid);
	    m->action = umode_destroy;
	    m->next = umodes;
	    umodes = m;
	    modeit = True;
	    continue;
	}
	if (!strcmp ("--addmode", argv[i]))
	{
	    umode_t  *m = malloc (sizeof (umode_t));

	    if (++i>=argc) usage ();
	    set_name (&m->output, argv[i], name_string|name_xid);
	    if (++i>=argc) usage();
	    set_name (&m->name, argv[i], name_string|name_xid);
	    m->action = umode_add;
	    m->next = umodes;
	    umodes = m;
	    modeit = True;
	    continue;
	}
	if (!strcmp ("--delmode", argv[i]))
	{
	    umode_t  *m = malloc (sizeof (umode_t));

	    if (++i>=argc) usage ();
	    set_name (&m->output, argv[i], name_string|name_xid);
	    if (++i>=argc) usage();
	    set_name (&m->name, argv[i], name_string|name_xid);
	    m->action = umode_delete;
	    m->next = umodes;
	    umodes = m;
	    modeit = True;
	    continue;
	}
#endif
	usage();
    }
    if (verbose) 
    {
	query = True;
	if (setit && !setit_1_2)
	    query_1 = True;
    }

    dpy = XOpenDisplay (display_name);

    if (dpy == NULL) {
	fprintf (stderr, "Can't open display %s\n", XDisplayName(display_name));
	exit (1);
    }
    if (screen < 0)
	screen = DefaultScreen (dpy);
    if (screen >= ScreenCount (dpy)) {
	fprintf (stderr, "Invalid screen number %d (display has %d)\n",
		 screen, ScreenCount (dpy));
	exit (1);
    }

    root = RootWindow (dpy, screen);

#if HAS_RANDR_1_2
    if (!XRRQueryVersion (dpy, &major, &minor))
    {
	fprintf (stderr, "RandR extension missing\n");
	exit (1);
    }
    if (major > 1 || (major == 1 && minor >= 2))
	has_1_2 = True;
	
    if (has_1_2 && modeit)
    {
	umode_t	*m;

        get_screen ();
	get_crtcs();
	get_outputs();
	
	for (m = umodes; m; m = m->next)
	{
	    XRRModeInfo *e;
	    output_t	*o;
	    
	    switch (m->action) {
	    case umode_create:
		XRRCreateMode (dpy, root, &m->mode);
		break;
	    case umode_destroy:
		e = find_mode (&m->name, 0);
		if (!e)
		    fatal ("cannot find mode \"%s\"\n", m->name.string);
		XRRDestroyMode (dpy, e->id);
		break;
	    case umode_add:
		o = find_output (&m->output);
		if (!o)
		    fatal ("cannot find output \"%s\"\n", m->output.string);
		e = find_mode (&m->name, 0);
		if (!e)
		    fatal ("cannot find mode \"%s\"\n", m->name.string);
		XRRAddOutputMode (dpy, o->output.xid, e->id);
		break;
	    case umode_delete:
		o = find_output (&m->output);
		if (!o)
		    fatal ("cannot find output \"%s\"\n", m->output.string);
		e = find_mode (&m->name, 0);
		if (!e)
		    fatal ("cannot find mode \"%s\"\n", m->name.string);
		XRRDeleteOutputMode (dpy, o->output.xid, e->id);
		break;
	    }
	}
	if (!setit_1_2)
	{
	    XSync (dpy, False);
	    exit (0);
	}
    }
    if (has_1_2 && propit)
    {
	
        get_screen ();
	get_crtcs();
	get_outputs();
	
	for (output = outputs; output; output = output->next)
	{
	    output_prop_t   *prop;

	    for (prop = output->props; prop; prop = prop->next)
	    {
		Atom		name = XInternAtom (dpy, prop->name, False);
		Atom		type;
		int		format;
		unsigned char	*data;
		int		nelements;
		int		int_value;
		unsigned long	ulong_value;
		unsigned char	*prop_data;
		int		actual_format;
		unsigned long	nitems, bytes_after;
		Atom		actual_type;
		XRRPropertyInfo *propinfo;

		type = AnyPropertyType;
		format=0;
		
		if (XRRGetOutputProperty (dpy, output->output.xid, name,
					  0, 100, False, False,
					  AnyPropertyType,
					  &actual_type, &actual_format,
					  &nitems, &bytes_after, &prop_data) == Success &&

		    (propinfo = XRRQueryOutputProperty(dpy, output->output.xid,
						      name)))
		{
		    type = actual_type;
		    format = actual_format;
		}
		
		if ((type == XA_INTEGER || type == AnyPropertyType) &&
		    (sscanf (prop->value, "%d", &int_value) == 1 ||
		     sscanf (prop->value, "0x%x", &int_value) == 1))
		{
		    type = XA_INTEGER;
		    ulong_value = int_value;
		    data = (unsigned char *) &ulong_value;
		    nelements = 1;
		    format = 32;
		}
		else if ((type == XA_ATOM))
		{
		    ulong_value = XInternAtom (dpy, prop->value, False);
		    data = (unsigned char *) &ulong_value;
		    nelements = 1;
		    format = 32;
		}
		else if ((type == XA_STRING || type == AnyPropertyType))
		{
		    type = XA_STRING;
		    data = (unsigned char *) prop->value;
		    nelements = strlen (prop->value);
		    format = 8;
		}
		XRRChangeOutputProperty (dpy, output->output.xid,
					 name, type, format, PropModeReplace,
					 data, nelements);
	    }
	}
	if (!setit_1_2)
	{
	    XSync (dpy, False);
	    exit (0);
	}
    }
    if (setit_1_2)
    {
	get_screen ();
	get_crtcs ();
	get_outputs ();
	set_positions ();
	set_screen_size ();

	pick_crtcs ();

	/*
	 * Assign outputs to crtcs
	 */
	set_crtcs ();
	
	/*
	 * Mark changing crtcs
	 */
	mark_changing_crtcs ();

	/*
	 * If an output was specified to track dpi, use it
	 */
	if (dpi_output)
	{
	    output_t	*output = find_output_by_name (dpi_output);
	    XRROutputInfo	*output_info;
	    XRRModeInfo	*mode_info;
	    if (!output)
		fatal ("Cannot find output %s\n", dpi_output);
	    output_info = output->output_info;
	    mode_info = output->mode_info;
	    if (output_info && mode_info && output_info->mm_height)
	    {
		/*
		 * When this output covers the whole screen, just use
		 * the known physical size
		 */
		if (fb_width == mode_info->width &&
		    fb_height == mode_info->height)
		{
		    fb_width_mm = output_info->mm_width;
		    fb_height_mm = output_info->mm_height;
		}
		else
		{
		    dpi = (25.4 * mode_info->height) / output_info->mm_height;
		}
	    }
	}

	/*
	 * Compute physical screen size
	 */
	if (fb_width_mm == 0 || fb_height_mm == 0)
	{
	    if (fb_width != DisplayWidth (dpy, screen) ||
		fb_height != DisplayHeight (dpy, screen) || dpi != 0.0)
	    {
		if (dpi <= 0)
		    dpi = (25.4 * DisplayHeight (dpy, screen)) / DisplayHeightMM(dpy, screen);

		fb_width_mm = (25.4 * fb_width) / dpi;
		fb_height_mm = (25.4 * fb_height) / dpi;
	    }
	    else
	    {
		fb_width_mm = DisplayWidthMM (dpy, screen);
		fb_height_mm = DisplayHeightMM (dpy, screen);
	    }
	}
	
	/*
	 * Now apply all of the changes
	 */
	apply ();
	
	XSync (dpy, False);
	exit (0);
    }
    if (query_1_2 || (query && has_1_2 && !query_1))
    {
	output_t    *output;
	int	    m;
	
#define ModeShown   0x80000000
	
	get_screen ();
	get_crtcs ();
	get_outputs ();

        printf ("<screen id=\"%d\" minimum_w=\"%d\" minimum_h=\"%d\" current_w=\"%d\" current_h=\"%d\" maximum_w=\"%d\" maximum_h=\"%d\">\n",
		screen, minWidth, minHeight,
		DisplayWidth (dpy, screen), DisplayHeight(dpy, screen),
		maxWidth, maxHeight);

	for (output = outputs; output; output = output->next)
	{
	    XRROutputInfo   *output_info = output->output_info;
	    XRRModeInfo	    *mode = output->mode_info;
	    Atom	    *props;
	    int		    j, k, nprop;
	    Bool	    *mode_shown;
	    Rotation	    rotations = output_rotations (output);

	    printf ("  <output name=\"%s\" connected=\"%s\"", output_info->name, connection[output_info->connection]);
	    if (mode)
	    {
		printf (" w=\"%d\" h=\"%d\" x=\"%d\" y=\"%d\"",
			mode_width (mode, output->rotation),
			mode_height (mode, output->rotation),
			output->x, output->y);
		if (verbose)
		    printf (" id=\"%x\"", mode->id);
		if (output->rotation != RR_Rotate_0 || verbose)
		{
		    printf (" rotation=\"%s\"", 
			    rotation_name (output->rotation));
		    if (output->rotation & (RR_Reflect_X|RR_Reflect_Y))
			printf (" reflection=\"%s\"", reflection_name (output->rotation));
		}
	    }
/*
	    if (rotations != RR_Rotate_0 || verbose)
	    {
		Bool    first = True;
		printf (" (");
		for (i = 0; i < 4; i ++) {
		    if ((rotations >> i) & 1) {
			if (!first) printf (" "); first = False;
			printf("%s", direction[i]);
			first = False;
		    }
		}
		if (rotations & RR_Reflect_X)
		{
		    if (!first) printf (" "); first = False;
		    printf ("x axis");
		}
		if (rotations & RR_Reflect_Y)
		{
		    if (!first) printf (" "); first = False;
		    printf ("y axis");
		}
		printf (")");
	    }
*/
	    if (mode)
	    {
		printf (" wmm=\"%d\" hmm=\"%d\"",
			output_info->mm_width, output_info->mm_height);
	    }
	    printf (">\n");

	    if (verbose)
	    {
		printf ("\tIdentifier: 0x%x\n", output->output.xid);
		printf ("\tTimestamp:  %d\n", output_info->timestamp);
		printf ("\tSubpixel:   %s\n", order[output_info->subpixel_order]);
		printf ("\tClones:    ");
		for (j = 0; j < output_info->nclone; j++)
		{
		    output_t	*clone = find_output_by_xid (output_info->clones[j]);

		    if (clone) printf (" %s", clone->output.string);
		}
		printf ("\n");
		if (output->crtc_info)
		    printf ("\tCRTC:       %d\n", output->crtc_info->crtc.index);
		printf ("\tCRTCs:     ");
		for (j = 0; j < output_info->ncrtc; j++)
		{
		    crtc_t	*crtc = find_crtc_by_xid (output_info->crtcs[j]);
		    if (crtc)
			printf (" %d", crtc->crtc.index);
		}
		printf ("\n");
	    }
	    if (verbose || properties)
	    {
		props = XRRListOutputProperties (dpy, output->output.xid,
						 &nprop);
		for (j = 0; j < nprop; j++) {
		    unsigned char *prop;
		    int actual_format;
		    unsigned long nitems, bytes_after;
		    Atom actual_type;
		    XRRPropertyInfo *propinfo;
    
		    XRRGetOutputProperty (dpy, output->output.xid, props[j],
					  0, 100, False, False,
					  AnyPropertyType,
					  &actual_type, &actual_format,
					  &nitems, &bytes_after, &prop);

		    propinfo = XRRQueryOutputProperty(dpy, output->output.xid,
						      props[j]);

		    if (actual_type == XA_INTEGER && actual_format == 8) {
			int k;
    
			printf("\t%s:\n", XGetAtomName (dpy, props[j]));
			for (k = 0; k < nitems; k++) {
			    if (k % 16 == 0)
				printf ("\t\t");
			    printf("%02x", (unsigned char)prop[k]);
			    if (k % 16 == 15)
				printf("\n");
			}
		    } else if (actual_type == XA_INTEGER &&
			       actual_format == 32)
		    {
			printf("\t%s: %d (0x%08x)",
			       XGetAtomName (dpy, props[j]),
			       *(INT32 *)prop, *(INT32 *)prop);

 			if (propinfo->range && propinfo->num_values > 0) {
			    printf(" range%s: ",
				   (propinfo->num_values == 2) ? "" : "s");

			    for (k = 0; k < propinfo->num_values / 2; k++)
				printf(" (%d,%d)", propinfo->values[k * 2],
				       propinfo->values[k * 2 + 1]);
			}

			printf("\n");
		    } else if (actual_type == XA_ATOM &&
			       actual_format == 32)
		    {
			printf("\t%s: %s",
			       XGetAtomName (dpy, props[j]),
			       XGetAtomName (dpy, *(Atom *)prop));

 			if (!propinfo->range && propinfo->num_values > 0) {
			    printf("\n\t\tsupported:");

			    for (k = 0; k < propinfo->num_values; k++)
			    {
				printf(" %-12.12s", XGetAtomName (dpy,
							    propinfo->values[k]));
				if (k % 4 == 3 && k < propinfo->num_values - 1)
				    printf ("\n\t\t          ");
			    }
			}
			printf("\n");
			
		    } else if (actual_format == 8) {
			printf ("\t\t%s: %s%s\n", XGetAtomName (dpy, props[j]),
				prop, bytes_after ? "..." : "");
		    } else {
			printf ("\t\t%s: ????\n", XGetAtomName (dpy, props[j]));
		    }

		    free(propinfo);
		}
	       printf("  </output>\n");
	    }
	    
	    if (verbose)
	    {
		for (j = 0; j < output_info->nmode; j++)
		{
		    XRRModeInfo	*mode = find_mode_by_xid (output_info->modes[j]);
		    int		f;
		    
		    printf ("  %s (0x%x) %6.1fMHz",
			    mode->name, mode->id,
			    (float)mode->dotClock / 1000000.0);
		    for (f = 0; mode_flags[f].flag; f++)
			if (mode->modeFlags & mode_flags[f].flag)
			    printf (" %s", mode_flags[f].string);
		    printf ("\n");
		    printf ("        h: width  %4d start %4d end %4d total %4d skew %4d clock %6.1fKHz\n",
			    mode->width, mode->hSyncStart, mode->hSyncEnd,
			    mode->hTotal, mode->hSkew, mode_hsync (mode) / 1000);
		    printf ("        v: height %4d start %4d end %4d total %4d           clock %6.1fHz\n",
			    mode->height, mode->vSyncStart, mode->vSyncEnd, mode->vTotal,
			    mode_refresh (mode));
		    mode->modeFlags |= ModeShown;
		}
	    }
	    else
	    {
		mode_shown = calloc (output_info->nmode, sizeof (Bool));
		if (!mode_shown) fatal ("out of memory\n");
		for (j = 0; j < output_info->nmode; j++)
		{
		    XRRModeInfo *jmode, *kmode;
		    
		    if (mode_shown[j]) continue;
    
		    jmode = find_mode_by_xid (output_info->modes[j]);
		    for (k = j; k < output_info->nmode; k++)
		    {
			if (mode_shown[k]) continue;
			kmode = find_mode_by_xid (output_info->modes[k]);
			if (strcmp (jmode->name, kmode->name) != 0) continue;
			mode_shown[k] = True;
			kmode->modeFlags |= ModeShown;
			printf ("    <mode id=\"0x%x\" name=\"%s\" w=\"%d\" h=\"%d\" hz=\"%.1f\"", kmode->id, kmode->name, kmode->width, kmode->height, mode_refresh (kmode));
			if (kmode == output->mode_info)
			    printf (" current=\"true\"");
			else
			    printf (" current=\"false\"");
			if (k < output_info->npreferred)
			    printf (" preferred=\"true\"");
			else
			    printf (" preferred=\"false\"");
                        printf("/>\n");
		    }
		}
		free (mode_shown);
	    }

	    printf("  </output>\n");
	}

/* 
        printf("  <unused>\n");
	for (m = 0; m < res->nmode; m++)
	{
	    XRRModeInfo	*mode = &res->modes[m];

	    if (!(mode->modeFlags & ModeShown))
	    {
		printf ("    <modeline name=\"%s\" id=\"%x\" mhz=\"%.1f\"",
			mode->name, mode->id,
			(float)mode->dotClock / 1000000.0);
		printf (" hwidth=\"%d\" hstart=\"%d\" hend=\"%d\" htotal=\"%d\" hskew=\"%d=\" hclock=\"%.1fKHz\"",
			mode->width, mode->hSyncStart, mode->hSyncEnd,
			mode->hTotal, mode->hSkew, mode_hsync (mode) / 1000);
		printf (" vheight=\"%d\" vstart=\"%d\" vend=\"%d\" vtotal=\"%d\" vclock=\"%.1fHz\"/>\n",
			mode->height, mode->vSyncStart, mode->vSyncEnd, mode->vTotal,
			mode_refresh (mode));
	    }
	}
        printf("  </unused>\n");
*/
        printf("</screen>\n");
	exit (0);
    }
#endif
    
    sc = XRRGetScreenInfo (dpy, root);

    if (sc == NULL) 
	exit (1);

    current_size = XRRConfigCurrentConfiguration (sc, &current_rotation);

    sizes = XRRConfigSizes(sc, &nsize);

    if (have_pixel_size) {
	for (size = 0; size < nsize; size++)
	{
	    if (sizes[size].width == width && sizes[size].height == height)
		break;
	}
	if (size >= nsize) {
	    fprintf (stderr,
		     "Size %dx%d not found in available modes\n", width, height);
	    exit (1);
	}
    }
    else if (size < 0)
	size = current_size;
    else if (size >= nsize) {
	fprintf (stderr,
		 "Size index %d is too large, there are only %d sizes\n",
		 size, nsize);
	exit (1);
    }

    if (rot < 0)
    {
	for (rot = 0; rot < 4; rot++)
	    if (1 << rot == (current_rotation & 0xf))
		break;
    }

    current_rate = XRRConfigCurrentRate (sc);

    if (rate < 0)
    {
	if (size == current_size)
	    rate = current_rate;
	else
	    rate = 0;
    }
    else
    {
	rates = XRRConfigRates (sc, size, &nrate);
	for (i = 0; i < nrate; i++)
	    if (rate == rates[i])
		break;
	if (i == nrate) {
	    fprintf (stderr, "Rate %.1f Hz not available for this size\n", rate);
	    exit (1);
	}
    }

    if (version) {
	int major_version, minor_version;
	XRRQueryVersion (dpy, &major_version, &minor_version);
	printf("Server reports RandR version %d.%d\n", 
	       major_version, minor_version);
    }

    if (query || query_1) {
	printf(" SZ:    Pixels          Physical       Refresh\n");
	for (i = 0; i < nsize; i++) {
	    printf ("%c%-2d %5d x %-5d  (%4dmm x%4dmm )",
		    i == current_size ? '*' : ' ',
		    i, sizes[i].width, sizes[i].height,
		    sizes[i].mwidth, sizes[i].mheight);
	    rates = XRRConfigRates (sc, i, &nrate);
	    if (nrate) printf ("  ");
	    for (j = 0; j < nrate; j++)
		printf ("%c%-4d",
			i == current_size && rates[j] == current_rate ? '*' : ' ',
			rates[j]);
	    printf ("\n");
	}
    }

    rotations = XRRConfigRotations(sc, &current_rotation);

    rotation = 1 << rot ;
    if (query) {
    	printf("Current rotation - %s\n",
	       rotation_name (current_rotation));

	printf("Current reflection - %s\n",
	       reflection_name (current_rotation));

	printf ("Rotations possible - ");
	for (i = 0; i < 4; i ++) {
	    if ((rotations >> i) & 1)  printf("%s ", direction[i]);
	}
	printf ("\n");

	printf ("Reflections possible - ");
	if (rotations & (RR_Reflect_X|RR_Reflect_Y))
	{
	    if (rotations & RR_Reflect_X) printf ("X Axis ");
	    if (rotations & RR_Reflect_Y) printf ("Y Axis");
	}
	else
	    printf ("none");
	printf ("\n");
    }

    if (verbose) { 
	printf("Setting size to %d, rotation to %s\n",  size, direction[rot]);

	printf ("Setting reflection on ");
	if (reflection)
	{
	    if (reflection & RR_Reflect_X) printf ("X Axis ");
	    if (reflection & RR_Reflect_Y) printf ("Y Axis");
	}
	else
	    printf ("neither axis");
	printf ("\n");

	if (reflection & RR_Reflect_X) printf("Setting reflection on X axis\n");

	if (reflection & RR_Reflect_Y) printf("Setting reflection on Y axis\n");
    }

    /* we should test configureNotify on the root window */
    XSelectInput (dpy, root, StructureNotifyMask);

    if (setit && !dryrun) XRRSelectInput (dpy, root,
			       RRScreenChangeNotifyMask);
    if (setit && !dryrun) status = XRRSetScreenConfigAndRate (dpy, sc,
						   DefaultRootWindow (dpy), 
						   (SizeID) size, (Rotation) (rotation | reflection), rate, CurrentTime);

    XRRQueryExtension(dpy, &event_base, &error_base);

    if (setit && !dryrun && status == RRSetConfigFailed) {
	printf ("Failed to change the screen configuration!\n");
	ret = 1;
    }

    if (verbose && setit && !dryrun && size != current_size) {
	if (status == RRSetConfigSuccess)
	{
	    Bool    seen_screen = False;
	    while (!seen_screen) {
		int spo;
		XNextEvent(dpy, (XEvent *) &event);

		printf ("Event received, type = %d\n", event.type);
		/* update Xlib's knowledge of the event */
		XRRUpdateConfiguration (&event);
		if (event.type == ConfigureNotify)
		    printf("Received ConfigureNotify Event!\n");

		switch (event.type - event_base) {
		case RRScreenChangeNotify:
		    sce = (XRRScreenChangeNotifyEvent *) &event;

		    printf("Got a screen change notify event!\n");
		    printf(" window = %d\n root = %d\n size_index = %d\n rotation %d\n", 
			   (int) sce->window, (int) sce->root, 
			   sce->size_index,  sce->rotation);
		    printf(" timestamp = %ld, config_timestamp = %ld\n",
			   sce->timestamp, sce->config_timestamp);
		    printf(" Rotation = %x\n", sce->rotation);
		    printf(" %d X %d pixels, %d X %d mm\n",
			   sce->width, sce->height, sce->mwidth, sce->mheight);
		    printf("Display width   %d, height   %d\n",
			   DisplayWidth(dpy, screen), DisplayHeight(dpy, screen));
		    printf("Display widthmm %d, heightmm %d\n", 
			   DisplayWidthMM(dpy, screen), DisplayHeightMM(dpy, screen));
		    spo = sce->subpixel_order;
		    if ((spo < 0) || (spo > 5))
			printf ("Unknown subpixel order, value = %d\n", spo);
		    else printf ("new Subpixel rendering model is %s\n", order[spo]);
		    seen_screen = True;
		    break;
		default:
		    if (event.type != ConfigureNotify) 
			printf("unknown event received, type = %d!\n", event.type);
		}
	    }
	}
    }
    XRRFreeScreenConfigInfo(sc);
    return(ret);
}
