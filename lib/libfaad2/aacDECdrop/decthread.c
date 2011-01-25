/*
 * function: Decoding thread for aacDECdrop
 *
 * This program is distributed under the GNU General Public License, version 2.
 * A copy of this license is included with this source.
 *
 * Copyright (C) 2002 John Edwards
 *
 * last mod: aacDecdrop decoder last updated 2002-03-14
 */

#include <windows.h>
#include <time.h>
#include <string.h>

#include "wave_out.h"
#include "decode.h"
#include "misc.h"

extern int decoding_done;
extern int animate;
extern double file_complete;
extern int totalfiles;
extern int numfiles;
int dec_mode;
int outputFormat;
int fileType;
int object_type;
extern char* fileName;
int stop_decoding;

typedef struct enclist_tag {
	char *filename;
	struct enclist_tag *next;
} enclist_t;

enclist_t *head = NULL;

CRITICAL_SECTION mutex;

DWORD WINAPI decode_thread(LPVOID arg);

void decthread_init(void)
{
	int thread_id;
	HANDLE thand;

	numfiles = 0;
	totalfiles = 0;
	file_complete = 0.0;

	InitializeCriticalSection(&mutex);

	thand = CreateThread(NULL, 0, decode_thread, NULL, 0, &thread_id);
	if (thand == NULL) {
		// something bad happened, might want to deal with that, maybe...
	}
}

void decthread_addfile(char *file)
{
	char *filename;
	enclist_t *entry, *node;

	if (file == NULL) return;

	// create entry
	filename = strdup(file);
	entry = (enclist_t *)malloc(sizeof(enclist_t));

	entry->filename = filename;
	entry->next = NULL;

	EnterCriticalSection(&mutex);

	// insert entry
	if (head == NULL) {
		head = entry;
	} else {
		node = head;
		while (node->next != NULL)
			node = node->next;

		node->next = entry;
	}
	numfiles++;
	totalfiles++;

	LeaveCriticalSection(&mutex);
}

/*
 * the caller is responsible for deleting the pointer
 */

char *_getfile()
{
	char *filename;
	enclist_t *entry;

	EnterCriticalSection(&mutex);

	if (head == NULL) {
		LeaveCriticalSection(&mutex);
		return NULL;
	}

	// pop entry
	entry = head;
	head = head->next;

	filename = entry->filename;
	free(entry);

	LeaveCriticalSection(&mutex);

	return filename;
}

void decthread_set_decode_mode(int decode_mode)
{
	dec_mode = decode_mode;
}

void decthread_set_outputFormat(int output_format)
{
	outputFormat = output_format;
}

void decthread_set_fileType(int file_type)
{
	fileType = file_type;
}

void decthread_set_object_type(int object_type)
{
	object_type = object_type;
}

void _error(char *errormessage)
{
	// do nothing
}

void _update(long total, long done)
{
	file_complete = (double)done / (double)total;
}

DWORD WINAPI decode_thread(LPVOID arg)
{
	char *in_file;

	while (!decoding_done)
	{
		while (in_file = _getfile())
		{
			aac_dec_opt      dec_opts;
			animate = 1;

			if(stop_decoding){
				numfiles--;
				break;
			}
			set_filename(in_file);

			dec_opts.progress_update = _update;
			dec_opts.filename = in_file;
			dec_opts.decode_mode = dec_mode;
			dec_opts.output_format = outputFormat;
			dec_opts.file_type = fileType;
			dec_opts.object_type = object_type;
			fileName = in_file;

			aac_decode(&dec_opts);

			numfiles--;
		} /* Finished this file, loop around to next... */

		file_complete = 0.0;
		animate = 0;
		totalfiles = 0;
		numfiles = 0;

		Sleep(500);
	} 

	DeleteCriticalSection(&mutex);

	return 0;
}

/******************************** end of decthread.c ********************************/

