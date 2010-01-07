/*
 * function: Header file for aacDECdrop
 *
 * This program is distributed under the GNU General Public License, version 2.
 * A copy of this license is included with this source.
 *
 * Copyright (C) 2002 John Edwards
 */

#ifndef __DECODE_H__
#define __DECODE_H__

#include <stdio.h>

typedef void (*progress_func)(long totalsamples, long samples);
typedef void (*error_func)(char *errormessage);

typedef struct
{
	progress_func progress_update;
	error_func error;
	int decode_mode;
	int output_format;
	int file_type;
	int object_type;
	char *filename;
} aac_dec_opt;


int aac_decode(aac_dec_opt *opt);

/*
 * Put this here for convenience
 */

typedef struct {
	char	TitleFormat[32];
	int	window_x;
	int	window_y;
	int	always_on_top;
	int	logerr;
	int     decode_mode;
	int     outputFormat;
	int     fileType;
	int     object_type;
} SettingsAAC;

/*
 * GLOBALS
 */

extern SettingsAAC iniSettings;


#endif /* __DECODE_H__ */
