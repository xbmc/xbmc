/*
    TiMidity -- Experimental MIDI to WAVE converter
    Copyright (C) 1999-2002 Masanao Izumo <mo@goice.co.jp>
    Copyright (C) 1995 Tuukka Toivonen <toivonen@clinet.fi>

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
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <windowsx.h>
#undef RC_NONE
#include "timidity.h"
#include "common.h"
#include "instrum.h"
#include "playmidi.h"
#include "readmidi.h"
#include "controls.h"
#include "w32g.h"
#include "w32g_res.h"

#define W32G_RANDOM_IS_SHUFFLE

void SetNumListWnd(int cursel, int nfiles);

// playlist
typedef struct _PlayListEntry {
    char *filename;	// malloc
    char *title;	// shared with midi_file_info
    struct midi_file_info *info;
} PlayListEntry;
static struct {
    int nfiles;
    int selected; /* 0..nfiles-1 */
    int allocated; /* number of PlayListEntry is allocated */
    PlayListEntry *list;
} playlist = {0, 0, 0, NULL};

static HWND playlist_box(void)
{
    if(!hListWnd)
	return 0;
    return GetDlgItem(hListWnd, IDC_LISTBOX_PLAYLIST);
}

static int w32g_add_playlist1(char *filename, int uniq, int refine)
{
    PlayListEntry *entry;
    char *title;
    struct midi_file_info *info;

    if(uniq)
    {
	int i;
	for(i = 0; i < playlist.nfiles; i++)
	    if(pathcmp(filename, playlist.list[i].filename, 0) == 0)
		return 0;
    }

    title = get_midi_title(filename);
    info = get_midi_file_info(filename, 1);
    if(refine && info->format < 0)
	return 0;

    if(playlist.allocated == 0)
    {
	playlist.allocated = 32;
	playlist.list = (PlayListEntry *)safe_malloc(playlist.allocated *
						     sizeof(PlayListEntry));
    }
    else if(playlist.nfiles == playlist.allocated)
    {
	playlist.allocated *= 2;
	playlist.list = (PlayListEntry *)safe_realloc(playlist.list,
						      playlist.allocated *
						      sizeof(PlayListEntry));
    }

    entry = &playlist.list[playlist.nfiles];
    entry->filename = safe_strdup(filename);
    entry->title = title;
    entry->info = info;
    playlist.nfiles++;
	w32g_shuffle_playlist_reset(1);
    return 1;
}

int w32g_add_playlist(int nfiles, char **files, int expand_flag,
		      int uniq, int refine)
{
    char **new_files1;
    char **new_files2;
    int i, n;
    extern int SeachDirRecursive;
    extern char **FilesExpandDir(int *, char **);

    if(nfiles == 0)
	return 0;

    if(SeachDirRecursive)
    {
	new_files1 = FilesExpandDir(&nfiles, files);
	if(new_files1 == NULL)
	    return 0;
	expand_flag = 1;
    }
    else
	new_files1 = files;

    if(!expand_flag)
	new_files2 = new_files1;
    else
    {
	new_files2 = expand_file_archives(new_files1, &nfiles);
	if(new_files2 == NULL)
	{
	    if(new_files1 != files)
	    {
		free(new_files1[0]);
		free(new_files1);
	    }
	    return 0;
	}
    }

    n = 0;
    for(i = 0; i < nfiles; i++)
	n += w32g_add_playlist1(new_files2[i], uniq, refine);

    if(new_files2 != new_files1)
    {
	free(new_files2[0]);
	free(new_files2);
    }
    if(new_files1 != files)
    {
	free(new_files1[0]);
	free(new_files1);
    }

    if(n > 0)
	w32g_update_playlist();
    return n;
}

int w32g_next_playlist(int skip_invalid_file)
{
    while(playlist.selected + 1 < playlist.nfiles)
    {
	playlist.selected++;
	if(!skip_invalid_file ||
	   playlist.list[playlist.selected].info->file_type != IS_ERROR_FILE)
	{
	    w32g_update_playlist();
	    return 1;
	}
    }
    return 0;
}

int w32g_prev_playlist(int skip_invalid_file)
{
    while(playlist.selected > 0)
    {
	playlist.selected--;
	if(!skip_invalid_file ||
	   playlist.list[playlist.selected].info->file_type != IS_ERROR_FILE)
	{
	    w32g_update_playlist();
	    return 1;
	}
    }
    return 0;
}

int w32g_random_playlist(int skip_invalid_file)
{
	int old_selected_index = playlist.selected;
	int select;
	int err = 0;
	for(;;) {
		if ( playlist.nfiles == 1) {
			select = old_selected_index;
		} else {
			if ( playlist.nfiles <= 1 )
				select = 0;
			else if ( playlist.nfiles == 2 )
				select = 1;
			else
				select = int_rand(playlist.nfiles - 1);
			select += old_selected_index;
			if ( select >= playlist.nfiles )
				select -= playlist.nfiles;
			if ( select < 0 )
				select = 0;
		}
		playlist.selected = select; 
		if(!skip_invalid_file ||
			playlist.list[playlist.selected].info->file_type != IS_ERROR_FILE) {
			w32g_update_playlist();
			return 1;
		}
		if ( playlist.nfiles == 2 ) {
			playlist.selected = old_selected_index; 
			if(!skip_invalid_file ||
				playlist.list[playlist.selected].info->file_type != IS_ERROR_FILE) {
				w32g_update_playlist();
				return 1;
			}
		}
		// for safety.
		if (playlist.selected == old_selected_index)
			break;
		err++;
		if (err > playlist.nfiles + 10)
			break;
	}
  return 0;
}

static struct playlist_shuffle_ {
	int * volatile list;
	int volatile cur;
	int volatile allocated;
	int volatile max;
} playlist_shuffle;
static int playlist_shuffle_init = 0;

#define PLAYLIST_SHUFFLE_LIST_SIZE 1024

int w32g_shuffle_playlist_reset(int preserve )
{
	int i;
	int cur_old = -1;
	int max_old = 0;
	int max = playlist.nfiles;
	int allocate_min;
	if ( max < 0 ) max = 0;
	if ( playlist_shuffle_init == 0 ){
		playlist_shuffle.list = NULL;
		playlist_shuffle.allocated = 0;
		playlist_shuffle.cur = -1;
		playlist_shuffle.max = 0;
		playlist_shuffle_init = 1;
	}
	if ( preserve ) {
		cur_old = playlist_shuffle.cur;
		max_old = playlist_shuffle.max;
	}
	allocate_min = playlist_shuffle.allocated - PLAYLIST_SHUFFLE_LIST_SIZE;
	if ( allocate_min < 0 ) allocate_min = 0;
	if ( playlist_shuffle.list == NULL || max < allocate_min || playlist_shuffle.allocated < max ) {
		playlist_shuffle.allocated = (max/PLAYLIST_SHUFFLE_LIST_SIZE + 1) * PLAYLIST_SHUFFLE_LIST_SIZE;
		playlist_shuffle.list = (int *) realloc ( playlist_shuffle.list, (playlist_shuffle.allocated + 1) * sizeof(int) );
		if ( playlist_shuffle.list == NULL ) {
			playlist_shuffle_init = 0;
			playlist_shuffle.cur = -1;
			playlist_shuffle.max = 0;
			return 0;
		}
	}
	for ( i = max_old; i < max; i ++ ){
		playlist_shuffle.list[i] = i;
	}
	playlist_shuffle.list[max] = -1;
	playlist_shuffle.cur = cur_old;
	playlist_shuffle.max = max;
	return 1;
}

int w32g_shuffle_playlist_next(int skip_invalid_file)
{
	if ( !playlist_shuffle_init ) {
		if ( !w32g_shuffle_playlist_reset(0) )
			return 0;
	}
	for ( playlist_shuffle.cur ++ ; playlist_shuffle.cur < playlist_shuffle.max; playlist_shuffle.cur ++ ) {
		int n = int_rand(playlist_shuffle.max - playlist_shuffle.cur) + playlist_shuffle.cur;
		int temp = playlist_shuffle.list[playlist_shuffle.cur];
		if ( n > playlist_shuffle.max ) n = playlist_shuffle.max;
		playlist_shuffle.list[playlist_shuffle.cur] = playlist_shuffle.list[n];
		playlist_shuffle.list[n] = temp;
		if ( playlist_shuffle.list[playlist_shuffle.cur] < playlist.nfiles ) {
			playlist.selected = playlist_shuffle.list[playlist_shuffle.cur];
			if(!skip_invalid_file ||
				playlist.list[playlist.selected].info->file_type != IS_ERROR_FILE) {
				w32g_update_playlist();
				return 1;
			}
		}
	}
    return 0;
}

// void w32g_rotate_playlist(int dest) —p
static int w32g_shuffle_playlist_rotate(int dest, int i1, int i2)
{
    int i, save;
	
	if ( i2 >= playlist_shuffle.max )
		i2 = playlist_shuffle.max - 1;
    if(i1 >= i2)
		return 1;
	
    if(dest > 0) {
		save = playlist_shuffle.list[i2];
		for(i = i2; i > i1; i--) /* i: i2 -> i1 */
			playlist_shuffle.list[i] = playlist_shuffle.list[i - 1];
		playlist_shuffle.list[i] = save;
		
	} else {
		save = playlist_shuffle.list[i1];
		for(i = i1; i < i2; i++) /* i: i1 -> i2 */
			playlist_shuffle.list[i] = playlist_shuffle.list[i + 1];
		playlist_shuffle.list[i] = save;
    }
	return 0;
}

// int w32g_delete_playlist(int pos) —p
static int w32g_shuffle_playlist_delete(int n)
{
	int i;
	int delete_flag = 0;
	for ( i = 0; i < playlist_shuffle.max; i++ ) {
		if ( playlist_shuffle.list[i] == n ) {
			delete_flag = 1;
			break;
		}
	}
	for ( ; i < playlist_shuffle.max; i++ ) {
		playlist_shuffle.list[i-1] = playlist_shuffle.list[i];
	}
	for ( i = 0; i < playlist_shuffle.max; i++ ) {
		if ( playlist_shuffle.list[i] >= n )
			playlist_shuffle.list[i]--;
	}
	if ( delete_flag )
		playlist_shuffle.max--;
	return 0;
}

void w32g_first_playlist(int skip_invalid_file)
{
    playlist.selected = 0;
    if(skip_invalid_file)
    {
	while(playlist.selected < playlist.nfiles &&
	      playlist.list[playlist.selected].info->file_type == IS_ERROR_FILE)
	    playlist.selected++;
	if(playlist.selected == playlist.nfiles)
	    playlist.selected = 0;
    }
    w32g_update_playlist();
}

int w32g_goto_playlist(int num, int skip_invalid_file)
{
    if(0 <= num && num < playlist.nfiles)
    {
	playlist.selected = num;
	if(skip_invalid_file)
	{
	    while(playlist.selected < playlist.nfiles &&
		  playlist.list[playlist.selected].info->file_type == IS_ERROR_FILE)
		playlist.selected++;
	    if(playlist.selected == playlist.nfiles)
		playlist.selected = num;
	}
	w32g_update_playlist();
	return 1;
    }
    return 0;
}

int w32g_isempty_playlist(void)
{
    return playlist.nfiles == 0;
}

#if 0
char *w32g_curr_playlist(void)
{
    if(!playlist.nfiles)
	return NULL;
    return playlist.list[playlist.selected].filename;
}
#endif

// Update an only list at the position.
void w32g_update_playlist_pos(int pos)
{
    int i, cur, modified;
    HWND hListBox;

    if(!(hListBox = playlist_box()))
	return;

    cur = ListBox_GetCurSel(hListBox);
    modified = 0;
	i = pos;
	if(i >= 0 && i < playlist.nfiles)
    {
	char *filename, *title, *item1, *item2;
	int maxlen, item2_len;
	int notitle = 0;

	filename = playlist.list[i].filename;
	title = playlist.list[i].title;
	if(title == NULL || title[0] == '\0')
	{
	    if(playlist.list[i].info->file_type == IS_ERROR_FILE)
		title = " --SKIP-- ";
		else
		{
//		title = " -------- ";
		title = playlist.list[i].filename;
		notitle = 1;
		}
	}
	maxlen = strlen(filename) + strlen(title) + 32 + 80;
	item1 = (char *)new_segment(&tmpbuffer, maxlen);
	if(!notitle)
	{
	if(i == playlist.selected)
	    snprintf(item1, maxlen, "==>%-80s   ==>(%s)", title, filename);
	else
	    snprintf(item1, maxlen, "   %-80s      (%s)", title, filename);
	} else
	{
	if(i == playlist.selected)
	    snprintf(item1, maxlen, "==>%-80s   ==>(%s)", title, filename);
	else
	    snprintf(item1, maxlen, "   %-80s      (%s)", title, filename);
	}
	item2_len = ListBox_GetTextLen(hListBox, i);
	item2 = (char *)new_segment(&tmpbuffer, item2_len + 1);
	ListBox_GetText(hListBox, i, item2);
	if(strcmp(item1, item2) != 0)
	{
	    ListBox_DeleteString(hListBox, i);
	    ListBox_InsertString(hListBox, i, item1);
	    modified = 1;
	}
	reuse_mblock(&tmpbuffer);
    }

    if(modified && cur==pos)
    {
	if(cur < 0)
	    cur = playlist.selected;
	else if(cur >= playlist.nfiles - 1)
	    cur = playlist.nfiles - 1;
	ListBox_SetCurSel(hListBox, cur);
	SetNumListWnd(cur,playlist.nfiles);
    }
}

void w32g_update_playlist(void)
{
#if 0
    int i, cur, modified;
    HWND hListBox;

    if(!(hListBox = playlist_box()))
	return;

    cur = ListBox_GetCurSel(hListBox);
    modified = 0;
    for(i = 0; i < playlist.nfiles; i++)
    {
	char *filename, *title, *item1, *item2;
	int maxlen, item2_len;

	filename = playlist.list[i].filename;
	title = playlist.list[i].title;
	if(title == NULL || title[0] == '\0')
	{
	    if(playlist.list[i].info->file_type == IS_ERROR_FILE)
		title = " --SKIP-- ";
	    else
		title = " -------- ";
	}
	maxlen = strlen(filename) + strlen(title) + 32;
	item1 = (char *)new_segment(&tmpbuffer, maxlen);
	if(i == playlist.selected)
	    snprintf(item1, maxlen, "==>%04d %s (%s)", i + 1, title, filename);
	else
	    snprintf(item1, maxlen, "   %04d %s (%s)", i + 1, title, filename);
	item2_len = ListBox_GetTextLen(hListBox, i);
	item2 = (char *)new_segment(&tmpbuffer, item2_len + 1);
	ListBox_GetText(hListBox, i, item2);
	if(strcmp(item1, item2) != 0)
	{
	    ListBox_DeleteString(hListBox, i);
	    ListBox_InsertString(hListBox, i, item1);
	    modified = 1;
	}
	reuse_mblock(&tmpbuffer);
    }

    if(modified)
    {
	if(cur < 0)
	    cur = playlist.selected;
	else if(cur >= playlist.nfiles - 1)
	    cur = playlist.nfiles - 1;
	ListBox_SetCurSel(hListBox, cur);
	SetNumListWnd(cur,playlist.nfiles);
    }
#else
    int i, cur, modified;
    HWND hListBox;

    if(!(hListBox = playlist_box()))
	return;

    cur = ListBox_GetCurSel(hListBox);
    modified = 0;
    for(i = 0; i < playlist.nfiles; i++)
    {
		w32g_update_playlist_pos(i);
    }

    if(modified)
    {
	if(cur < 0)
	    cur = playlist.selected;
	else if(cur >= playlist.nfiles - 1)
	    cur = playlist.nfiles - 1;
	ListBox_SetCurSel(hListBox, cur);
	SetNumListWnd(cur,playlist.nfiles);
    }
#endif
}

void w32g_get_playlist_index(int *selected, int *nfiles, int *cursel)
{
    if(selected != NULL)
	*selected = playlist.selected;
    if(nfiles != NULL)
	*nfiles = playlist.nfiles;
    if(cursel != NULL)
    {
	HWND hListBox;
	hListBox = playlist_box();
	if(hListBox)
	    *cursel = ListBox_GetCurSel(hListBox);
	else
	    *cursel = 0;
    }
}

int w32g_delete_playlist(int pos)
{
    int i;
    HWND hListBox;

    if(!(hListBox = playlist_box()))
	return 0;

    if(pos >= playlist.nfiles)
	return 0;

#ifdef W32G_RANDOM_IS_SHUFFLE
	w32g_shuffle_playlist_delete(pos);
#endif
    ListBox_DeleteString(hListBox, pos);
    free(playlist.list[pos].filename);
    playlist.nfiles--;
    for(i = pos; i < playlist.nfiles; i++)
		playlist.list[i] = playlist.list[i + 1];
    if(pos < playlist.selected || pos == playlist.nfiles)
    {
	playlist.selected--;
	if(playlist.selected < 0){
	    playlist.selected = 0;
		SetNumListWnd(playlist.selected,playlist.nfiles);
	} else
		w32g_update_playlist_pos(playlist.selected);
    }
    if(playlist.nfiles > 0)
    {
	if(pos == playlist.nfiles)
	    pos--;
	ListBox_SetCurSel(hListBox, pos);
	SetNumListWnd(pos,playlist.nfiles);
    }
    return 1;
}

int w32g_ismidi_playlist(int n)
{
    if(n < 0 || n >= playlist.nfiles)
	return 0;
    return playlist.list[n].info->format >= 0;
}

int w32g_nvalid_playlist(void)
{
    int i, n;

    n = 0;
    for(i = 0; i < playlist.nfiles; i++)
	if(w32g_ismidi_playlist(i))
	    n++;
    return n;
}

void w32g_setcur_playlist(void)
{
    HWND hListBox;
    if(!(hListBox = playlist_box()))
	return;
    ListBox_SetCurSel(hListBox, playlist.selected);
	SetNumListWnd(playlist.selected,playlist.nfiles);
}

int w32g_uniq_playlist(int *is_selected_removed)
{
    int nremoved;
    int i, n, j1, j2, cursel;
    HWND hListBox;

    hListBox = playlist_box();
    if(hListBox)
	cursel = ListBox_GetCurSel(hListBox);
    else
	cursel = -1;

    if(is_selected_removed != NULL)
	*is_selected_removed = 0;
    nremoved = 0;
    n = playlist.nfiles;
    for(i = 0; i < n - 1; i++)
    {
	int save_n;

	/* remove list[i] from list[i+1 .. n-1] */
	j1 = j2 = i + 1;
	save_n = n;
	while(j2 < save_n) /* j1 <= j2 */
	{
	    if(pathcmp(playlist.list[i].filename,
		       playlist.list[j2].filename, 0) == 0)
	    {
		nremoved++;
		n--;
		free(playlist.list[j2].filename);
		if(j2 == playlist.selected &&
		   is_selected_removed != NULL &&
		   !*is_selected_removed)
		{
		    *is_selected_removed = 1;
		    playlist.selected = j1;
		}
		if(j2 < playlist.selected)
		    playlist.selected--;
		if(j2 < cursel)
		    cursel--;
	    }
	    else
	    {
		playlist.list[j1] = playlist.list[j2];
		j1++;
	    }
	    j2++;
	}
    }
    if(nremoved)
    {
	for(i = 0; i < nremoved; i++)
	    ListBox_DeleteString(hListBox, --playlist.nfiles);
	if(cursel >= 0){
	    ListBox_SetCurSel(hListBox, cursel);
		SetNumListWnd(cursel,playlist.nfiles);
	}
	w32g_update_playlist();
    }
    return nremoved;
}

int w32g_refine_playlist(int *is_selected_removed)
{
    int nremoved;
    int i, j1, j2, cursel;
    HWND hListBox;

    hListBox = playlist_box();
    if(hListBox)
	cursel = ListBox_GetCurSel(hListBox);
    else
	cursel = -1;

    if(is_selected_removed != NULL)
	*is_selected_removed = 0;
    nremoved = 0;
    j1 = j2 = 0;
    while(j2 < playlist.nfiles) /* j1 <= j2 */
    {
	if(playlist.list[j2].info->format < 0)
	{
	    nremoved++;
	    free(playlist.list[j2].filename);
		if(j2 == playlist.selected &&
		   is_selected_removed != NULL &&
		   !*is_selected_removed)
		{
		    *is_selected_removed = 1;
		    playlist.selected = j1;
		}
		if(j2 < playlist.selected)
		    playlist.selected--;
		if(j2 < cursel)
		    cursel--;
	}
	else
	{
	    playlist.list[j1] = playlist.list[j2];
	    j1++;
	}
	j2++;
    }
    if(nremoved)
    {
	for(i = 0; i < nremoved; i++)
	    ListBox_DeleteString(hListBox, --playlist.nfiles);
	if(cursel >= playlist.nfiles)
	    cursel = playlist.nfiles - 1;
	if(cursel >= 0){
	    ListBox_SetCurSel(hListBox, cursel);
		SetNumListWnd(cursel,playlist.nfiles);
	}
	w32g_update_playlist();
    }
    return nremoved;
}

void w32g_clear_playlist(void)
{
    HWND hListBox;

    hListBox = playlist_box();
    while(playlist.nfiles > 0)
    {
	playlist.nfiles--;
	free(playlist.list[playlist.nfiles].filename);
#if 0
	if(hListBox)
	    ListBox_DeleteString(hListBox, playlist.nfiles);
#endif
    }
//	LB_RESETCONTENT
	if(hListBox)
	    ListBox_ResetContent(hListBox);
	playlist.selected = 0;
	SetNumListWnd(0,0);
}

void w32g_rotate_playlist(int dest)
{
    int i, i1, i2;
    HWND hListBox;
    PlayListEntry save;
	char temp[1024];

    if(playlist.nfiles == 0)
	return;
    if(!(hListBox = playlist_box()))
	return;

    i1 = ListBox_GetCurSel(hListBox);
    i2 = playlist.nfiles - 1;
    if(i1 >= i2)
	return;

#ifdef W32G_RANDOM_IS_SHUFFLE
	w32g_shuffle_playlist_rotate(dest,i1,i2);
#endif
    if(dest > 0)
    {
	save = playlist.list[i2];
	for(i = i2; i > i1; i--) /* i: i2 -> i1 */
	    playlist.list[i] = playlist.list[i - 1];
	playlist.list[i] = save;
	ListBox_GetText(hListBox,i2,temp);
    ListBox_DeleteString(hListBox,i2);
    ListBox_InsertString(hListBox,i1,temp);
	ListBox_SetCurSel(hListBox,i1);
	if(playlist.selected == i2){
	    playlist.selected = i1;
		w32g_update_playlist_pos(playlist.selected);
	} else if(i1 <= playlist.selected && playlist.selected < i2){
	    playlist.selected++;
		w32g_update_playlist_pos(playlist.selected);
	}
    }
    else
    {
	save = playlist.list[i1];
	for(i = i1; i < i2; i++) /* i: i1 -> i2 */
	    playlist.list[i] = playlist.list[i + 1];
	playlist.list[i] = save;
	ListBox_GetText(hListBox,i1,temp);
    ListBox_DeleteString(hListBox,i1);
    ListBox_InsertString(hListBox,-1,temp);
	ListBox_SetCurSel(hListBox,i1);
	if(playlist.selected == i1){
	    playlist.selected = i2;
		w32g_update_playlist_pos(playlist.selected);
	} else if(i1 < playlist.selected && playlist.selected <= i2){
	    playlist.selected--;    
		w32g_update_playlist_pos(playlist.selected);
	}
    }
}

char *w32g_get_playlist(int idx)
{
    if(idx < 0 || idx >= playlist.nfiles)
	return NULL;
    return playlist.list[idx].filename;
}
