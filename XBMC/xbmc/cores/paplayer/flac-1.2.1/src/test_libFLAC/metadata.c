/* test_libFLAC - Unit tester for libFLAC
 * Copyright (C) 2002,2003,2004,2005,2006,2007  Josh Coalson
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include "metadata.h"
#include <stdio.h>

extern FLAC__bool test_metadata_object(void);
extern FLAC__bool test_metadata_file_manipulation(void);

FLAC__bool test_metadata(void)
{
	if(!test_metadata_object())
		return false;

	if(!test_metadata_file_manipulation())
		return false;

	printf("\nPASSED!\n");

	return true;
}
