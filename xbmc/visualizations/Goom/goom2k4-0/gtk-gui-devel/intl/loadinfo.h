/* Copyright (C) 1996, 1997 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@cygnus.com>, 1996.

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

#ifndef PARAMS
# if __STDC__
#  define PARAMS(args) args
# else
#  define PARAMS(args) ()
# endif
#endif

/* Encoding of locale name parts.  */
#define CEN_REVISION		1
#define CEN_SPONSOR		2
#define CEN_SPECIAL		4
#define XPG_NORM_CODESET	8
#define XPG_CODESET		16
#define TERRITORY		32
#define CEN_AUDIENCE		64
#define XPG_MODIFIER		128

#define CEN_SPECIFIC	(CEN_REVISION|CEN_SPONSOR|CEN_SPECIAL|CEN_AUDIENCE)
#define XPG_SPECIFIC	(XPG_CODESET|XPG_NORM_CODESET|XPG_MODIFIER)


struct loaded_l10nfile
{
  const char *filename;
  int decided;

  const void *data;

  struct loaded_l10nfile *next;
  struct loaded_l10nfile *successor[1];
};


extern const char *_nl_normalize_codeset PARAMS ((const unsigned char *codeset,
						  size_t name_len));

extern struct loaded_l10nfile *
_nl_make_l10nflist PARAMS ((struct loaded_l10nfile **l10nfile_list,
			    const char *dirlist, size_t dirlist_len, int mask,
			    const char *language, const char *territory,
			    const char *codeset,
			    const char *normalized_codeset,
			    const char *modifier, const char *special,
			    const char *sponsor, const char *revision,
			    const char *filename, int do_allocate));


extern const char *_nl_expand_alias PARAMS ((const char *name));

extern int _nl_explode_name PARAMS ((char *name, const char **language,
				     const char **modifier,
				     const char **territory,
				     const char **codeset,
				     const char **normalized_codeset,
				     const char **special,
				     const char **sponsor,
				     const char **revision));
