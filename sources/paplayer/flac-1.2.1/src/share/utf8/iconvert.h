/*
 * Copyright (C) 2001 Edmund Grimley Evans <edmundo@rano.org>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef HAVE_ICONV

/*
 * Convert data from one encoding to another. Return:
 *
 *  -2 : memory allocation failed
 *  -1 : unknown encoding
 *   0 : data was converted exactly
 *   1 : data was converted inexactly
 *   2 : data was invalid (but still converted)
 *
 * We convert in two steps, via UTF-8, as this is the only
 * reliable way of distinguishing between invalid input
 * and valid input which iconv refuses to transliterate.
 * We convert from UTF-8 twice, because we have no way of
 * knowing whether the conversion was exact if iconv returns
 * E2BIG (due to a bug in the specification of iconv).
 * An alternative approach is to assume that the output of
 * iconv is never more than 4 times as long as the input,
 * but I prefer to avoid that assumption if possible.
 */

int iconvert(const char *fromcode, const char *tocode,
	     const char *from, size_t fromlen,
	     char **to, size_t *tolen) ;

#endif /* HAVE_ICONV */
