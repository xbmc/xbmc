/* grabbag - Convenience lib for various routines common to several tools
 * Copyright (C) 2002,2003,2004,2005,2006,2007  Josh Coalson
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include "share/grabbag.h"
#include "FLAC/assert.h"
#include <stdlib.h> /* for atoi() */
#include <string.h>

#ifdef _MSC_VER
/* There's no strtoll() in MSVC6 so we just write a specialized one */
static FLAC__int64 local__strtoll(const char *src, char **endptr)
{
	FLAC__bool neg = false;
	FLAC__int64 ret = 0;
	int c;
	FLAC__ASSERT(0 != src);
	if(*src == '-') {
		neg = true;
		src++;
	}
	while(0 != (c = *src)) {
		c -= '0';
		if(c >= 0 && c <= 9)
			ret = (ret * 10) + c;
		else
			break;
		src++;
	}
	if(endptr)
		*endptr = (char*)src;
	return neg? -ret : ret;
}
#endif

FLAC__bool grabbag__seektable_convert_specification_to_template(const char *spec, FLAC__bool only_explicit_placeholders, FLAC__uint64 total_samples_to_encode, unsigned sample_rate, FLAC__StreamMetadata *seektable_template, FLAC__bool *spec_has_real_points)
{
	unsigned i;
	const char *pt;

	FLAC__ASSERT(0 != spec);
	FLAC__ASSERT(0 != seektable_template);
	FLAC__ASSERT(seektable_template->type = FLAC__METADATA_TYPE_SEEKTABLE);

	if(0 != spec_has_real_points)
		*spec_has_real_points = false;

	for(pt = spec, i = 0; pt && *pt; i++) {
		const char *q = strchr(pt, ';');
		FLAC__ASSERT(0 != q);

		if(q > pt) {
			if(0 == strncmp(pt, "X;", 2)) { /* -S X */
				if(!FLAC__metadata_object_seektable_template_append_placeholders(seektable_template, 1))
					return false;
			}
			else if(q[-1] == 'x') { /* -S #x */
				if(total_samples_to_encode > 0) { /* we can only do these if we know the number of samples to encode up front */
					if(0 != spec_has_real_points)
						*spec_has_real_points = true;
					if(!only_explicit_placeholders) {
						const int n = (unsigned)atoi(pt);
						if(n > 0)
							if(!FLAC__metadata_object_seektable_template_append_spaced_points(seektable_template, (unsigned)n, total_samples_to_encode))
								return false;
					}
				}
			}
			else if(q[-1] == 's') { /* -S #s */
				if(total_samples_to_encode > 0) { /* we can only do these if we know the number of samples to encode up front */
					FLAC__ASSERT(sample_rate > 0);
					if(0 != spec_has_real_points)
						*spec_has_real_points = true;
					if(!only_explicit_placeholders) {
						const double sec = atof(pt);
						if(sec > 0.0) {
							unsigned samples = (unsigned)(sec * (double)sample_rate);
							if(samples > 0) {
								/* +1 for the initial point at sample 0 */
								if(!FLAC__metadata_object_seektable_template_append_spaced_points_by_samples(seektable_template, samples, total_samples_to_encode))
									return false;
							}
						}
					}
				}
			}
			else { /* -S # */
				if(0 != spec_has_real_points)
					*spec_has_real_points = true;
				if(!only_explicit_placeholders) {
					char *endptr;
#ifdef _MSC_VER
					const FLAC__int64 n = local__strtoll(pt, &endptr);
#else
					const FLAC__int64 n = (FLAC__int64)strtoll(pt, &endptr, 10);
#endif
					if(
						(n > 0 || (endptr > pt && *endptr == ';')) && /* is a valid number (extra check needed for "0") */
						(total_samples_to_encode == 0 || (FLAC__uint64)n < total_samples_to_encode) /* number is not >= the known total_samples_to_encode */
					)
						if(!FLAC__metadata_object_seektable_template_append_point(seektable_template, (FLAC__uint64)n))
							return false;
				}
			}
		}

		pt = ++q;
	}

	if(!FLAC__metadata_object_seektable_template_sort(seektable_template, /*compact=*/true))
		return false;

	return true;
}
