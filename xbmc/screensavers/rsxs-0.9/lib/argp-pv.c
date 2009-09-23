/* Default definition for ARGP_PROGRAM_VERSION.
   Copyright (C) 1996, 1997, 1999, 2006 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Written by Miles Bader <miles@gnu.ai.mit.edu>.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

/* If set by the user program to a non-zero value, then a default option
   --version is added (unless the ARGP_NO_HELP flag is used), which will
   print this string followed by a newline and exit (unless the
   ARGP_NO_EXIT flag is used).  Overridden by ARGP_PROGRAM_VERSION_HOOK.  */
const char *argp_program_version;
