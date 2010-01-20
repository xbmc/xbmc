/*
    TiMidity++ -- MIDI to WAVE converter and player
    Copyright (C) 1999-2002 Masanao Izumo <mo@goice.co.jp>
    Copyright (C) 1995 Tuukka Toivonen <tt@cgs.fi>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA


   common.h
*/

#ifndef ___COMMON_H_
#define ___COMMON_H_

#include "sysdep.h"
#include "url.h"
#include "mblock.h"

extern char *program_name, current_filename[];
extern const char *note_name[];

typedef struct {
  char *path;
  void *next;
} PathList;

struct timidity_file
{
    URL url;
    char *tmpname;
};

/* Noise modes for open_file */
#define OF_SILENT	0
#define OF_NORMAL	1
#define OF_VERBOSE	2


extern void add_to_pathlist(char *s);
extern void clean_up_pathlist(void);
extern int is_url_prefix(const char *name);
extern struct timidity_file *open_file(char *name,
				       int decompress, int noise_mode);
extern struct timidity_file *open_with_mem(char *mem, int32 memlen,
					   int noise_mode);
extern void close_file(struct timidity_file *tf);
extern void skip(struct timidity_file *tf, size_t len);
extern char *tf_gets(char *buff, int n, struct timidity_file *tf);
#define tf_getc(tf) (url_getc((tf)->url))
extern long tf_read(void *buff, int32 size, int32 nitems,
		    struct timidity_file *tf);
extern long tf_seek(struct timidity_file *tf, long offset, int whence);
extern long tf_tell(struct timidity_file *tf);
extern int int_rand(int n);	/* random [0..n-1] */
extern int check_file_extension(char *filename, char *ext, int decompress);

extern void *safe_malloc(size_t count);
extern void *safe_realloc(void *old_ptr, size_t new_size);
extern void *safe_large_malloc(size_t count);
extern char *safe_strdup(const char *s);
extern void free_ptr_list(void *ptr_list, int count);
extern int string_to_7bit_range(const char *s, int *start, int *end);
extern char **expand_file_archives(char **files, int *nfiles_in_out);
extern void randomize_string_list(char **strlist, int nstr);
extern int pathcmp(const char *path1, const char *path2, int ignore_case);
extern void sort_pathname(char **files, int nfiles);
extern int  load_table(char *file);
extern char *pathsep_strrchr(const char *path);
extern char *pathsep_strchr(const char *path);
extern int str2mID(char *str);


/* code:
 * "EUC"	- Extended Unix Code
 * NULL		- Auto conversion
 * "JIS"	- Japanese Industrial Standard code
 * "SJIS"	- shift-JIS code
 */
extern void code_convert(char *in, char *out, int outsiz,
			 char *in_code, char *out_code);

extern void safe_exit(int status);

extern char *timidity_version;
extern MBlockList tmpbuffer;
extern char *output_text_code;

#endif /* ___COMMON_H_ */
