/* Default definition for ARGP_PROGRAM_BUG_ADDRESS.
   Copyright (C) 1996-1997, 1999, 2009-2011 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Written by Miles Bader <miles@gnu.ai.mit.edu>.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* If set by the user program, it should point to string that is the
   bug-reporting address for the program.  It will be printed by argp_help if
   the ARGP_HELP_BUG_ADDR flag is set (as it is by various standard help
   messages), embedded in a sentence that says something like `Report bugs to
   ADDR.'.  */
const char *argp_program_bug_address
/* This variable should be zero-initialized.  On most systems, putting it into
   BSS is sufficient.  Not so on MacOS X 10.3 and 10.4, see
   <http://lists.gnu.org/archive/html/bug-gnulib/2009-01/msg00329.html>
   <http://lists.gnu.org/archive/html/bug-gnulib/2009-08/msg00096.html>.  */
#if defined __ELF__
  /* On ELF systems, variables in BSS behave well.  */
#else
  = (const char *) 0
#endif
  ;
