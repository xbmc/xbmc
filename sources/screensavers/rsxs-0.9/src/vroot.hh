/*
 * Really Slick XScreenSavers
 * Copyright (C) 2002-2006  Michael Chapman
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *****************************************************************************
 *
 * This is a Linux port of the Really Slick Screensavers,
 * Copyright (C) 2002 Terence M. Welsh, available from www.reallyslick.com
 */

/*
 * Changed declaration of i to unsigned int so g++ doesn't give a warning.
 * -- Michael Chapman, 17 Nov 2002
 * Filename and wrapper #ifndef update.
 * -- Michael Chapman, 22 Jun 2003
 *
 * -- Michael Chapman, 17 Nov 2002
 */

/*****************************************************************************/
/**                   Copyright 1991 by Andreas Stolcke                     **/
/**               Copyright 1990 by Solbourne Computer Inc.                 **/
/**                          Longmont, Colorado                             **/
/**                                                                         **/
/**                           All Rights Reserved                           **/
/**                                                                         **/
/**    Permission to use, copy, modify, and distribute this software and    **/
/**    its documentation  for  any  purpose  and  without  fee is hereby    **/
/**    granted, provided that the above copyright notice appear  in  all    **/
/**    copies and that both  that  copyright  notice  and  this  permis-    **/
/**    sion  notice appear in supporting  documentation,  and  that  the    **/
/**    name of Solbourne not be used in advertising                         **/
/**    in publicity pertaining to distribution of the  software  without    **/
/**    specific, written prior permission.                                  **/
/**                                                                         **/
/**    ANDREAS STOLCKE AND SOLBOURNE COMPUTER INC. DISCLAIMS ALL WARRANTIES **/
/**    WITH REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF    **/
/**    MERCHANTABILITY  AND  FITNESS,  IN  NO  EVENT SHALL ANDREAS STOLCKE  **/
/**    OR SOLBOURNE BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL    **/
/**    DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA   **/
/**    OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER    **/
/**    TORTIOUS ACTION, ARISING OUT OF OR IN  CONNECTION  WITH  THE  USE    **/
/**    OR PERFORMANCE OF THIS SOFTWARE.                                     **/
/*****************************************************************************/
/*
 * vroot.h -- Virtual Root Window handling header file
 *
 * This header file redefines the X11 macros RootWindow and DefaultRootWindow,
 * making them look for a virtual root window as provided by certain `virtual'
 * window managers like swm and tvtwm. If none is found, the ordinary root
 * window is returned, thus retaining backward compatibility with standard
 * window managers.
 * The function implementing the virtual root lookup remembers the result of
 * its last invocation to avoid overhead in the case of repeated calls
 * on the same display and screen arguments.
 * The lookup code itself is taken from Tom LaStrange's ssetroot program.
 *
 * Most simple root window changing X programs can be converted to using
 * virtual roots by just including
 *
 * #include <X11/vroot.h>
 *
 * after all the X11 header files.  It has been tested on such popular
 * X clients as xphoon, xfroot, xloadimage, and xaqua.
 * It also works with the core clients xprop, xwininfo, xwd, and editres
 * (and is necessary to get those clients working under tvtwm).
 * It does NOT work with xsetroot; get the xsetroot replacement included in
 * the tvtwm distribution instead.
 *
 * Andreas Stolcke <stolcke@ICSI.Berkeley.EDU>, 9/7/90
 * - replaced all NULL's with properly cast 0's, 5/6/91
 * - free children list (suggested by Mark Martin <mmm@cetia.fr>), 5/16/91
 * - include X11/Xlib.h and support RootWindowOfScreen, too 9/17/91
 */

#ifndef _VROOT_HH
#define _VROOT_HH

#if !defined(lint) && !defined(SABER)
static const char vroot_rcsid[] =
		"#Id: vroot.h,v 1.4 1991/09/30 19:23:16 stolcke Exp stolcke #";
#endif

#include <X11/X.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>

static Window
#ifdef __STDC__ /* ANSIfication added by jwz, to avoid superfluous warnings. */
VirtualRootWindowOfScreen(Screen *screen)
#else /* !__STDC__ */
VirtualRootWindowOfScreen(screen) Screen *screen;
#endif /* !__STDC__ */
{
	static Screen *save_screen = (Screen *)0;
	static Window root = (Window)0;

	if (screen != save_screen) {
		Display *dpy = DisplayOfScreen(screen);
		Atom __SWM_VROOT = None;
		unsigned int i;
		Window rootReturn, parentReturn, *children;
		unsigned int numChildren;

		root = RootWindowOfScreen(screen);

		/* go look for a virtual root */
		__SWM_VROOT = XInternAtom(dpy, "__SWM_VROOT", False);
		if (XQueryTree(dpy, root, &rootReturn, &parentReturn,
				 &children, &numChildren)) {
			for (i = 0; i < numChildren; i++) {
				Atom actual_type;
				int actual_format;
				unsigned long nitems, bytesafter;
				unsigned char *newRoot = (unsigned char *)0;

				if (XGetWindowProperty(dpy, children[i],
					__SWM_VROOT, 0, 1, False, XA_WINDOW,
					&actual_type, &actual_format,
					&nitems, &bytesafter, &newRoot) == Success
				    && newRoot) {
				    root = *(Window *)newRoot;
				    break;
				}
			}
			if (children)
				XFree((char *)children);
		}

		save_screen = screen;
	}

	return root;
}

#undef RootWindowOfScreen
#define RootWindowOfScreen(s) VirtualRootWindowOfScreen(s)

#undef RootWindow
#define RootWindow(dpy,screen) \
		VirtualRootWindowOfScreen(ScreenOfDisplay(dpy,screen))

#undef DefaultRootWindow
#define DefaultRootWindow(dpy) \
		VirtualRootWindowOfScreen(DefaultScreenOfDisplay(dpy))

#endif /* _VROOT_HH */
