/* test_libFLAC++ - Unit tester for libFLAC++
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

#include "decoders.h"
#include "encoders.h"
#include "metadata.h"

int main(int argc, char *argv[])
{
	(void)argc, (void)argv;

	if(!test_encoders())
		return 1;

	if(!test_decoders())
		return 1;

	if(!test_metadata())
		return 1;

	return 0;
}
