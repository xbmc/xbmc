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

#include "Xsel.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/time.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>

#define debug_property(level, requestor, property, target, length)
#define print_debug(x,y...) 
#define MAXLINE 1024

Xsel::Xsel(Display *_display):
  display(_display)
{
  Window root = XDefaultRootWindow (display);
  
  /* Create an unmapped window for receiving events */
  int black = BlackPixel (display, DefaultScreen (display));
  window = XCreateSimpleWindow (display, root, 0, 0, 1, 1, 0, black, black);

  /* Get a timestamp */
  XSelectInput (display, window, PropertyChangeMask);
  timestamp = get_timestamp ();

  /* Get the DELETE atom */
  delete_atom = XInternAtom (display, "DELETE", False);

  /* Get the INCR atom */
  incr_atom = XInternAtom (display, "INCR", False);

  /* Get the UTF8_STRING atom */
  utf8_atom = XInternAtom (display, "UTF8_STRING", True);

  /* Get the NULL atom */
  null_atom = XInternAtom (display, "NULL", False);

  /* Get the COMPOUND_TEXT atom.
   * NB. We do not currently serve COMPOUND_TEXT; we can retrieve it but
   * do not perform charset conversion.
   */
  compound_text_atom = XInternAtom (display, "COMPOUND_TEXT", False);
}

Xsel::~Xsel()
{
  XDestroyWindow (display, window);
}

Xsel::operator std::string() const
{
  unsigned char *selectionBuffer = get_selection_text (XA_PRIMARY);
  std::string result = (const char*) selectionBuffer;
  free (selectionBuffer);
  return result;
}

/*
 * exit_err (fmt)
 *
 * Print a formatted error message and errno information to stderr,
 * then exit with return code 1.
 */
static void
exit_err (const char * fmt, ...)
{
  va_list ap;
  int errno_save;
  char buf[MAXLINE];
  int n;

  errno_save = errno;

  va_start (ap, fmt);

  snprintf (buf, MAXLINE, "FATAL ERROR: ");
  n = strlen (buf);

  vsnprintf (buf+n, MAXLINE-n, fmt, ap);
  n = strlen (buf);

  snprintf (buf+n, MAXLINE-n, ": %s\n", strerror (errno_save));

  fflush (stdout); /* in case stdout and stderr are the same */
  fputs (buf, stderr);
  fflush (NULL);

  va_end (ap);
  exit (1);
} 

/*
 * xs_malloc (size)
 *
 * Malloc wrapper. Always returns a successful allocation. Exits if the
 * allocation didn't succeed.
 */
static void *
xs_malloc (size_t size)
{
  void * ret;

  if (size == 0) size = 1;
  if ((ret = malloc (size)) == NULL) {
    exit_err ("malloc error");
  }

  return ret;
}

/*
 * xs_strdup (s)
 *
 * strdup wrapper for unsigned char *
 */
#define xs_strdup(s) ((unsigned char *) _xs_strdup ((const char *)s))
static char * _xs_strdup (const char * s)
{
  char * ret;

  if (s == NULL) return NULL;
  if ((ret = strdup(s)) == NULL) {
    exit_err ("strdup error");
  }

  return ret; 
}

/*
 * xs_strlen (s)
 *
 * strlen wrapper for unsigned char *
 */
#define xs_strlen(s) (strlen ((const char *) s))

/*
 * xs_strncpy (s)
 *
 * strncpy wrapper for unsigned char *
 */

#define xs_strncpy(dest,s,n) (_xs_strncpy ((char *)dest, (const char *)s, n))
static char *
_xs_strncpy (char * dest, const char * src, size_t n)
{
  if (n > 0) {
    strncpy (dest, src, n);
    dest[n-1] = '\0';
  }
  return dest;
}

/*
 * get_timestamp ()
 *
 * Get the current X server time.
 *
 * This is done by doing a zero-length append to a random property of the
 * window, and checking the time on the subsequent PropertyNotify event.
 *
 * PRECONDITION: the window must have PropertyChangeMask set.
 */

Time Xsel::get_timestamp (void) const
{
  XEvent event;

  XChangeProperty (display, window, XA_WM_NAME, XA_STRING, 8,
                   PropModeAppend, NULL, 0);

  while (1) {
    XNextEvent (display, &event);

    if (event.type == PropertyNotify)
      return event.xproperty.time;
  }
}


/*
 * SELECTION RETRIEVAL
 * ===================
 *
 * The following functions implement retrieval of an X selection.
 * 
 */

/*
 * get_append_property ()
 *
 * Get a window property and append its data to a buffer at a given offset
 * pointed to by *offset. 'offset' is modified by this routine to point to
 * the end of the data.
 *
 * Returns True if more data is available for receipt.
 *
 * If an error is encountered, the buffer is free'd.
 */
Bool Xsel::get_append_property (XSelectionEvent * xsl, unsigned char ** buffer,
                     unsigned long * offset, unsigned long * alloc) const
{
  unsigned char * ptr;
  Atom target;
  int format;
  unsigned long bytesafter, length;
  unsigned char * value;

  XGetWindowProperty (xsl->display, xsl->requestor, xsl->property,
                      0L, 1000000, True, (Atom)AnyPropertyType,
                      &target, &format, &length, &bytesafter, &value);

  debug_property (D_TRACE, xsl->requestor, xsl->property, target, length);

  if (target != XA_STRING && target != utf8_atom) {
    print_debug (D_OBSC, "target %s not XA_STRING nor UTF8_STRING in get_append_property()",
                 get_atom_name (target));
    free (*buffer);
    *buffer = NULL;
    return False;
  } else if (length == 0) {
    /* A length of 0 indicates the end of the transfer */
    print_debug (D_TRACE, "Got zero length property; end of INCR transfer");
    return False;
  } else if (format == 8) {
    if (*offset + length > *alloc) {
      *alloc = *offset + length;
      if ((*buffer = (unsigned char*) realloc (*buffer, *alloc)) == NULL) {
        exit_err ("realloc error");
      }
    }
    ptr = *buffer + *offset;
    xs_strncpy (ptr, value, length);
    *offset += length;
    print_debug (D_TRACE, "Appended %d bytes to buffer\n", length);
  } else {
    print_debug (D_WARN, "Retrieved non-8-bit data\n");
  }

  return True;
}


/*
 * wait_incr_selection (selection)
 *
 * Retrieve a property of target type INCR. Perform incremental retrieval
 * and return the resulting data.
 */
unsigned char * Xsel::wait_incr_selection (Atom selection, XSelectionEvent * xsl, int init_alloc) const
{
  XEvent event;
  unsigned char * incr_base = NULL;
  unsigned long incr_alloc = 0, incr_xfer = 0;
  Bool wait_prop = True;

  print_debug (D_TRACE, "Initialising incremental retrieval of at least %d bytes\n", init_alloc);

  /* Take an interest in the requestor */
  XSelectInput (xsl->display, xsl->requestor, PropertyChangeMask);

  incr_alloc = init_alloc;
  incr_base = (unsigned char*) xs_malloc (incr_alloc);

  print_debug (D_TRACE, "Deleting property that informed of INCR transfer");
  XDeleteProperty (xsl->display, xsl->requestor, xsl->property);

  print_debug (D_TRACE, "Waiting on PropertyNotify events");
  while (wait_prop) {
    XNextEvent (xsl->display, &event);

    switch (event.type) {
    case PropertyNotify:
      if (event.xproperty.state != PropertyNewValue) break;

      wait_prop = get_append_property (xsl, &incr_base, &incr_xfer,
                                       &incr_alloc);
      break;
    default:
      break;
    }
  }

  /* when zero length found, finish up & delete last */
  XDeleteProperty (xsl->display, xsl->requestor, xsl->property);

  print_debug (D_TRACE, "Finished INCR retrieval");

  return incr_base;
}

/*
 * wait_selection (selection, request_target)
 *
 * Block until we receive a SelectionNotify event, and return its
 * contents; or NULL in the case of a deletion or error. This assumes we
 * have already called XConvertSelection, requesting a string (explicitly
 * XA_STRING) or deletion (delete_atom).
 */
unsigned char * Xsel::wait_selection (Atom selection, Atom request_target) const
{
  XEvent event;
  Atom target;
  int format;
  unsigned long bytesafter, length;
  unsigned char * value, * retval = NULL;
  Bool keep_waiting = True;

  while (keep_waiting) {
    XNextEvent (display, &event);

    switch (event.type) {
    case SelectionNotify:
      if (event.xselection.selection != selection) break;

      if (event.xselection.property == None) {
        print_debug (D_WARN, "Conversion refused");
        value = NULL;
        keep_waiting = False;
      } else if (event.xselection.property == null_atom &&
                 request_target == delete_atom) {
      } else {
	XGetWindowProperty (event.xselection.display,
			    event.xselection.requestor,
			    event.xselection.property, 0L, 1000000,
			    False, (Atom)AnyPropertyType, &target,
			    &format, &length, &bytesafter, &value);

        debug_property (D_TRACE, event.xselection.requestor,
                        event.xselection.property, target, length);

        if (request_target == delete_atom && value == NULL) {
          keep_waiting = False;
        } else if (target == incr_atom) {
          /* Handle INCR transfers */
          retval = wait_incr_selection (selection, &event.xselection,
                                        *(int *)value);
          keep_waiting = False;
        } else if (target != utf8_atom && target != XA_STRING &&
                   target != compound_text_atom &&
                   request_target != delete_atom) {
          /* Report non-TEXT atoms */
          print_debug (D_WARN, "Selection (type %s) is not a string.",
                       get_atom_name (target));
          free (retval);
          retval = NULL;
          keep_waiting = False;
        } else {
          retval = xs_strdup (value);
          XFree (value);
          keep_waiting = False;
        }

        XDeleteProperty (event.xselection.display,
                         event.xselection.requestor,
                         event.xselection.property);

      }
      break;
    default:
      break;
    }
  }

  return retval;
}

/*
 * get_selection (selection, request_target)
 *
 * Retrieves the specified selection and returns its value.
 *
 */
unsigned char *Xsel::get_selection (Atom selection, Atom request_target) const
{
  Atom prop;
  unsigned char * retval;

  prop = XInternAtom (display, "XSEL_DATA", False);
  XConvertSelection (display, selection, request_target, prop, window,
                     timestamp);
  XSync (display, False);

  retval = wait_selection (selection, request_target);

  return retval;
}

/*
 * get_selection_text (Atom selection)
 *
 * Retrieve a text selection. First attempt to retrieve it as UTF_STRING,
 * and if that fails attempt to retrieve it as a plain XA_STRING.
 *
 * NB. Before implementing this, an attempt was made to query TARGETS and
 * request UTF8_STRING only if listed there, as described in:
 * http://www.pps.jussieu.fr/~jch/software/UTF8_STRING/UTF8_STRING.text
 * However, that did not seem to work reliably when tested against various
 * applications (eg. Mozilla Firefox). This method is of course more
 * reliable.
 */
unsigned char *Xsel::get_selection_text (Atom selection) const
{
  unsigned char * retval;

  if ((retval = get_selection (selection, utf8_atom)) == NULL)
    retval = get_selection (selection, XA_STRING);

  return retval;
}

