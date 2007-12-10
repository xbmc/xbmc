/* test_libFLAC - Unit tester for libFLAC
 * Copyright (C) 2004,2005,2006,2007  Josh Coalson
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

#include "FLAC/assert.h"
#include "FLAC/format.h"
#include "format.h"
#include <stdio.h>

static const char *true_false_string_[2] = { "false", "true" };

static struct {
	unsigned rate;
	FLAC__bool valid;
	FLAC__bool subset;
} SAMPLE_RATES[] = {
	{ 0      , false, false },
	{ 1      , true , true  },
	{ 9      , true , true  },
	{ 10     , true , true  },
	{ 4000   , true , true  },
	{ 8000   , true , true  },
	{ 11025  , true , true  },
	{ 12000  , true , true  },
	{ 16000  , true , true  },
	{ 22050  , true , true  },
	{ 24000  , true , true  },
	{ 32000  , true , true  },
	{ 32768  , true , true  },
	{ 44100  , true , true  },
	{ 48000  , true , true  },
	{ 65000  , true , true  },
	{ 65535  , true , true  },
	{ 65536  , true , false },
	{ 65540  , true , true  },
	{ 65550  , true , true  },
	{ 65555  , true , false },
	{ 66000  , true , true  },
	{ 66001  , true , false },
	{ 96000  , true , true  },
	{ 100000 , true , true  },
	{ 100001 , true , false },
	{ 192000 , true , true  },
	{ 500000 , true , true  },
	{ 500001 , true , false },
	{ 500010 , true , true  },
	{ 655349 , true , false },
	{ 655350 , true , true  },
	{ 655351 , false, false },
	{ 655360 , false, false },
	{ 700000 , false, false },
	{ 700010 , false, false },
	{ 1000000, false, false },
	{ 1100000, false, false }
};

static struct {
	const char *string;
	FLAC__bool valid;
} VCENTRY_NAMES[] = {
	{ ""    , true  },
	{ "a"   , true  },
	{ "="   , false },
	{ "a="  , false },
	{ "\x01", false },
	{ "\x1f", false },
	{ "\x7d", true  },
	{ "\x7e", false },
	{ "\xff", false }
};

static struct {
	unsigned length;
	const FLAC__byte *string;
	FLAC__bool valid;
} VCENTRY_VALUES[] = {
	{ 0, (const FLAC__byte*)""            , true  },
	{ 1, (const FLAC__byte*)""            , true  },
	{ 1, (const FLAC__byte*)"\x01"        , true  },
	{ 1, (const FLAC__byte*)"\x7f"        , true  },
	{ 1, (const FLAC__byte*)"\x80"        , false },
	{ 1, (const FLAC__byte*)"\x81"        , false },
	{ 1, (const FLAC__byte*)"\xc0"        , false },
	{ 1, (const FLAC__byte*)"\xe0"        , false },
	{ 1, (const FLAC__byte*)"\xf0"        , false },
	{ 2, (const FLAC__byte*)"\xc0\x41"    , false },
	{ 2, (const FLAC__byte*)"\xc1\x41"    , false },
	{ 2, (const FLAC__byte*)"\xc0\x85"    , false }, /* non-shortest form */
	{ 2, (const FLAC__byte*)"\xc1\x85"    , false }, /* non-shortest form */
	{ 2, (const FLAC__byte*)"\xc2\x85"    , true  },
	{ 2, (const FLAC__byte*)"\xe0\x41"    , false },
	{ 2, (const FLAC__byte*)"\xe1\x41"    , false },
	{ 2, (const FLAC__byte*)"\xe0\x85"    , false },
	{ 2, (const FLAC__byte*)"\xe1\x85"    , false },
	{ 3, (const FLAC__byte*)"\xe0\x85\x41", false },
	{ 3, (const FLAC__byte*)"\xe1\x85\x41", false },
	{ 3, (const FLAC__byte*)"\xe0\x85\x80", false }, /* non-shortest form */
	{ 3, (const FLAC__byte*)"\xe0\x95\x80", false }, /* non-shortest form */
	{ 3, (const FLAC__byte*)"\xe0\xa5\x80", true  },
	{ 3, (const FLAC__byte*)"\xe1\x85\x80", true  },
	{ 3, (const FLAC__byte*)"\xe1\x95\x80", true  },
	{ 3, (const FLAC__byte*)"\xe1\xa5\x80", true  }
};

static struct {
	const FLAC__byte *string;
	FLAC__bool valid;
} VCENTRY_VALUES_NT[] = {
	{ (const FLAC__byte*)""            , true  },
	{ (const FLAC__byte*)"\x01"        , true  },
	{ (const FLAC__byte*)"\x7f"        , true  },
	{ (const FLAC__byte*)"\x80"        , false },
	{ (const FLAC__byte*)"\x81"        , false },
	{ (const FLAC__byte*)"\xc0"        , false },
	{ (const FLAC__byte*)"\xe0"        , false },
	{ (const FLAC__byte*)"\xf0"        , false },
	{ (const FLAC__byte*)"\xc0\x41"    , false },
	{ (const FLAC__byte*)"\xc1\x41"    , false },
	{ (const FLAC__byte*)"\xc0\x85"    , false }, /* non-shortest form */
	{ (const FLAC__byte*)"\xc1\x85"    , false }, /* non-shortest form */
	{ (const FLAC__byte*)"\xc2\x85"    , true  },
	{ (const FLAC__byte*)"\xe0\x41"    , false },
	{ (const FLAC__byte*)"\xe1\x41"    , false },
	{ (const FLAC__byte*)"\xe0\x85"    , false },
	{ (const FLAC__byte*)"\xe1\x85"    , false },
	{ (const FLAC__byte*)"\xe0\x85\x41", false },
	{ (const FLAC__byte*)"\xe1\x85\x41", false },
	{ (const FLAC__byte*)"\xe0\x85\x80", false }, /* non-shortest form */
	{ (const FLAC__byte*)"\xe0\x95\x80", false }, /* non-shortest form */
	{ (const FLAC__byte*)"\xe0\xa5\x80", true  },
	{ (const FLAC__byte*)"\xe1\x85\x80", true  },
	{ (const FLAC__byte*)"\xe1\x95\x80", true  },
	{ (const FLAC__byte*)"\xe1\xa5\x80", true  }
};

static struct {
	unsigned length;
	const FLAC__byte *string;
	FLAC__bool valid;
} VCENTRIES[] = {
	{ 0, (const FLAC__byte*)""              , false },
	{ 1, (const FLAC__byte*)"a"             , false },
	{ 1, (const FLAC__byte*)"="             , true  },
	{ 2, (const FLAC__byte*)"a="            , true  },
	{ 2, (const FLAC__byte*)"\x01="         , false },
	{ 2, (const FLAC__byte*)"\x1f="         , false },
	{ 2, (const FLAC__byte*)"\x7d="         , true  },
	{ 2, (const FLAC__byte*)"\x7e="         , false },
	{ 2, (const FLAC__byte*)"\xff="         , false },
	{ 3, (const FLAC__byte*)"a=\x01"        , true  },
	{ 3, (const FLAC__byte*)"a=\x7f"        , true  },
	{ 3, (const FLAC__byte*)"a=\x80"        , false },
	{ 3, (const FLAC__byte*)"a=\x81"        , false },
	{ 3, (const FLAC__byte*)"a=\xc0"        , false },
	{ 3, (const FLAC__byte*)"a=\xe0"        , false },
	{ 3, (const FLAC__byte*)"a=\xf0"        , false },
	{ 4, (const FLAC__byte*)"a=\xc0\x41"    , false },
	{ 4, (const FLAC__byte*)"a=\xc1\x41"    , false },
	{ 4, (const FLAC__byte*)"a=\xc0\x85"    , false }, /* non-shortest form */
	{ 4, (const FLAC__byte*)"a=\xc1\x85"    , false }, /* non-shortest form */
	{ 4, (const FLAC__byte*)"a=\xc2\x85"    , true  },
	{ 4, (const FLAC__byte*)"a=\xe0\x41"    , false },
	{ 4, (const FLAC__byte*)"a=\xe1\x41"    , false },
	{ 4, (const FLAC__byte*)"a=\xe0\x85"    , false },
	{ 4, (const FLAC__byte*)"a=\xe1\x85"    , false },
	{ 5, (const FLAC__byte*)"a=\xe0\x85\x41", false },
	{ 5, (const FLAC__byte*)"a=\xe1\x85\x41", false },
	{ 5, (const FLAC__byte*)"a=\xe0\x85\x80", false }, /* non-shortest form */
	{ 5, (const FLAC__byte*)"a=\xe0\x95\x80", false }, /* non-shortest form */
	{ 5, (const FLAC__byte*)"a=\xe0\xa5\x80", true  },
	{ 5, (const FLAC__byte*)"a=\xe1\x85\x80", true  },
	{ 5, (const FLAC__byte*)"a=\xe1\x95\x80", true  },
	{ 5, (const FLAC__byte*)"a=\xe1\xa5\x80", true  }
};

FLAC__bool test_format(void)
{
	unsigned i;

	printf("\n+++ libFLAC unit test: format\n\n");

	for(i = 0; i < sizeof(SAMPLE_RATES)/sizeof(SAMPLE_RATES[0]); i++) {
		printf("testing FLAC__format_sample_rate_is_valid(%u)... ", SAMPLE_RATES[i].rate);
		if(FLAC__format_sample_rate_is_valid(SAMPLE_RATES[i].rate) != SAMPLE_RATES[i].valid) {
			printf("FAILED, expected %s, got %s\n", true_false_string_[SAMPLE_RATES[i].valid], true_false_string_[!SAMPLE_RATES[i].valid]);
			return false;
		}
		printf("OK\n");
	}

	for(i = 0; i < sizeof(SAMPLE_RATES)/sizeof(SAMPLE_RATES[0]); i++) {
		printf("testing FLAC__format_sample_rate_is_subset(%u)... ", SAMPLE_RATES[i].rate);
		if(FLAC__format_sample_rate_is_subset(SAMPLE_RATES[i].rate) != SAMPLE_RATES[i].subset) {
			printf("FAILED, expected %s, got %s\n", true_false_string_[SAMPLE_RATES[i].subset], true_false_string_[!SAMPLE_RATES[i].subset]);
			return false;
		}
		printf("OK\n");
	}

	for(i = 0; i < sizeof(VCENTRY_NAMES)/sizeof(VCENTRY_NAMES[0]); i++) {
		printf("testing FLAC__format_vorbiscomment_entry_name_is_legal(\"%s\")... ", VCENTRY_NAMES[i].string);
		if(FLAC__format_vorbiscomment_entry_name_is_legal(VCENTRY_NAMES[i].string) != VCENTRY_NAMES[i].valid) {
			printf("FAILED, expected %s, got %s\n", true_false_string_[VCENTRY_NAMES[i].valid], true_false_string_[!VCENTRY_NAMES[i].valid]);
			return false;
		}
		printf("OK\n");
	}

	for(i = 0; i < sizeof(VCENTRY_VALUES)/sizeof(VCENTRY_VALUES[0]); i++) {
		printf("testing FLAC__format_vorbiscomment_entry_value_is_legal(\"%s\", %u)... ", VCENTRY_VALUES[i].string, VCENTRY_VALUES[i].length);
		if(FLAC__format_vorbiscomment_entry_value_is_legal(VCENTRY_VALUES[i].string, VCENTRY_VALUES[i].length) != VCENTRY_VALUES[i].valid) {
			printf("FAILED, expected %s, got %s\n", true_false_string_[VCENTRY_VALUES[i].valid], true_false_string_[!VCENTRY_VALUES[i].valid]);
			return false;
		}
		printf("OK\n");
	}

	for(i = 0; i < sizeof(VCENTRY_VALUES_NT)/sizeof(VCENTRY_VALUES_NT[0]); i++) {
		printf("testing FLAC__format_vorbiscomment_entry_value_is_legal(\"%s\", -1)... ", VCENTRY_VALUES_NT[i].string);
		if(FLAC__format_vorbiscomment_entry_value_is_legal(VCENTRY_VALUES_NT[i].string, (unsigned)(-1)) != VCENTRY_VALUES_NT[i].valid) {
			printf("FAILED, expected %s, got %s\n", true_false_string_[VCENTRY_VALUES_NT[i].valid], true_false_string_[!VCENTRY_VALUES_NT[i].valid]);
			return false;
		}
		printf("OK\n");
	}

	for(i = 0; i < sizeof(VCENTRIES)/sizeof(VCENTRIES[0]); i++) {
		printf("testing FLAC__format_vorbiscomment_entry_is_legal(\"%s\", %u)... ", VCENTRIES[i].string, VCENTRIES[i].length);
		if(FLAC__format_vorbiscomment_entry_is_legal(VCENTRIES[i].string, VCENTRIES[i].length) != VCENTRIES[i].valid) {
			printf("FAILED, expected %s, got %s\n", true_false_string_[VCENTRIES[i].valid], true_false_string_[!VCENTRIES[i].valid]);
			return false;
		}
		printf("OK\n");
	}

	printf("\nPASSED!\n");
	return true;
}
