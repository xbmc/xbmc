/*
    TiMidity++ -- MIDI to WAVE converter and player
    Copyright (C) 1999-2002 Masanao Izumo <mo@goice.co.jp>
    Copyright (C) 1995 Tuukka Toivonen <tt@cgs.fi>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

   controls.c

   */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#include "interface.h"
#include "timidity.h"
#include "controls.h"

#if defined(__MACOS__)
extern ControlMode mac_control_mode;
#define DEFAULT_CONTROL_MODE &mac_control_mode
#elif defined(IA_W32GUI)
extern ControlMode w32gui_control_mode;
#define DEFAULT_CONTROL_MODE &w32gui_control_mode
#elif defined(IA_W32G_SYN)
extern ControlMode winsyn_control_mode;
#define DEFAULT_CONTROL_MODE &winsyn_control_mode
#else
extern ControlMode dumb_control_mode;
#define DEFAULT_CONTROL_MODE &dumb_control_mode
#endif


#ifdef IA_PLUGIN
  extern ControlMode plugin_control_mode;
# ifndef DEFAULT_CONTROL_MODE
#  define DEFAULT_CONTROL_MODE &plugin_control_mode
# endif
#endif

#ifdef IA_MOTIF
  extern ControlMode motif_control_mode;
# ifndef DEFAULT_CONTROL_MODE
#  define DEFAULT_CONTROL_MODE &motif_control_mode
# endif
#endif

#ifdef IA_TCLTK
  extern ControlMode tk_control_mode;
# ifndef DEFAULT_CONTROL_MODE
#  define DEFAULT_CONTROL_MODE &tk_control_mode
# endif
#endif

#ifdef IA_NCURSES
  extern ControlMode ncurses_control_mode;
# ifndef DEFAULT_CONTROL_MODE
#  define DEFAULT_CONTROL_MODE &ncurses_control_mode
# endif
#endif

#ifdef IA_VT100
  extern ControlMode vt100_control_mode;
# ifndef DEFAULT_CONTROL_MODE
#  define DEFAULT_CONTROL_MODE &vt100_control_mode
# endif
#endif

#ifdef IA_SLANG
  extern ControlMode slang_control_mode;
# ifndef DEFAULT_CONTROL_MODE
#  define DEFAULT_CONTROL_MODE &slang_control_mode
# endif
#endif

#ifdef IA_DYNAMIC
  extern ControlMode dynamic_control_mode;
# ifndef DEFAULT_CONTROL_MODE
#  define DEFAULT_CONTROL_MODE &dynamic_control_mode
# endif
#endif /* IA_DYNAMIC */

#ifdef IA_EMACS
  extern ControlMode emacs_control_mode;
# ifndef DEFAULT_CONTROL_MODE
#  define DEFAULT_CONTROL_MODE &emacs_control_mode
# endif
#endif /* IA_EMACS */

#ifdef IA_XAW
  extern ControlMode xaw_control_mode;
# ifndef DEFAULT_CONTROL_MODE
#  define DEFAULT_CONTROL_MODE &xaw_control_mode
# endif
#endif /* IA_XAW */

#ifdef IA_XSKIN
  extern ControlMode xskin_control_mode;
# ifndef DEFAULT_CONTROL_MODE
#  define DEFAULT_CONTROL_MODE &xskin_control_mode
# endif
#endif /* IA_XSKIN */

#ifdef IA_KMIDI
  extern ControlMode kmidi_control_mode;
# ifndef DEFAULT_CONTROL_MODE
#  define DEFAULT_CONTROL_MODE &kmidi_control_mode
# endif
#endif /* IA_KMIDI */

#ifdef IA_GTK
  extern ControlMode gtk_control_mode;
# ifndef DEFAULT_CONTROL_MODE
#  define DEFAULT_CONTROL_MODE &gtk_control_mode
# endif
#endif

#ifdef IA_PLUGIN
  extern ControlMode plugin_control_mode;
# ifndef DEFAULT_CONTROL_MODE
#  define DEFAULT_CONTROL_MODE &plugin_control_mode
# endif
#endif

#ifdef IA_SERVER
extern ControlMode server_control_mode;
#endif /* IA_SERVER */

#ifdef IA_ALSASEQ
extern ControlMode alsaseq_control_mode;
#endif /* IA_ALSASEQ */

#ifdef IA_WINSYN
extern ControlMode winsyn_control_mode;
#endif /* IA_WINSYN */

#ifdef IA_PORTMIDISYN
extern ControlMode portmidisyn_control_mode;
#endif /* IA_PORTMIDISYN */

#ifdef IA_MACOSX
extern ControlMode macosx_control_mode;
#endif /* IA_MACOSX */

/* Minimal control mode */
extern ControlMode dumb_control_mode;
#ifndef DEFAULT_CONTROL_MODE
# define DEFAULT_CONTROL_MODE &dumb_control_mode
#endif

ControlMode *ctl_list[]={
#ifdef IA_NCURSES
  &ncurses_control_mode,
#endif
#ifdef IA_VT100
  &vt100_control_mode,
#endif
#ifdef IA_SLANG
  &slang_control_mode,
#endif
#ifdef IA_MOTIF
  &motif_control_mode,
#endif
#ifdef IA_TCLTK
  &tk_control_mode,
#endif
#ifdef IA_EMACS
  &emacs_control_mode,
#endif
#ifdef IA_XAW
  &xaw_control_mode,
#endif
#ifdef IA_XSKIN
  &xskin_control_mode,
#endif
#ifdef IA_KMIDI
  &kmidi_control_mode
#endif
#ifdef IA_GTK
  &gtk_control_mode,
#endif
#ifdef IA_PLUGIN
  &plugin_control_mode,
#endif
#ifdef __MACOS__
  &mac_control_mode,
#endif
#ifdef IA_MACOSX
  &macosx_control_mode,
#endif /* IA_MACOSX */
#ifdef IA_W32GUI
  &w32gui_control_mode,
#endif /* IA_W32GUI */
#ifdef IA_W32G_SYN
  &winsyn_control_mode,
#endif /* IA_W32GUI */
#ifndef __MACOS__
  &dumb_control_mode,
#endif
#ifdef IA_DYNAMIC
  &dynamic_control_mode,
#endif
#ifdef IA_PLUGIN
  &plugin_control_mode,
#endif
#ifdef IA_SERVER
  &server_control_mode,
#endif /* IA_SERVER */
#ifdef IA_ALSASEQ
  &alsaseq_control_mode,
#endif
#ifdef IA_WINSYN
  &winsyn_control_mode,
#endif
#ifdef IA_PORTMIDISYN
  &portmidisyn_control_mode,
#endif
  0
};

ControlMode *ctl=DEFAULT_CONTROL_MODE;
