/*
 * function: Header file for decthread.c
 *
 * This program is distributed under the GNU General Public License, version 2.
 * A copy of this license is included with this source.
 *
 * Copyright (C) 2002 John Edwards
 */
#ifndef __DECTHREAD_H__
#define __DECTHREAD_H__

void decthread_init(void);
void decthread_addfile(char *file);
void decthread_set_decode_mode(int decode_mode);
void decthread_set_outputFormat(int output_format);
void decthread_set_fileType(int file_type);
void decthread_set_object_type(int object_type);

#endif  /* __DECTHREAD_H__ */
