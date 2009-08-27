/* libxmms-flac - XMMS FLAC input plugin
 * Copyright (C) 2000,2001,2002,2003,2004,2005,2006,2007  Josh Coalson
 * Copyright (C) 2002,2003,2004,2005,2006,2007  Daisuke Shimamura
 *
 * Based on FLAC plugin.c and mpg123 plugin
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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <glib.h>
#include <xmms/plugin.h>
#include <xmms/util.h>
#include <xmms/configfile.h>
#include <xmms/titlestring.h>

#include "FLAC/metadata.h"
#include "plugin_common/tags.h"
#include "charset.h"
#include "configure.h"

/*
 * Function local__extname (filename)
 *
 *    Return pointer within filename to its extenstion, or NULL if
 *    filename has no extension.
 *
 */
static char *local__extname(const char *filename)
{
	char *ext = strrchr(filename, '.');

	if (ext != NULL)
		++ext;

	return ext;
}

static char *local__getstr(char* str)
{
	if (str && strlen(str) > 0)
		return str;
	return NULL;
}

static int local__getnum(char* str)
{
	if (str && strlen(str) > 0)
		return atoi(str);
	return 0;
}

static char *local__getfield(const FLAC__StreamMetadata *tags, const char *name)
{
	if (0 != tags) {
		const char *utf8 = FLAC_plugin__tags_get_tag_utf8(tags, name);
		if (0 != utf8) {
			if(flac_cfg.title.convert_char_set)
				return convert_from_utf8_to_user(utf8);
			else
				return strdup(utf8);
		}
	}

	return 0;
}

static void local__safe_free(char *s)
{
	if (0 != s)
		free(s);
}

/*
 * Function flac_format_song_title (tag, filename)
 *
 *    Create song title according to `tag' and/or `filename' and
 *    return it.  The title must be subsequently freed using g_free().
 *
 */
char *flac_format_song_title(char *filename)
{
	char *ret = NULL;
	TitleInput *input = NULL;
	FLAC__StreamMetadata *tags;
	char *title, *artist, *performer, *album, *date, *tracknumber, *genre, *description;

	FLAC_plugin__tags_get(filename, &tags);

	title       = local__getfield(tags, "TITLE");
	artist      = local__getfield(tags, "ARTIST");
	performer   = local__getfield(tags, "PERFORMER");
	album       = local__getfield(tags, "ALBUM");
	date        = local__getfield(tags, "DATE");
	tracknumber = local__getfield(tags, "TRACKNUMBER");
	genre       = local__getfield(tags, "GENRE");
	description = local__getfield(tags, "DESCRIPTION");

	XMMS_NEW_TITLEINPUT(input);

	input->performer = local__getstr(performer);
	if(!input->performer)
		input->performer = local__getstr(artist);
	input->album_name = local__getstr(album);
	input->track_name = local__getstr(title);
	input->track_number = local__getnum(tracknumber);
	input->year = local__getnum(date);
	input->genre = local__getstr(genre);
	input->comment = local__getstr(description);

	input->file_name = g_basename(filename);
	input->file_path = filename;
	input->file_ext = local__extname(filename);
	ret = xmms_get_titlestring(flac_cfg.title.tag_override ? flac_cfg.title.tag_format : xmms_get_gentitle_format(), input);
	g_free(input);

	if (!ret) {
		/*
		 * Format according to filename.
		 */
		ret = g_strdup(g_basename(filename));
		if (local__extname(ret) != NULL)
			*(local__extname(ret) - 1) = '\0';	/* removes period */
	}

	FLAC_plugin__tags_destroy(&tags);
	local__safe_free(title);
	local__safe_free(artist);
	local__safe_free(performer);
	local__safe_free(album);
	local__safe_free(date);
	local__safe_free(tracknumber);
	local__safe_free(genre);
	local__safe_free(description);
	return ret;
}
