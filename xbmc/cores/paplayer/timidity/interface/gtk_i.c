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

    gtk_i.c - Glenn Trigg 29 Oct 1998

*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H*/

#include <string.h>
#ifdef HAVE_GLOB_H
#include <glob.h>
#endif
#if HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#include <gtk/gtk.h>

#include "timidity.h"
#include "common.h"
#include "output.h"
#include "gtk_h.h"

#include "pixmaps/playpaus.xpm"
#include "pixmaps/prevtrk.xpm"
#include "pixmaps/nexttrk.xpm"
#include "pixmaps/rew.xpm"
#include "pixmaps/ff.xpm"
#include "pixmaps/restart.xpm"
#include "pixmaps/quit.xpm"
#include "pixmaps/quiet.xpm"
#include "pixmaps/loud.xpm"
#include "pixmaps/open.xpm"
#include "pixmaps/keyup.xpm"
#include "pixmaps/keydown.xpm"
#include "pixmaps/slow.xpm"
#include "pixmaps/fast.xpm"
#include "pixmaps/timidity.xpm"

static GtkWidget *create_menubar(void);
static GtkWidget *create_button_with_pixmap(GtkWidget *, gchar **, gint, gchar *);
static GtkWidget *create_pixmap_label(GtkWidget *, gchar **);
static gint delete_event(GtkWidget *, GdkEvent *, gpointer);
static void destroy (GtkWidget *, gpointer);
static GtkTooltips *create_yellow_tooltips(void);
static void handle_input(gpointer, gint, GdkInputCondition);
static void generic_cb(GtkWidget *, gpointer);
static void generic_scale_cb(GtkAdjustment *, gpointer);
static void open_file_cb(GtkWidget *, gpointer);
static void playlist_cb(GtkWidget *, guint);
static void playlist_op(GtkWidget *, guint);
static void file_list_cb(GtkWidget *, gint, gint, GdkEventButton *, gpointer);
static void clear_all_cb(GtkWidget *, gpointer);
static void filer_cb(GtkWidget *, gpointer);
static void tt_toggle_cb(GtkWidget *, gpointer);
static void locate_update_cb(GtkWidget *, GdkEventButton *, gpointer);
static void my_adjustment_set_value(GtkAdjustment *, gint);
static void set_icon_pixmap(GtkWidget *, gchar **);

static GtkWidget *window, *clist, *text, *vol_scale, *locator;
static GtkWidget *filesel = NULL, *plfilesel = NULL;
static GtkWidget *tot_lbl, *cnt_lbl, *auto_next, *ttshow;
static GtkTooltips *ttip;
static int file_number_to_play; /* Number of the file we're playing in the list */
static int max_sec, is_quitting = 0, locating = 0, local_adjust = 0;

static GtkItemFactoryEntry ife[] = {
    {"/File/Open", "<control>O", open_file_cb, 0, NULL},
    {"/File/sep", NULL, NULL, 0, "<Separator>"},
    {"/File/Load Playlist", "<control>L", playlist_cb,
     'l', NULL},
    {"/File/Save Playlist", "<control>S", playlist_cb,
     's', NULL},
    {"/File/sep", NULL, NULL, 0, "<Separator>"},
    {"/File/Quit", "<control>Q", generic_cb, GTK_QUIT, NULL},
    {"/Options/Auto next", "<control>A", NULL, 0, "<CheckItem>"},
    {"/Options/Show tooltips", "<control>T", tt_toggle_cb, 0, "<CheckItem>"},
    {"/Options/Clear All", "<control>C", clear_all_cb, 0, NULL}
};

#ifdef HAVE_GTK_2
static GtkTextBuffer *textbuf;
static GtkTextIter iter, start_iter, end_iter;
static GtkTextMark *mark;
#endif

/*----------------------------------------------------------------------*/

static void
generic_cb(GtkWidget *widget, gpointer data)
{
    gtk_pipe_int_write((int)data);
    if((int)data == GTK_PAUSE) {
	gtk_label_set(GTK_LABEL(cnt_lbl), "Pause");
    }
}

static void
tt_toggle_cb(GtkWidget *widget, gpointer data)
{
    if( GTK_CHECK_MENU_ITEM(ttshow)->active ) {
	gtk_tooltips_enable(ttip);
    }
    else {
	gtk_tooltips_disable(ttip);
    }
}

static void
open_file_cb(GtkWidget *widget, gpointer data)
{
    if( ! filesel ) {
	filesel = gtk_file_selection_new("Open File");
	gtk_file_selection_hide_fileop_buttons(GTK_FILE_SELECTION(filesel));

#ifdef HAVE_GTK_2
	gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(filesel)->ok_button),
			   "clicked",
			   G_CALLBACK (filer_cb), (gpointer)1);
#else
	gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(filesel)->ok_button),
			   "clicked",
			   GTK_SIGNAL_FUNC (filer_cb), (gpointer)1);
#endif
#ifdef HAVE_GTK_2
	gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(filesel)->cancel_button),
			   "clicked",
			   G_CALLBACK (filer_cb), (gpointer)0);
#else
	gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(filesel)->cancel_button),
			   "clicked",
			   GTK_SIGNAL_FUNC (filer_cb), (gpointer)0);
#endif
    }

    gtk_widget_show(GTK_WIDGET(filesel));
}

static void
filer_cb(GtkWidget *widget, gpointer data)
{
    gchar *filenames[2];
#ifdef GLOB_BRACE
    int i;
#ifdef HAVE_GTK_2
    const gchar *patt;
#else
    gchar *patt;
#endif
    glob_t pglob;

    if((int)data == 1) {
	patt = gtk_file_selection_get_filename(GTK_FILE_SELECTION(filesel));
	if(glob(patt, GLOB_BRACE|GLOB_NOMAGIC|GLOB_TILDE, NULL, &pglob))
	    return;
	for( i = 0; i < pglob.gl_pathc; i++) {
	    filenames[0] = pglob.gl_pathv[i];
	    filenames[1] = NULL;
	    gtk_clist_append(GTK_CLIST(clist), filenames);
	}
	globfree(&pglob);
    }
#else
    if((int)data == 1) {
	filenames[0] = gtk_file_selection_get_filename(GTK_FILE_SELECTION(filesel));
	filenames[1] = NULL;
	gtk_clist_append(GTK_CLIST(clist), filenames);
    }
#endif
    gtk_widget_hide(filesel);
    gtk_clist_columns_autosize(GTK_CLIST(clist));
}

static void
generic_scale_cb(GtkAdjustment *adj, gpointer data)
{
    if(local_adjust)
	return;

    gtk_pipe_int_write((int)data);

    /* This is a bit of a hack as the volume scale (a GtkVScale) seems
       to have it's minimum at the top which is counter-intuitive. */
    if((int)data == GTK_CHANGE_VOLUME) {
	gtk_pipe_int_write(MAX_AMPLIFICATION - adj->value);
    }
    else {
	gtk_pipe_int_write((int)adj->value*100);
    }
}

static void
file_list_cb(GtkWidget *widget, gint row, gint column,
	     GdkEventButton *event, gpointer data)
{
    gint	retval;
    gchar	*fname;

    if(event && (event->button == 3)) {
	if(event->type == GDK_2BUTTON_PRESS) {
	    gtk_clist_remove(GTK_CLIST(clist), row);
	}
	else {
	    return;
	}
    }
    retval = gtk_clist_get_text(GTK_CLIST(widget), row, 0, &fname);
    if(retval) {
	gtk_pipe_int_write(GTK_PLAY_FILE);
	gtk_pipe_string_write(fname);
	file_number_to_play=row;
    }
}

static void
playlist_cb(GtkWidget *widget, guint data)
{
    const gchar	*pldir;
    gchar	*plpatt;

    if( ! plfilesel ) {
	plfilesel = gtk_file_selection_new("");
	gtk_file_selection_hide_fileop_buttons(GTK_FILE_SELECTION(plfilesel));

	pldir = g_getenv("TIMIDITY_PLAYLIST_DIR");
	if(pldir != NULL) {
	    plpatt = g_strconcat(pldir, "/*.tpl", NULL);
	    gtk_file_selection_set_filename(GTK_FILE_SELECTION(plfilesel),
					    plpatt);
	    g_free(plpatt);
	}

	gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(plfilesel)->ok_button),
			   "clicked",
			   GTK_SIGNAL_FUNC (playlist_op), (gpointer)1);
	gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(plfilesel)->cancel_button),
			   "clicked",
			   GTK_SIGNAL_FUNC (playlist_op), NULL);
    }

    gtk_window_set_title(GTK_WINDOW(plfilesel), ((char)data == 'l')?
			 "Load Playlist":
			 "Save Playlist");
    gtk_object_set_user_data(GTK_OBJECT(plfilesel), (gpointer)data);
    gtk_file_selection_complete(GTK_FILE_SELECTION(plfilesel), "*.tpl");

    gtk_widget_show(plfilesel);
} /* playlist_cb */

static void
playlist_op(GtkWidget *widget, guint data)
{
    int		i;
    gchar	*filename[2], action, *rowdata, fname[BUFSIZ], *tmp;
    FILE	*plfp;

    gtk_widget_hide(plfilesel);

    if(!data)
	return;

    action = (gchar)(int)gtk_object_get_user_data(GTK_OBJECT(plfilesel));
    filename[0] = gtk_file_selection_get_filename(GTK_FILE_SELECTION(plfilesel));

    if(action == 'l') {
	if((plfp = fopen(filename[0], "r")) == NULL) {
	    g_error("Can't open %s for reading.", filename[0]);
	    return;
	}
	while(fgets(fname, BUFSIZ, plfp) != NULL) {
	    if(fname[strlen(fname) - 1] == '\n')
		fname[strlen(fname) - 1] = '\0';
	    filename[0] = fname;
	    filename[1] = NULL;
	    gtk_clist_append(GTK_CLIST(clist), filename);
	}
	fclose(plfp);
	gtk_clist_columns_autosize(GTK_CLIST(clist));
    }
    else if(action == 's') {
	if((plfp = fopen(filename[0], "w")) == NULL) {
	    g_error("Can't open %s for writing.", filename[0]);
	    return;
	}
	for(i = 0; i < GTK_CLIST(clist)->rows; i++) {
	    gtk_clist_get_text(GTK_CLIST(clist), i, 0, &rowdata);
	    /* Make sure we have an absolute path. */
	    if(*rowdata != '/') {
		tmp = g_get_current_dir();
		rowdata = g_strconcat(tmp, "/", rowdata, NULL);
		fprintf(plfp, "%s\n", rowdata);
		g_free(rowdata);
		g_free(tmp);
	    }
	    else {
		fprintf(plfp, "%s\n", rowdata);
	    }
	}
	fclose(plfp);
    }
    else {
	g_error("Invalid playlist action!.");
    }
} /* playlist_op */

static void
clear_all_cb(GtkWidget *widget, gpointer data)
{
    gtk_clist_clear(GTK_CLIST(clist));
} /* clear_all_cb */

static gint
delete_event(GtkWidget *widget, GdkEvent *event, gpointer data)
{
    return (FALSE);
}

static void
destroy (GtkWidget *widget, gpointer data)
{
    is_quitting = 1;
    gtk_pipe_int_write(GTK_QUIT);
}

static void
locate_update_cb (GtkWidget *widget, GdkEventButton *ev, gpointer data)
{
    if( (ev->button == 1) || (ev->button == 2)) {
	if( ev->type == GDK_BUTTON_RELEASE ) {
	    locating = 0;
	}
	else {
	    locating = 1;
	}
    }
}

static void
my_adjustment_set_value(GtkAdjustment *adj, gint value)
{
    local_adjust = 1;
    gtk_adjustment_set_value(adj, (gfloat)value);
    local_adjust = 0;
}

void
Launch_Gtk_Process(int pipe_number)
{
    int	argc = 0;
    GtkWidget *button, *mbar, *swin;
    GtkWidget *table, *align, *handlebox;
    GtkWidget *vbox, *hbox, *vbox2, *scrolled_win;
    GtkObject *adj;

    /* enable locale */
    gtk_set_locale ();

    gtk_init (&argc, NULL);

    ttip = create_yellow_tooltips();
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_widget_set_name(window, "TiMidity");
    gtk_window_set_title(GTK_WINDOW(window), "TiMidity - MIDI Player");
    gtk_window_set_wmclass(GTK_WINDOW(window), "timidity", "TiMidity");

    gtk_signal_connect(GTK_OBJECT(window), "delete_event",
		       GTK_SIGNAL_FUNC (delete_event), NULL);

    gtk_signal_connect(GTK_OBJECT(window), "destroy",
		       GTK_SIGNAL_FUNC (destroy), NULL);

    vbox = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    mbar = create_menubar();
    gtk_box_pack_start(GTK_BOX(vbox), mbar, FALSE, FALSE, 0);

    scrolled_win = gtk_scrolled_window_new(NULL, NULL);
    gtk_box_pack_start(GTK_BOX(vbox), scrolled_win, TRUE, TRUE ,0);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_win),GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
    gtk_widget_show(scrolled_win);

#ifdef HAVE_GTK_2
    text = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(text), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text), GTK_WRAP_WORD);
#else
    text = gtk_text_new(NULL, NULL);
#endif
    gtk_widget_show(text);
    gtk_container_add(GTK_CONTAINER(scrolled_win), text);

    hbox = gtk_hbox_new(FALSE, 4);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);
    gtk_widget_show(hbox);

    adj = gtk_adjustment_new(0., 0., 100., 1., 20., 0.);
    locator = gtk_hscale_new(GTK_ADJUSTMENT(adj));
    gtk_scale_set_draw_value(GTK_SCALE(locator), TRUE);
    gtk_signal_connect(GTK_OBJECT(adj), "value_changed",
			GTK_SIGNAL_FUNC(generic_scale_cb),
			(gpointer)GTK_CHANGE_LOCATOR);
    gtk_signal_connect(GTK_OBJECT(locator), "button_press_event",
			GTK_SIGNAL_FUNC(locate_update_cb),
			NULL);
    gtk_signal_connect(GTK_OBJECT(locator), "button_release_event",
			GTK_SIGNAL_FUNC(locate_update_cb),
			NULL);
    gtk_range_set_update_policy(GTK_RANGE(locator),
				GTK_UPDATE_DISCONTINUOUS);
    gtk_scale_set_digits(GTK_SCALE(locator), 0);
    gtk_widget_show(locator);
    gtk_box_pack_start(GTK_BOX(hbox), locator, TRUE, TRUE, 4);

    align = gtk_alignment_new(0., 1., 1., 0.);
    gtk_widget_show(align);
    cnt_lbl = gtk_label_new("00:00");
    gtk_widget_show(cnt_lbl);
    gtk_container_add(GTK_CONTAINER(align), cnt_lbl);
    gtk_box_pack_start(GTK_BOX(hbox), align, FALSE, TRUE, 0);

    align = gtk_alignment_new(0., 1., 1., 0.);
    gtk_widget_show(align);
    tot_lbl = gtk_label_new("/00:00");
    gtk_widget_show(tot_lbl);
    gtk_container_add(GTK_CONTAINER(align), tot_lbl);
    gtk_box_pack_start(GTK_BOX(hbox), align, FALSE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 4);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 4);

    swin = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin),
				   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    clist = gtk_clist_new(1);
    gtk_container_add(GTK_CONTAINER(swin), clist);
    gtk_widget_show(swin);
    gtk_widget_show(clist);
    gtk_widget_set_usize(clist, 200, 10);
    gtk_clist_set_reorderable(GTK_CLIST(clist), TRUE);
    gtk_clist_set_button_actions(GTK_CLIST(clist), 0, GTK_BUTTON_SELECTS);
    gtk_clist_set_button_actions(GTK_CLIST(clist), 1, GTK_BUTTON_DRAGS);
    gtk_clist_set_button_actions(GTK_CLIST(clist), 2, GTK_BUTTON_SELECTS);
    gtk_clist_set_selection_mode(GTK_CLIST(clist), GTK_SELECTION_SINGLE);
    gtk_clist_set_column_auto_resize(GTK_CLIST(clist), 1, TRUE);
    gtk_signal_connect(GTK_OBJECT(clist), "select_row",
		       GTK_SIGNAL_FUNC(file_list_cb), NULL);

    gtk_box_pack_start(GTK_BOX(hbox), swin, TRUE, TRUE, 0);

    vbox2 = gtk_vbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), vbox2, FALSE, FALSE, 0);
    gtk_widget_show(vbox2);

    /* This is so the pixmap creation works properly. */
    gtk_widget_realize(window);
    set_icon_pixmap(window, timidity_xpm);

    gtk_box_pack_start(GTK_BOX(vbox2),
		       create_pixmap_label(window, loud_xpm),
		       FALSE, FALSE, 0);

    adj = gtk_adjustment_new(30., 0., (gfloat)MAX_AMPLIFICATION, 1., 20., 0.);
    vol_scale = gtk_vscale_new(GTK_ADJUSTMENT(adj));
    gtk_scale_set_draw_value(GTK_SCALE(vol_scale), FALSE);
    gtk_signal_connect (GTK_OBJECT(adj), "value_changed",
			GTK_SIGNAL_FUNC(generic_scale_cb),
			(gpointer)GTK_CHANGE_VOLUME);
    gtk_range_set_update_policy(GTK_RANGE(vol_scale),
				GTK_UPDATE_DELAYED);
    gtk_widget_show(vol_scale);
    gtk_tooltips_set_tip(ttip, vol_scale, "Volume control", NULL);

    gtk_box_pack_start(GTK_BOX(vbox2), vol_scale, TRUE, TRUE, 0);

    gtk_box_pack_start(GTK_BOX(vbox2),
		       create_pixmap_label(window, quiet_xpm),
		       FALSE, FALSE, 0);

    handlebox = gtk_handle_box_new();
    gtk_box_pack_start(GTK_BOX(hbox), handlebox, FALSE, FALSE, 0);

    table = gtk_table_new(7, 2, TRUE);
    gtk_container_add(GTK_CONTAINER(handlebox), table);

    button = create_button_with_pixmap(window, playpaus_xpm, GTK_PAUSE,
				       "Play/Pause");
    gtk_table_attach_defaults(GTK_TABLE(table), button,
			      0, 2, 0, 1);

    button = create_button_with_pixmap(window, prevtrk_xpm, GTK_PREV,
				       "Previous file");
    gtk_table_attach_defaults(GTK_TABLE(table), button,
			      0, 1, 1, 2);

    button = create_button_with_pixmap(window, nexttrk_xpm, GTK_NEXT,
				       "Next file");
    gtk_table_attach_defaults(GTK_TABLE(table), button,
			      1, 2, 1, 2);

    button = create_button_with_pixmap(window, rew_xpm, GTK_RWD,
				       "Rewind");
    gtk_table_attach_defaults(GTK_TABLE(table), button,
			      0, 1, 2, 3);

    button = create_button_with_pixmap(window, ff_xpm, GTK_FWD,
				       "Fast forward");
    gtk_table_attach_defaults(GTK_TABLE(table), button,
			      1, 2, 2, 3);

    button = create_button_with_pixmap(window, keydown_xpm, GTK_KEYDOWN,
				       "Lower pitch");
    gtk_table_attach_defaults(GTK_TABLE(table), button,
			      0, 1, 3, 4);

    button = create_button_with_pixmap(window, keyup_xpm, GTK_KEYUP,
				       "Raise pitch");
    gtk_table_attach_defaults(GTK_TABLE(table), button,
			      1, 2, 3, 4);

    button = create_button_with_pixmap(window, slow_xpm, GTK_SLOWER,
				       "Decrease tempo");
    gtk_table_attach_defaults(GTK_TABLE(table), button,
			      0, 1, 4, 5);

    button = create_button_with_pixmap(window, fast_xpm, GTK_FASTER,
				       "Increase tempo");
    gtk_table_attach_defaults(GTK_TABLE(table), button,
			      1, 2, 4, 5);

    button = create_button_with_pixmap(window, restart_xpm, GTK_RESTART,
				       "Restart");
    gtk_table_attach_defaults(GTK_TABLE(table), button,
			      0, 1, 5, 6);

    button = create_button_with_pixmap(window, open_xpm, 0,
				       "Open");
#ifdef HAVE_GTK_2
    gtk_signal_disconnect_by_func(GTK_OBJECT(button), G_CALLBACK(generic_cb), 0);
#else
    gtk_signal_disconnect_by_func(GTK_OBJECT(button), generic_cb, 0);
#endif
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
			      GTK_SIGNAL_FUNC(open_file_cb), 0);
    gtk_table_attach_defaults(GTK_TABLE(table), button,
			      1, 2, 5, 6);

    button = create_button_with_pixmap(window, quit_xpm, GTK_QUIT,
				       "Quit");
    gtk_table_attach_defaults(GTK_TABLE(table), button,
			      0, 2, 6, 7);

    gtk_widget_show(hbox);
    gtk_widget_show(vbox);
    gtk_widget_show(table);
    gtk_widget_show(handlebox);
    gtk_widget_show(window);

    gdk_input_add(pipe_number, GDK_INPUT_READ, handle_input, NULL);

    gtk_main();
}

static GtkWidget *
create_button_with_pixmap(GtkWidget *window, gchar **bits, gint data, gchar *thelp)
{
    GtkWidget	*pw, *button;
    GdkPixmap	*pixmap;
    GdkBitmap	*mask;
    GtkStyle	*style;

    style = gtk_widget_get_style(window);
    pixmap = gdk_pixmap_create_from_xpm_d(window->window,
					  &mask,
					  &style->bg[GTK_STATE_NORMAL],
					  bits);
    pw = gtk_pixmap_new(pixmap, mask);
    gtk_widget_show(pw);

    button = gtk_button_new();
    gtk_container_add(GTK_CONTAINER(button), pw);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
			      GTK_SIGNAL_FUNC(generic_cb),
			      (gpointer)data);
    gtk_widget_show(button);
    gtk_tooltips_set_tip(ttip, button, thelp, NULL);

    return button;
}

static GtkWidget *
create_pixmap_label(GtkWidget *window, gchar **bits)
{
    GtkWidget	*pw;
    GdkPixmap	*pixmap;
    GdkBitmap	*mask;
    GtkStyle	*style;

    style = gtk_widget_get_style(window);
    pixmap = gdk_pixmap_create_from_xpm_d(window->window,
					  &mask,
					  &style->bg[GTK_STATE_NORMAL],
					  bits);
    pw = gtk_pixmap_new(pixmap, mask);
    gtk_widget_show(pw);

    return pw;
}

static void
set_icon_pixmap(GtkWidget *window, gchar **bits)
{
    GdkPixmap	*pixmap;
    GdkBitmap	*mask;
    GtkStyle	*style;

    style = gtk_widget_get_style(window);
    pixmap = gdk_pixmap_create_from_xpm_d(window->window,
					  &mask,
					  &style->bg[GTK_STATE_NORMAL],
					  bits);
    gdk_window_set_icon(window->window, NULL, pixmap, mask);
    gdk_window_set_icon_name(window->window, "TiMidity");
}

static GtkWidget *
create_menubar(void)
{
    GtkItemFactory	*ifactory;
    GtkAccelGroup	*ag;

#ifdef HAVE_GTK_2
    ag = gtk_accel_group_new();
#else
    ag = gtk_accel_group_get_default();
#endif
    ifactory = gtk_item_factory_new(GTK_TYPE_MENU_BAR, "<Main>", ag);
    gtk_item_factory_create_items(ifactory,
				  sizeof(ife) / sizeof(GtkItemFactoryEntry),
				  ife, NULL);
    gtk_widget_show(ifactory->widget);

    auto_next = gtk_item_factory_get_widget(ifactory, "/Options/Auto next");
    gtk_check_menu_item_set_state(GTK_CHECK_MENU_ITEM(auto_next), TRUE);
    ttshow = gtk_item_factory_get_widget(ifactory, "/Options/Show tooltips");
    gtk_check_menu_item_set_state(GTK_CHECK_MENU_ITEM(ttshow), TRUE);

    return ifactory->widget;
}

/* Following function curtesy of the gtk mailing list. */

static GtkTooltips *
create_yellow_tooltips()
{
    GdkColor	*t_back;
    GtkTooltips	*tip;

    t_back = (GdkColor*)g_malloc( sizeof(GdkColor));

    /* First create a default Tooltip */
    tip = gtk_tooltips_new();

#ifndef HAVE_GTK_2
    /* Try to get the colors */
    if ( gdk_color_parse("linen", t_back)){
	if(gdk_colormap_alloc_color(gdk_colormap_get_system(),
				    t_back,
				    FALSE, TRUE)) {
	    gtk_tooltips_set_colors(tip, t_back, NULL);
	}
    }
#endif

    return tip;
}

/* Receive DATA sent by the application on the pipe     */

static void
handle_input(gpointer client_data, gint source, GdkInputCondition ic)
{
    int message;

    gtk_pipe_int_read(&message);

    switch (message) {
    case REFRESH_MESSAGE:
	g_warning("REFRESH MESSAGE IS OBSOLETE !!!");
	break;

    case TOTALTIME_MESSAGE:
	{
	    int tt;
	    int minutes,seconds;
	    char local_string[20];
	    GtkObject *adj;

	    gtk_pipe_int_read(&tt);

	    seconds=max_sec=tt/play_mode->rate;
	    minutes=seconds/60;
	    seconds-=minutes*60;
	    sprintf(local_string,"/ %i:%02i",minutes,seconds);
	    gtk_label_set(GTK_LABEL(tot_lbl), local_string);

	    /* Readjust the time scale */
	    adj = gtk_adjustment_new(0., 0., (gfloat)max_sec,
				     1., 10., 0.);
	    gtk_signal_connect(GTK_OBJECT(adj), "value_changed",
			       GTK_SIGNAL_FUNC(generic_scale_cb),
			       (gpointer)GTK_CHANGE_LOCATOR);
	    gtk_range_set_adjustment(GTK_RANGE(locator),
				     GTK_ADJUSTMENT(adj));
	}
	break;

    case MASTERVOL_MESSAGE:
	{
	    int volume;
	    GtkAdjustment *adj;

	    gtk_pipe_int_read(&volume);
	    adj = gtk_range_get_adjustment(GTK_RANGE(vol_scale));
	    my_adjustment_set_value(adj, MAX_AMPLIFICATION - volume);
	}
	break;

    case FILENAME_MESSAGE:
	{
	    char filename[255], title[255];
	    char *pc;

	    gtk_pipe_string_read(filename);

	    /* Extract basename of the file */
	    pc = strrchr(filename, '/');
	    if (pc == NULL)
		pc = filename;
	    else
		pc++;

	    sprintf(title, "Timidity %s - %s", timidity_version, pc);
	    gtk_window_set_title(GTK_WINDOW(window), title);

	    /* Clear the text area. */
#ifdef HAVE_GTK_2
	    textbuf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text));
	    gtk_text_buffer_get_start_iter(textbuf, &start_iter);
	    gtk_text_buffer_get_end_iter(textbuf, &end_iter);
	    iter = start_iter;
#else
	    gtk_text_freeze(GTK_TEXT(text));
	    gtk_text_set_point(GTK_TEXT(text), 0);
	    gtk_text_forward_delete(GTK_TEXT(text),
				    gtk_text_get_length(GTK_TEXT(text)));
	    gtk_text_thaw(GTK_TEXT(text));
#endif
	}
	break;

    case FILE_LIST_MESSAGE:
	{
	    gchar filename[255], *fnames[2];
	    gint i, number_of_files, row;

	    /* reset the playing list : play from the start */
	    file_number_to_play = -1;

	    gtk_pipe_int_read(&number_of_files);
	    for (i = 0; i < number_of_files; i++)
	    {
		gtk_pipe_string_read(filename);
		fnames[0] = filename;
		fnames[1] = NULL;
		row = gtk_clist_append(GTK_CLIST(clist), fnames);
	    }
	    gtk_clist_columns_autosize(GTK_CLIST(clist));
	}
	break;

    case NEXT_FILE_MESSAGE:
    case PREV_FILE_MESSAGE:
    case TUNE_END_MESSAGE:
	{
	    int nbfile;

	    /* When a file ends, launch next if auto_next toggle */
	    if ( (message==TUNE_END_MESSAGE) &&
		 !GTK_CHECK_MENU_ITEM(auto_next)->active )
		return;

	    /* Total number of file to play in the list */
	    nbfile = GTK_CLIST(clist)->rows;

	    if (message == PREV_FILE_MESSAGE)
		file_number_to_play--;
	    else
		file_number_to_play++;

	    /* Do nothing if requested file is before first one */
	    if (file_number_to_play < 0) {
		file_number_to_play = 0;
		return;
	    }

	    /* Stop after playing the last file */
	    if (file_number_to_play >= nbfile) {
		file_number_to_play = nbfile - 1;
		return;
	    }

	    if(gtk_clist_row_is_visible(GTK_CLIST(clist),
					file_number_to_play) !=
	       GTK_VISIBILITY_FULL) {
		gtk_clist_moveto(GTK_CLIST(clist), file_number_to_play,
				 -1, 1.0, 0.0);
	    }
	    gtk_clist_select_row(GTK_CLIST(clist), file_number_to_play, 0);
	}
	break;

    case CURTIME_MESSAGE:
	{
	    int		seconds, minutes;
	    int		nbvoice;
	    char	local_string[20];

	    gtk_pipe_int_read(&seconds);
	    gtk_pipe_int_read(&nbvoice);

	    if( is_quitting )
		return;

	    minutes=seconds/60;

	    sprintf(local_string,"%2d:%02d", minutes, (int)(seconds % 60));

	    gtk_label_set(GTK_LABEL(cnt_lbl), local_string);

	    /* Readjust the time scale if not dragging the scale */
	    if( !locating && (seconds <= max_sec)) {
		GtkAdjustment *adj;

		adj = gtk_range_get_adjustment(GTK_RANGE(locator));
		my_adjustment_set_value(adj, (gfloat)seconds);
	    }
	}
	break;

    case NOTE_MESSAGE:
	{
	    int channel;
	    int note;

	    gtk_pipe_int_read(&channel);
	    gtk_pipe_int_read(&note);
	    g_warning("NOTE chn%i %i", channel, note);
	}
	break;

    case PROGRAM_MESSAGE:
	{
	    int channel;
	    int pgm;

	    gtk_pipe_int_read(&channel);
	    gtk_pipe_int_read(&pgm);
	    g_warning("NOTE chn%i %i", channel, pgm);
	}
	break;

    case VOLUME_MESSAGE:
	{
	    int channel;
	    int volume;

	    gtk_pipe_int_read(&channel);
	    gtk_pipe_int_read(&volume);
	    g_warning("VOLUME= chn%i %i", channel, volume);
	}
	break;


    case EXPRESSION_MESSAGE:
	{
	    int channel;
	    int express;

	    gtk_pipe_int_read(&channel);
	    gtk_pipe_int_read(&express);
	    g_warning("EXPRESSION= chn%i %i", channel, express);
	}
	break;

    case PANNING_MESSAGE:
	{
	    int channel;
	    int pan;

	    gtk_pipe_int_read(&channel);
	    gtk_pipe_int_read(&pan);
	    g_warning("PANNING= chn%i %i", channel, pan);
	}
	break;

    case SUSTAIN_MESSAGE:
	{
	    int channel;
	    int sust;

	    gtk_pipe_int_read(&channel);
	    gtk_pipe_int_read(&sust);
	    g_warning("SUSTAIN= chn%i %i", channel, sust);
	}
	break;

    case PITCH_MESSAGE:
	{
	    int channel;
	    int bend;

	    gtk_pipe_int_read(&channel);
	    gtk_pipe_int_read(&bend);
	    g_warning("PITCH BEND= chn%i %i", channel, bend);
	}
	break;

    case RESET_MESSAGE:
	g_warning("RESET_MESSAGE");
	break;

    case CLOSE_MESSAGE:
	gtk_exit(0);
	break;

    case CMSG_MESSAGE:
	{
	    int type;
	    char message[1000];
#ifdef HAVE_GTK_2
	    gchar *message_u8;
#endif

	    gtk_pipe_int_read(&type);
	    gtk_pipe_string_read(message);
#ifdef HAVE_GTK_2
	    message_u8 = g_locale_to_utf8( message, -1, NULL, NULL, NULL );
	    gtk_text_buffer_get_bounds(textbuf, &start_iter, &end_iter);
	    gtk_text_buffer_insert(textbuf, &end_iter,
			    message_u8, -1);
	    gtk_text_buffer_insert(textbuf, &end_iter,
			    "\n", 1);
	    gtk_text_buffer_get_bounds(textbuf, &start_iter, &end_iter);

	    mark = gtk_text_buffer_create_mark(textbuf, NULL, &end_iter, 1);
	    gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(text), mark, 0.0, 0, 0.0, 1.0);
	    gtk_text_buffer_delete_mark(textbuf, mark);
	    g_free( message_u8 );
#else
	    gtk_text_insert(GTK_TEXT(text), NULL, NULL, NULL,
			    message, -1);
	    gtk_text_insert(GTK_TEXT(text), NULL, NULL, NULL,
			    "\n", 1);
#endif
	}
	break;
    case LYRIC_MESSAGE:
	{
	    char message[1000];
#ifdef HAVE_GTK_2
	    gchar *message_u8;
#endif

	    gtk_pipe_string_read(message);

#ifdef HAVE_GTK_2
	    message_u8 = g_locale_to_utf8( message, -1, NULL, NULL, NULL );
	    gtk_text_buffer_get_bounds(textbuf, &start_iter, &end_iter);
	    gtk_text_buffer_insert(textbuf, &iter,
			    message_u8, -1);
	    gtk_text_buffer_get_bounds(textbuf, &start_iter, &end_iter);

	    mark = gtk_text_buffer_create_mark(textbuf, NULL, &end_iter, 1);
	    gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(text), mark, 0.0, 0, 0.0, 1.0);
	    gtk_text_buffer_delete_mark(textbuf, mark);
#else
	    gtk_text_insert(GTK_TEXT(text), NULL, NULL, NULL,
			    message, -1);
#endif
	}
	break;
    default:
	g_warning("UNKNOWN Gtk+ MESSAGE %i", message);
    }
}
