/* Copyright (C) 2001 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */


#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <limits.h>

#include "mbchar.h"

#if IS_BASIC_ASCII

/* Bit table of characters in the ISO C "basic character set".  */
unsigned int is_basic_table [UCHAR_MAX / 32 + 1] =
{
  0x00001a00,		/* '\t' '\v' '\f' */
  0xffffffef,		/* ' '...'#' '%'...'?' */
  0xfffffffe,		/* 'A'...'Z' '[' '\\' ']' '^' '_' */
  0x7ffffffe		/* 'a'...'z' '{' '|' '}' '~' */
  /* The remaining bits are 0.  */
};

#endif /* IS_BASIC_ASCII */
