/*
 * libasap-xmms.c - ASAP plugin for XMMS
 *
 * Copyright (C) 2006-2008  Piotr Fusik
 *
 * This file is part of ASAP (Another Slight Atari Player),
 * see http://asap.sourceforge.net
 *
 * ASAP is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 *
 * ASAP is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ASAP; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <pthread.h>
#include <string.h>
#ifdef USE_STDIO
#include <stdio.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif

#include <xmms/plugin.h>
#include <xmms/titlestring.h>
#include <xmms/util.h>

#include "asap.h"

#define BITS_PER_SAMPLE  16
#define BUFFERED_BLOCKS  512

static int channels;

static InputPlugin mod;

static byte module[ASAP_MODULE_MAX];
static int module_len;
static ASAP_State asap;

static volatile int seek_to;
static pthread_t thread_handle;
static volatile abool thread_run = FALSE;
static volatile abool generated_eof = FALSE;

static char *asap_stpcpy(char *dest, const char *src)
{
	size_t len = strlen(src);
	memcpy(dest, src, len);
	return dest + len;
}

static void asap_show_message(gchar *title, gchar *text)
{
	xmms_show_message(title, text, "Ok", FALSE, NULL, NULL);
}

static void asap_about(void)
{
	asap_show_message("About ASAP XMMS plugin " ASAP_VERSION,
		ASAP_CREDITS "\n" ASAP_COPYRIGHT);
}

static int asap_is_our_file(char *filename)
{
	return ASAP_IsOurFile(filename);
}

static int asap_load_file(const char *filename)
{
#ifdef USE_STDIO
	FILE *fp;
	fp = fopen(filename, "rb");
	if (fp == NULL)
		return FALSE;
	module_len = (int) fread(module, 1, sizeof(module), fp);
	fclose(fp);
#else
	int fd;
	fd = open(filename, O_RDONLY);
	if (fd == -1)
		return FALSE;
	module_len = read(fd, module, sizeof(module));
	close(fd);
	if (module_len < 0)
		return FALSE;
#endif
	return TRUE;
}

static char *asap_get_title(char *filename, ASAP_ModuleInfo *module_info)
{
	char *path;
	char *filepart;
	char *ext;
	TitleInput *title_input;
	char *title;

	path = g_strdup(filename);
	filepart = strrchr(path, '/');
	if (filepart != NULL) {
		filepart[1] = '\0';
		filepart += 2;
	}
	else
		filepart = path;
	ext = strrchr(filepart, '.');
	if (ext != NULL)
		ext++;

	XMMS_NEW_TITLEINPUT(title_input);
	if (module_info->author[0] != '\0')
		title_input->performer = module_info->author;
	title_input->track_name = module_info->name;
	if (module_info->date[0] != '\0')
		title_input->date = module_info->date;
	title_input->file_name = g_basename(filename);
	title_input->file_ext = ext;
	title_input->file_path = path;
	title = xmms_get_titlestring(xmms_get_gentitle_format(), title_input);
	if (title == NULL)
		title = g_strdup(module_info->name);

	g_free(path);
	return title;
}

static void *asap_play_thread(void *arg)
{
	while (thread_run) {
		static
#if BITS_PER_SAMPLE == 8
			byte
#else
			short
#endif
			buffer[BUFFERED_BLOCKS * 2];
		int buffered_bytes;
		if (generated_eof) {
			xmms_usleep(10000);
			continue;
		}
		if (seek_to >= 0) {
			mod.output->flush(seek_to);
			ASAP_Seek(&asap, seek_to);
			seek_to = -1;
		}
		buffered_bytes = BUFFERED_BLOCKS * channels * (BITS_PER_SAMPLE / 8);
		buffered_bytes = ASAP_Generate(&asap, buffer, buffered_bytes, BITS_PER_SAMPLE);
		if (buffered_bytes == 0) {
			generated_eof = TRUE;
			mod.output->buffer_free();
			mod.output->buffer_free();
			continue;
		}
		mod.add_vis_pcm(mod.output->written_time(),
			BITS_PER_SAMPLE == 8 ? FMT_U8 : FMT_S16_NE,
			channels, buffered_bytes, buffer);
		while (thread_run && mod.output->buffer_free() < buffered_bytes)
			xmms_usleep(20000);
		if (thread_run)
			mod.output->write_audio(buffer, buffered_bytes);
	}
	pthread_exit(NULL);
}

static void asap_play_file(char *filename)
{
	int song;
	int duration;
	char *title;
	if (!asap_load_file(filename))
		return;
	if (!ASAP_Load(&asap, filename, module, module_len))
		return;
	song = asap.module_info.default_song;
	duration = asap.module_info.durations[song];
	ASAP_PlaySong(&asap, song, duration);
	channels = asap.module_info.channels;
	if (!mod.output->open_audio(BITS_PER_SAMPLE == 8 ? FMT_U8 : FMT_S16_NE,
		ASAP_SAMPLE_RATE, channels))
		return;
	title = asap_get_title(filename, &asap.module_info);
	mod.set_info(title, duration, BITS_PER_SAMPLE * 1000, ASAP_SAMPLE_RATE, channels);
	g_free(title);
	seek_to = -1;
	thread_run = TRUE;
	generated_eof = FALSE;
	pthread_create(&thread_handle, NULL, asap_play_thread, NULL);
}

static void asap_seek(int time)
{
	seek_to = time * 1000;
	generated_eof = FALSE;
	while (thread_run && seek_to >= 0)
		xmms_usleep(10000);
}

static void asap_pause(short paused)
{
	mod.output->pause(paused);
}

static void asap_stop(void)
{
	if (thread_run) {
		thread_run = FALSE;
		pthread_join(thread_handle, NULL);
		mod.output->close_audio();
	}
}

static int asap_get_time(void)
{
	if (!thread_run || (generated_eof && !mod.output->buffer_playing()))
		return -1;
	return mod.output->output_time();
}

static void asap_get_song_info(char *filename, char **title, int *length)
{
	ASAP_ModuleInfo module_info;
	if (!asap_load_file(filename))
		return;
	if (!ASAP_GetModuleInfo(&module_info, filename, module, module_len))
		return;
	*title = asap_get_title(filename, &module_info);
	*length = module_info.durations[module_info.default_song];
}

static void asap_file_info_box(char *filename)
{
	ASAP_ModuleInfo module_info;
	char info[512];
	char *p;
	if (!asap_load_file(filename))
		return;
	if (!ASAP_GetModuleInfo(&module_info, filename, module, module_len))
		return;
	p = asap_stpcpy(info, "Author: ");
	p = asap_stpcpy(p, module_info.author);
	p = asap_stpcpy(p, "\nName: ");
	p = asap_stpcpy(p, module_info.name);
	p = asap_stpcpy(p, "\nDate: ");
	p = asap_stpcpy(p, module_info.date);
	*p = '\0';
	asap_show_message("File information", info);
}

static InputPlugin mod = {
	NULL, NULL,
	"ASAP " ASAP_VERSION,
	NULL,
	asap_about,
	NULL,
	asap_is_our_file,
	NULL,
	asap_play_file,
	asap_stop,
	asap_pause,
	asap_seek,
	NULL,
	asap_get_time,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	asap_get_song_info,
	asap_file_info_box,
	NULL
};

InputPlugin *get_iplugin_info(void)
{
	return &mod;
}
