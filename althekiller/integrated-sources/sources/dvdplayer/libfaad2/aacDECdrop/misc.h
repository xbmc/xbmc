/*
 * function: Header file for misc.c
 *
 * This program is distributed under the GNU General Public License, version 2.
 * A copy of this license is included with this source.
 *
 * Copyright (C) 2002 John Edwards
 */

#ifndef __MISC_H__
#define __MISC_H__

#include "decode.h"
#include <stdio.h>

void set_filename(char *filename);

extern void error_dialog(const char *fmt, ...);
extern void log_error(const char *fmt, ...);
extern void set_use_dialogs(int use_dialogs);
extern void (*error_handler)(const char *fmt, ...);


#endif /* __MISC_H__ */

