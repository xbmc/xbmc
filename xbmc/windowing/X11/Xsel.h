/*
 * Xsel is a class for obtaining X selection.
 * Ported to C++ by Amir Gonnen, from xsel utility by Conrad Parker.
 * Extracted only the "paste" code, removed timeout support. 
 * 
 * xsel -- manipulate the X selection
 * Copyright (C) 2001 Conrad Parker <conrad@vergenet.net>
 *
 * Permission to use, copy, modify, distribute, and sell this software and
 * its documentation for any purpose is hereby granted without fee, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation.  No representations are made about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 */

#include <string>
#include "X11/Xlib.h"

class Xsel
{
public:
  Xsel(Display*);
  virtual ~Xsel();

  operator std::string() const;

private:

  Time get_timestamp (void) const;
  Bool get_append_property (XSelectionEvent * xsl, unsigned char ** buffer,
		  unsigned long * offset, unsigned long * alloc) const;
  unsigned char * wait_incr_selection (Atom selection, XSelectionEvent * xsl, int init_alloc) const;
  unsigned char * wait_selection (Atom selection, Atom request_target) const;
  unsigned char *get_selection (Atom selection, Atom request_target) const;
  unsigned char *get_selection_text (Atom selection) const;

  /* Our X Display and Window */
  Display * display;
  Window window;

  Atom delete_atom; /* The DELETE atom */
  Atom incr_atom; /* The INCR atom */
  Atom null_atom; /* The NULL atom */
  Atom utf8_atom; /* The UTF8 atom */
  Atom compound_text_atom; /* The COMPOUND_TEXT atom */

  /* Our timestamp for all operations */
  Time timestamp;
};

