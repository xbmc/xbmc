/*
  $Id: report.c,v 1.2 2005/01/06 01:15:51 rocky Exp $

  Copyright (C) 2004 Rocky Bernstein <rocky@panix.com>
  Copyright (C) 1998 Monty xiphmont@mit.edu
  
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
*/
/******************************************************************
 * 
 * reporting/logging routines
 *
 ******************************************************************/

#include <stdio.h>
#include "config.h"
#include <cdio/cdda.h>
#include "report.h"

int quiet=0;
int verbose=CDDA_MESSAGE_FORGETIT;

void 
report(const char *s)
{
  if (!quiet) {
    fprintf(stderr,s);
    fputc('\n',stderr);
  }
}

void 
report2(const char *s, char *s2)
{
  if (!quiet) {
    fprintf(stderr,s,s2);
    fputc('\n',stderr);
  }
}

void 
report3(const char *s, char *s2, char *s3)
{
  if (!quiet) {
    fprintf(stderr,s,s2,s3);
    fputc('\n',stderr);
  }
}
