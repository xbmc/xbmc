/* Masanao Izumo <mo@goice.co.jp>:
 *
 * There is problem if both -lXm and -lXaw are linked.
 * This source code to re-define XAW vendorShell.
 * To change motif vendorShell to XAW vendorShell in runtime.
 * 
 * #define vendorShellClassRec xaw_vendorShellClassRec
 * #define vendorShellWidgetClass xaw_vendorShellWidgetClass
 * #include "xaw_redef.c"
 * #undef vendorShellClassRec
 * #undef vendorShellWidgetClass
 * extern WidgetClass vendorShellWidgetClass;
 * extern VendorShellClassRec vendorShellClassRec;
 * static void xaw_vendor_setup(void)
 * {
 *     memcpy(&vendorShellClassRec, &xaw_vendorShellClassRec,
 * 	   sizeof(VendorShellClassRec));
 *     vendorShellWidgetClass = (WidgetClass)&xaw_vendorShellWidgetClass;
 * }
 */

/* $XConsortium: Vendor.c,v 1.27 94/04/17 20:13:25 kaleb Exp $ */

/***********************************************************

Copyright (c) 1987, 1988, 1994  X Consortium

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of the X Consortium shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from the X Consortium.


Copyright 1987, 1988 by Digital Equipment Corporation, Maynard, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/

/*
 * This is a copy of Xt/Vendor.c with an additional ClassInitialize
 * procedure to register Xmu resource type converters, and all the
 * monkey business associated with input methods...
 *
 */

/* Make sure all wm properties can make it out of the resource manager */

#include <stdio.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/ShellP.h>
#include <X11/VendorP.h>
#include <X11/Xmu/Converters.h>
#include <X11/Xmu/Atoms.h>
#include <X11/Xmu/Editres.h>


#ifdef HAVE_XMUREGISTEREXTERNALAGENT
#ifdef HAVE_X11_XMU_EXTAGENT_H
#include <X11/Xmu/ExtAgent.h>
#else
/* There is no ExtAgent.h in openwin package, but XmuRegisterExternalAgent
 * is surely defined in libXmu on Solaris 7.
 */
extern void XmuRegisterExternalAgent(
#if NeedFunctionPrototypes
    Widget /* w */,
    XtPointer /* data */,
    XEvent* /* event */,
    Boolean* /* cont */
#endif
);
#endif /* HAVE_X11_XMU_EXTAGENT_H */
#endif /* HAVE_XMUREGISTEREXTERNALAGENT */


/* The following two headers are for the input method. */

/* $XConsortium: VendorEP.h,v 1.2 94/04/17 20:13:25 kaleb Exp $ */

/*
 * Copyright 1991 by OMRON Corporation
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of OMRON not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  OMRON makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * OMRON DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
 * ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
 * OMRON BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
 * ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTUOUS ACTION,
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE. 
 *
 *	Author:	Seiji Kuwari	OMRON Corporation
 *				kuwa@omron.co.jp
 *				kuwa%omron.co.jp@uunet.uu.net
 */				

/*

Copyright (c) 1994  X Consortium

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of the X Consortium shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from the X Consortium.

*/

#ifndef _VendorEP_h
#define _VendorEP_h

/* $XConsortium: XawImP.h,v 1.4 95/06/06 20:50:30 kaleb Exp $ */

/*
 * Copyright 1991 by OMRON Corporation
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of OMRON not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  OMRON makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * OMRON DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
 * ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
 * OMRON BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
 * ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTUOUS ACTION,
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE. 
 *
 *	Author:	Seiji Kuwari	OMRON Corporation
 *				kuwa@omron.co.jp
 *				kuwa%omron.co.jp@uunet.uu.net
 */				

/*

Copyright (c) 1994  X Consortium

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of the X Consortium shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from the X Consortium.

*/

#ifndef _XawImP_h
#define _XawImP_h

#define XtNinputMethod		"inputMethod"
#define XtCInputMethod		"InputMethod"
#define XtNpreeditType		"preeditType"
#define XtCPreeditType		"PreeditType"
#define XtNopenIm		"openIm"
#define XtCOpenIm		"OpenIm"
#define XtNsharedIc		"sharedIc"
#define XtCSharedIc		"SharedIc"

#include <X11/Xaw/Text.h>

#define	CIICFocus	(1 << 0)
#define	CIFontSet	(1 << 1)
#define	CIFg		(1 << 2)
#define	CIBg		(1 << 3)
#define	CIBgPixmap	(1 << 4)
#define	CICursorP	(1 << 5)
#define	CILineS		(1 << 6)

typedef	struct _XawImPart
{
    XIM			xim;
    XrmResourceList	resources;
    Cardinal		num_resources;
    Boolean		open_im;
    Boolean		initialized;
    Dimension		area_height;
    String		input_method;
    String		preedit_type;
} XawImPart;

typedef struct _XawIcTablePart
{
    Widget		widget;
    XIC			xic;
    XIMStyle		input_style;
    unsigned long	flg;
    unsigned long	prev_flg;
    Boolean		ic_focused;
    XFontSet		font_set;
    Pixel		foreground;
    Pixel		background;
    Pixmap		bg_pixmap;
    XawTextPosition	cursor_position;
    unsigned long	line_spacing;
    Boolean		openic_error;
    struct _XawIcTablePart *next;
} XawIcTablePart, *XawIcTableList;

typedef	struct _XawIcPart
{
    XIMStyle		input_style;
    Boolean		shared_ic;
    XawIcTableList	shared_ic_table;
    XawIcTableList	current_ic_table;
    XawIcTableList	ic_table;
} XawIcPart;

typedef	struct _contextDataRec
{
    Widget		parent;
    Widget		ve;
} contextDataRec;

typedef	struct _contextErrDataRec
{
    Widget		widget;
    XIM			xim;
} contextErrDataRec;

void _XawImResizeVendorShell( 
#if NeedFunctionPrototypes
    Widget /* w */
#endif
);

Dimension _XawImGetShellHeight( 
#if NeedFunctionPrototypes
    Widget /* w */
#endif
);

void _XawImRealize( 
#if NeedFunctionPrototypes
    Widget /* w */
#endif
);

void _XawImInitialize( 
#if NeedFunctionPrototypes
    Widget, /* w */
    Widget  /* ext */
#endif
);

void _XawImReconnect( 
#if NeedFunctionPrototypes
    Widget  /* w */
#endif
);

void _XawImRegister( 
#if NeedFunctionPrototypes
    Widget  /* w */
#endif
);

void _XawImUnregister( 
#if NeedFunctionPrototypes
    Widget  /* w */
#endif
);

void _XawImSetValues( 
#if NeedFunctionPrototypes
    Widget,  /* w */
    ArgList, /* args */
    Cardinal /* num_args */
#endif
);

/* DON'T USE THIS FUNCTION -- it's going away in the next release */
void _XawImVASetValues( 
#if NeedVarargsPrototypes
    Widget,  /* w */
    ... 
#endif
);

void _XawImSetFocusValues( 
#if NeedFunctionPrototypes
    Widget,  /* w */
    ArgList, /* args */
    Cardinal /* num_args */
#endif
);

/* DON'T USE THIS FUNCTION -- it's going away in the next release */
void _XawImVASetFocusValues( 
#if NeedVarargsPrototypes
    Widget,  /* w */
    ... 
#endif
);

void _XawImUnsetFocus( 
#if NeedFunctionPrototypes
    Widget  /* w */
#endif
);

int  _XawImWcLookupString( 
#if NeedFunctionPrototypes
    Widget,   /* w */
    XKeyPressedEvent*, /* event */
    wchar_t*, /* buffer_return */
    int,      /* bytes_buffer */
    KeySym*,  /* keysym_return */
    Status*   /* status return */
#endif
);

int  _XawImGetImAreaHeight( 
#if NeedFunctionPrototypes
    Widget  /* w */
#endif
);

void _XawImCallVendorShellExtResize( 
#if NeedFunctionPrototypes
    Widget  /* w */
#endif
);

void _XawImDestroy( 
#if NeedFunctionPrototypes
    Widget,  /* w */
    Widget   /* ext */
#endif
);

#endif	/* _XawImP_h */


typedef struct {
    XtPointer	extension;
} XawVendorShellExtClassPart;

typedef	struct _VendorShellExtClassRec {
    ObjectClassPart	object_class;
    XawVendorShellExtClassPart	vendor_shell_ext_class;
} XawVendorShellExtClassRec;

typedef struct {
    Widget	parent;
    XawImPart	im;
    XawIcPart	ic;
} XawVendorShellExtPart;

typedef	struct XawVendorShellExtRec {
    ObjectPart	object;
    XawVendorShellExtPart	vendor_ext;
} XawVendorShellExtRec, *XawVendorShellExtWidget;

#endif	/* _VendorEP_h */



static XtResource resources[] = {
  {XtNinput, XtCInput, XtRBool, sizeof(Bool),
		XtOffsetOf(VendorShellRec, wm.wm_hints.input),
		XtRImmediate, (XtPointer)True}
};

/***************************************************************************
 *
 * Vendor shell class record
 *
 ***************************************************************************/

static void XawVendorShellClassInitialize();
static void XawVendorShellClassPartInit();
static void XawVendorShellInitialize();
static Boolean XawVendorShellSetValues();
static void Realize(), ChangeManaged();
static XtGeometryResult GeometryManager();
void XawVendorShellExtResize();

static CompositeClassExtensionRec vendorCompositeExt = {
    /* next_extension     */	NULL,
    /* record_type        */    NULLQUARK,
    /* version            */    XtCompositeExtensionVersion,
    /* record_size        */    sizeof (CompositeClassExtensionRec),
    /* accepts_objects    */    TRUE,
    /* allows_change_managed_set */ FALSE
};

#define SuperClass (&wmShellClassRec)
externaldef(vendorshellclassrec) VendorShellClassRec vendorShellClassRec = {
  {
    /* superclass	  */	(WidgetClass)SuperClass,
    /* class_name	  */	"VendorShell",
    /* size		  */	sizeof(VendorShellRec),
    /* class_initialize	  */	XawVendorShellClassInitialize,
    /* class_part_init	  */	XawVendorShellClassPartInit,
    /* Class init'ed ?	  */	FALSE,
    /* initialize         */	XawVendorShellInitialize,
    /* initialize_hook	  */	NULL,		
    /* realize		  */	Realize,
    /* actions		  */	NULL,
    /* num_actions	  */	0,
    /* resources	  */	resources,
    /* resource_count	  */	XtNumber(resources),
    /* xrm_class	  */	NULLQUARK,
    /* compress_motion	  */	FALSE,
    /* compress_exposure  */	TRUE,
    /* compress_enterleave*/	FALSE,
    /* visible_interest	  */	FALSE,
    /* destroy		  */	NULL,
    /* resize		  */	XawVendorShellExtResize,
    /* expose		  */	NULL,
    /* set_values	  */	XawVendorShellSetValues,
    /* set_values_hook	  */	NULL,			
    /* set_values_almost  */	XtInheritSetValuesAlmost,  
    /* get_values_hook	  */	NULL,
    /* accept_focus	  */	NULL,
    /* intrinsics version */	XtVersion,
    /* callback offsets	  */	NULL,
    /* tm_table		  */	NULL,
    /* query_geometry	  */	NULL,
    /* display_accelerator*/	NULL,
    /* extension	  */	NULL
  },{
    /* geometry_manager	  */	GeometryManager,
    /* change_managed	  */	ChangeManaged,
    /* insert_child	  */	XtInheritInsertChild,
    /* delete_child	  */	XtInheritDeleteChild,
    /* extension	  */	(XtPointer) &vendorCompositeExt
  },{
    /* extension	  */	NULL
  },{
    /* extension	  */	NULL
  },{
    /* extension	  */	NULL
  }
};

externaldef(vendorshellwidgetclass) WidgetClass vendorShellWidgetClass =
	(WidgetClass) (&vendorShellClassRec);


/***************************************************************************
 *
 * The following section is for the Vendor shell Extension class record
 *
 ***************************************************************************/

static XtResource ext_resources[] = {
  {XtNinputMethod, XtCInputMethod, XtRString, sizeof(String),
		XtOffsetOf(XawVendorShellExtRec, vendor_ext.im.input_method),
		XtRString, (XtPointer)NULL},
  {XtNpreeditType, XtCPreeditType, XtRString, sizeof(String),
		XtOffsetOf(XawVendorShellExtRec, vendor_ext.im.preedit_type),
		XtRString, (XtPointer)"OverTheSpot,OffTheSpot,Root"},
  {XtNopenIm, XtCOpenIm, XtRBoolean, sizeof(Boolean),
		XtOffsetOf(XawVendorShellExtRec, vendor_ext.im.open_im),
		XtRImmediate, (XtPointer)TRUE},
  {XtNsharedIc, XtCSharedIc, XtRBoolean, sizeof(Boolean),
		XtOffsetOf(XawVendorShellExtRec, vendor_ext.ic.shared_ic),
		XtRImmediate, (XtPointer)FALSE}
};

static void XawVendorShellExtClassInitialize();
static void XawVendorShellExtInitialize();
static void XawVendorShellExtDestroy();
static Boolean XawVendorShellExtSetValues();

externaldef(vendorshellextclassrec) XawVendorShellExtClassRec
       xawvendorShellExtClassRec = {
  {
    /* superclass	  */	(WidgetClass)&objectClassRec,
    /* class_name	  */	"VendorShellExt",
    /* size		  */	sizeof(XawVendorShellExtRec),
    /* class_initialize	  */	XawVendorShellExtClassInitialize,
    /* class_part_initialize*/	NULL,
    /* Class init'ed ?	  */	FALSE,
    /* initialize	  */	XawVendorShellExtInitialize,
    /* initialize_hook	  */	NULL,		
    /* pad		  */	NULL,
    /* pad		  */	NULL,
    /* pad		  */	0,
    /* resources	  */	ext_resources,
    /* resource_count	  */	XtNumber(ext_resources),
    /* xrm_class	  */	NULLQUARK,
    /* pad		  */	FALSE,
    /* pad		  */	FALSE,
    /* pad		  */	FALSE,
    /* pad		  */	FALSE,
    /* destroy		  */	XawVendorShellExtDestroy,
    /* pad		  */	NULL,
    /* pad		  */	NULL,
    /* set_values	  */	XawVendorShellExtSetValues,
    /* set_values_hook	  */	NULL,			
    /* pad		  */	NULL,  
    /* get_values_hook	  */	NULL,
    /* pad		  */	NULL,
    /* version		  */	XtVersion,
    /* callback_offsets	  */	NULL,
    /* pad		  */	NULL,
    /* pad		  */	NULL,
    /* pad		  */	NULL,
    /* extension	  */	NULL
  },{
    /* extension	  */	NULL
  }
};

externaldef(xawvendorshellwidgetclass) WidgetClass
     xawvendorShellExtWidgetClass = (WidgetClass) (&xawvendorShellExtClassRec);


/*ARGSUSED*/
static Boolean
XawCvtCompoundTextToString(dpy, args, num_args, fromVal, toVal, cvt_data)
Display *dpy;
XrmValuePtr args;
Cardinal    *num_args;
XrmValue *fromVal;
XrmValue *toVal;
XtPointer *cvt_data;
{
    XTextProperty prop;
    char **list;
    int count;
    static char *mbs = NULL;
    int len;

    prop.value = (unsigned char *)fromVal->addr;
    prop.encoding = XA_COMPOUND_TEXT(dpy);
    prop.format = 8;
    prop.nitems = fromVal->size;

    if(XmbTextPropertyToTextList(dpy, &prop, &list, &count) < Success) {
	XtAppWarningMsg(XtDisplayToApplicationContext(dpy),
	"converter", "XmbTextPropertyToTextList", "XawError",
	"conversion from CT to MB failed.", NULL, 0);
	return False;
    }
    len = strlen(*list);
    toVal->size = len;
    mbs = XtRealloc(mbs, len + 1); /* keep buffer because no one call free :( */
    strcpy(mbs, *list);
    XFreeStringList(list);
    toVal->addr = (XtPointer)mbs;
    return True;
}

static void XawVendorShellClassInitialize()
{
    static XtConvertArgRec screenConvertArg[] = {
        {XtWidgetBaseOffset, (XtPointer) XtOffsetOf(WidgetRec, core.screen),
	     sizeof(Screen *)}
    };

    XtAddConverter(XtRString, XtRCursor, XmuCvtStringToCursor,      
		   screenConvertArg, XtNumber(screenConvertArg));

    XtAddConverter(XtRString, XtRBitmap, XmuCvtStringToBitmap,
		   screenConvertArg, XtNumber(screenConvertArg));

    XtSetTypeConverter("CompoundText", XtRString, XawCvtCompoundTextToString,
			NULL, 0, XtCacheNone, NULL);
}

static void XawVendorShellClassPartInit(class)
    WidgetClass class;
{
    CompositeClassExtension ext;
    VendorShellWidgetClass vsclass = (VendorShellWidgetClass) class;

    if ((ext = (CompositeClassExtension) 
	    XtGetClassExtension (class,
				 XtOffsetOf(CompositeClassRec, 
					    composite_class.extension),
				 NULLQUARK, 1L, (Cardinal) 0)) == NULL) {
	ext = (CompositeClassExtension) XtNew (CompositeClassExtensionRec);
	if (ext != NULL) {
	    ext->next_extension = vsclass->composite_class.extension;
	    ext->record_type = NULLQUARK;
	    ext->version = XtCompositeExtensionVersion;
	    ext->record_size = sizeof (CompositeClassExtensionRec);
	    ext->accepts_objects = TRUE;
	    ext->allows_change_managed_set = FALSE;
	    vsclass->composite_class.extension = (XtPointer) ext;
	}
    }
}

#ifdef __osf__
/* stupid OSF/1 shared libraries have the wrong semantics */
/* symbols do not get resolved external to the shared library */
void _XawFixupVendorShell()
{
    transientShellWidgetClass->core_class.superclass =
        (WidgetClass) &vendorShellClassRec;
    topLevelShellWidgetClass->core_class.superclass =
        (WidgetClass) &vendorShellClassRec;
}
#endif

/* ARGSUSED */
static void XawVendorShellInitialize(req, new, args, num_args)
	Widget req, new;
	ArgList     args;
	Cardinal    *num_args;
{
    XtAddEventHandler(new, (EventMask) 0, TRUE, _XEditResCheckMessages, NULL);
#ifdef HAVE_XMUREGISTEREXTERNALAGENT
    XtAddEventHandler(new, (EventMask) 0, TRUE, XmuRegisterExternalAgent, NULL);
#endif /* HAVE_XMUREGISTEREXTERNALAGENT */

    XtCreateWidget("shellext", xawvendorShellExtWidgetClass,
		   new, args, *num_args);
}

/* ARGSUSED */
static Boolean XawVendorShellSetValues(old, ref, new)
	Widget old, ref, new;
{
	return FALSE;
}

static void Realize(wid, vmask, attr)
	Widget wid;
	Mask *vmask;
	XSetWindowAttributes *attr;
{
	WidgetClass super = wmShellWidgetClass;

	/* Make my superclass do all the dirty work */

	(*super->core_class.realize) (wid, vmask, attr);
	_XawImRealize(wid);
}


static void XawVendorShellExtClassInitialize()
{
}

/* ARGSUSED */
static void XawVendorShellExtInitialize(req, new)
        Widget req, new;
{
    _XawImInitialize(new->core.parent, new);
}

/* ARGSUSED */
static void XawVendorShellExtDestroy( w )
        Widget w;
{
    _XawImDestroy( w->core.parent, w );
}

/* ARGSUSED */
static Boolean XawVendorShellExtSetValues(old, ref, new)
	Widget old, ref, new;
{
	return FALSE;
}

void XawVendorShellExtResize( w )
    Widget w;
{
	ShellWidget sw = (ShellWidget) w;
	Widget childwid;
	int i;
	int core_height;

	_XawImResizeVendorShell( w );
	core_height = _XawImGetShellHeight( w );
	for( i = 0; i < sw->composite.num_children; i++ ) {
	    if( XtIsManaged( sw->composite.children[ i ] ) ) {
		childwid = sw->composite.children[ i ];
		XtResizeWidget( childwid, sw->core.width, core_height,
			       childwid->core.border_width );
	    }
	}
}

/*ARGSUSED*/
static XtGeometryResult GeometryManager( wid, request, reply )
	Widget wid;
	XtWidgetGeometry *request;
	XtWidgetGeometry *reply;
{
	ShellWidget shell = (ShellWidget)(wid->core.parent);
	XtWidgetGeometry my_request;

	if(shell->shell.allow_shell_resize == FALSE && XtIsRealized(wid))
		return(XtGeometryNo);

	if (request->request_mode & (CWX | CWY))
	    return(XtGeometryNo);

	/* %%% worry about XtCWQueryOnly */
	my_request.request_mode = 0;
	if (request->request_mode & CWWidth) {
	    my_request.width = request->width;
	    my_request.request_mode |= CWWidth;
	}
	if (request->request_mode & CWHeight) {
	    my_request.height = request->height
			      + _XawImGetImAreaHeight( wid );
	    my_request.request_mode |= CWHeight;
	}
	if (request->request_mode & CWBorderWidth) {
	    my_request.border_width = request->border_width;
	    my_request.request_mode |= CWBorderWidth;
	}
	if (XtMakeGeometryRequest((Widget)shell, &my_request, NULL)
		== XtGeometryYes) {
	    /* assert: if (request->request_mode & CWWidth) then
	     * 		  shell->core.width == request->width
	     * assert: if (request->request_mode & CWHeight) then
	     * 		  shell->core.height == request->height
	     *
	     * so, whatever the WM sized us to (if the Shell requested
	     * only one of the two) is now the correct child size
	     */
	    
	    wid->core.width = shell->core.width;
	    wid->core.height = shell->core.height;
	    if (request->request_mode & CWBorderWidth) {
		wid->core.x = wid->core.y = -request->border_width;
	    }
	    _XawImCallVendorShellExtResize(wid);
	    return XtGeometryYes;
	} else return XtGeometryNo;
}

static void ChangeManaged(wid)
	Widget wid;
{
	ShellWidget w = (ShellWidget) wid;
	Widget* childP;
	int i;

	(*SuperClass->composite_class.change_managed)(wid);
	for (i = w->composite.num_children, childP = w->composite.children;
	     i; i--, childP++) {
	    if (XtIsManaged(*childP)) {
		XtSetKeyboardFocus(wid, *childP);
		break;
	    }
	}
}
