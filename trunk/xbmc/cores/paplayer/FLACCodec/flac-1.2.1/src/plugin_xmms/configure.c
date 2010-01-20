/* libxmms-flac - XMMS FLAC input plugin
 * Copyright (C) 2002,2003,2004,2005,2006  Daisuke Shimamura
 *
 * Based on mpg123 plugin
 *          and prefs.c - 2000/05/06
 *  EasyTAG - Tag editor for MP3 and OGG files
 *  Copyright (C) 2000-2002  Jerome Couderc <j.couderc@ifrance.com>
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
#include <glib.h>
#include <gtk/gtk.h>
#include <pthread.h>
#include <math.h>

#include <xmms/configfile.h>
#include <xmms/dirbrowser.h>
#include <xmms/titlestring.h>
#include <xmms/util.h>
#include <xmms/plugin.h>

#include "share/replaygain_synthesis.h" /* for NOISE_SHAPING_LOW */
#include "charset.h"
#include "configure.h"
#include "locale_hack.h"

/*
 * Initialize Global Valueable
 */
flac_config_t flac_cfg = {
	/* title */
	{
		FALSE, /* tag_override */
		NULL, /* tag_format */
		FALSE, /* convert_char_set */
		NULL /* user_char_set */
	},
	/* stream */
	{
		100 /* KB */, /* http_buffer_size */
		50, /* http_prebuffer */
		FALSE, /* use_proxy */
		NULL, /* proxy_host */
		0, /* proxy_port */
		FALSE, /* proxy_use_auth */
		NULL, /* proxy_user */
		NULL, /* proxy_pass */
		FALSE, /* save_http_stream */
		NULL, /* save_http_path */
		FALSE, /* cast_title_streaming */
		FALSE /* use_udp_channel */
	},
	/* output */
	{
		/* replaygain */
		{
			FALSE, /* enable */
			TRUE, /* album_mode */
			0, /* preamp */
			FALSE /* hard_limit */
		},
		/* resolution */
		{
			/* normal */
			{
				TRUE /* dither_24_to_16 */
			},
			/* replaygain */
			{
				TRUE, /* dither */
				NOISE_SHAPING_LOW, /* noise_shaping */
				16 /* bps_out */
			}
		}
	}
};


static GtkWidget *flac_configurewin = NULL;
static GtkWidget *vbox, *notebook;

static GtkWidget *title_tag_override, *title_tag_box, *title_tag_entry, *title_desc;
static GtkWidget *convert_char_set, *fileCharacterSetEntry, *userCharacterSetEntry;
static GtkWidget *replaygain_enable, *replaygain_album_mode;
static GtkWidget *replaygain_preamp_hscale, *replaygain_preamp_label, *replaygain_hard_limit;
static GtkObject *replaygain_preamp;
static GtkWidget *resolution_normal_dither_24_to_16;
static GtkWidget *resolution_replaygain_dither;
static GtkWidget *resolution_replaygain_noise_shaping_frame;
static GtkWidget *resolution_replaygain_noise_shaping_radio_none;
static GtkWidget *resolution_replaygain_noise_shaping_radio_low;
static GtkWidget *resolution_replaygain_noise_shaping_radio_medium;
static GtkWidget *resolution_replaygain_noise_shaping_radio_high;
static GtkWidget *resolution_replaygain_bps_out_frame;
static GtkWidget *resolution_replaygain_bps_out_radio_16bps;
static GtkWidget *resolution_replaygain_bps_out_radio_24bps;

static GtkObject *streaming_size_adj, *streaming_pre_adj;
static GtkWidget *streaming_proxy_use, *streaming_proxy_host_entry;
static GtkWidget *streaming_proxy_port_entry, *streaming_save_use, *streaming_save_entry;
static GtkWidget *streaming_proxy_auth_use;
static GtkWidget *streaming_proxy_auth_pass_entry, *streaming_proxy_auth_user_entry;
static GtkWidget *streaming_proxy_auth_user_label, *streaming_proxy_auth_pass_label;
#ifdef FLAC_ICECAST
static GtkWidget *streaming_cast_title, *streaming_udp_title;
#endif
static GtkWidget *streaming_proxy_hbox, *streaming_proxy_auth_hbox, *streaming_save_dirbrowser;
static GtkWidget *streaming_save_hbox;

static gchar *gtk_entry_get_text_1 (GtkWidget *widget);
static void flac_configurewin_ok(GtkWidget * widget, gpointer data);
static void configure_destroy(GtkWidget * w, gpointer data);

static void flac_configurewin_ok(GtkWidget * widget, gpointer data)
{
	ConfigFile *cfg;
	gchar *filename;

	(void)widget, (void)data; /* unused arguments */
	g_free(flac_cfg.title.tag_format);
	flac_cfg.title.tag_format = g_strdup(gtk_entry_get_text(GTK_ENTRY(title_tag_entry)));
	flac_cfg.title.user_char_set = Charset_Get_Name_From_Title(gtk_entry_get_text_1(userCharacterSetEntry));

	filename = g_strconcat(g_get_home_dir(), "/.xmms/config", NULL);
	cfg = xmms_cfg_open_file(filename);
	if (!cfg)
		cfg = xmms_cfg_new();
	/* title */
	xmms_cfg_write_boolean(cfg, "flac", "title.tag_override", flac_cfg.title.tag_override);
	xmms_cfg_write_string(cfg, "flac", "title.tag_format", flac_cfg.title.tag_format);
	xmms_cfg_write_boolean(cfg, "flac", "title.convert_char_set", flac_cfg.title.convert_char_set);
	xmms_cfg_write_string(cfg, "flac", "title.user_char_set", flac_cfg.title.user_char_set);
	/* output */
	xmms_cfg_write_boolean(cfg, "flac", "output.replaygain.enable", flac_cfg.output.replaygain.enable);
	xmms_cfg_write_boolean(cfg, "flac", "output.replaygain.album_mode", flac_cfg.output.replaygain.album_mode);
	xmms_cfg_write_int(cfg, "flac", "output.replaygain.preamp", flac_cfg.output.replaygain.preamp);
	xmms_cfg_write_boolean(cfg, "flac", "output.replaygain.hard_limit", flac_cfg.output.replaygain.hard_limit);
	xmms_cfg_write_boolean(cfg, "flac", "output.resolution.normal.dither_24_to_16", flac_cfg.output.resolution.normal.dither_24_to_16);
	xmms_cfg_write_boolean(cfg, "flac", "output.resolution.replaygain.dither", flac_cfg.output.resolution.replaygain.dither);
	xmms_cfg_write_int(cfg, "flac", "output.resolution.replaygain.noise_shaping", flac_cfg.output.resolution.replaygain.noise_shaping);
	xmms_cfg_write_int(cfg, "flac", "output.resolution.replaygain.bps_out", flac_cfg.output.resolution.replaygain.bps_out);
	/* streaming */
	flac_cfg.stream.http_buffer_size = (gint) GTK_ADJUSTMENT(streaming_size_adj)->value;
	flac_cfg.stream.http_prebuffer = (gint) GTK_ADJUSTMENT(streaming_pre_adj)->value;

	flac_cfg.stream.use_proxy = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(streaming_proxy_use));
	if(flac_cfg.stream.proxy_host)
		g_free(flac_cfg.stream.proxy_host);
	flac_cfg.stream.proxy_host = g_strdup(gtk_entry_get_text(GTK_ENTRY(streaming_proxy_host_entry)));
	flac_cfg.stream.proxy_port = atoi(gtk_entry_get_text(GTK_ENTRY(streaming_proxy_port_entry)));

	flac_cfg.stream.proxy_use_auth = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(streaming_proxy_auth_use));

	if(flac_cfg.stream.proxy_user)
		g_free(flac_cfg.stream.proxy_user);
	flac_cfg.stream.proxy_user = NULL;
	if(strlen(gtk_entry_get_text(GTK_ENTRY(streaming_proxy_auth_user_entry))) > 0)
		flac_cfg.stream.proxy_user = g_strdup(gtk_entry_get_text(GTK_ENTRY(streaming_proxy_auth_user_entry)));

	if(flac_cfg.stream.proxy_pass)
		g_free(flac_cfg.stream.proxy_pass);
	flac_cfg.stream.proxy_pass = NULL;
	if(strlen(gtk_entry_get_text(GTK_ENTRY(streaming_proxy_auth_pass_entry))) > 0)
		flac_cfg.stream.proxy_pass = g_strdup(gtk_entry_get_text(GTK_ENTRY(streaming_proxy_auth_pass_entry)));


	flac_cfg.stream.save_http_stream = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(streaming_save_use));
	if (flac_cfg.stream.save_http_path)
		g_free(flac_cfg.stream.save_http_path);
	flac_cfg.stream.save_http_path = g_strdup(gtk_entry_get_text(GTK_ENTRY(streaming_save_entry)));

#ifdef FLAC_ICECAST
	flac_cfg.stream.cast_title_streaming = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(streaming_cast_title));
	flac_cfg.stream.use_udp_channel = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(streaming_udp_title));
#endif

	xmms_cfg_write_int(cfg, "flac", "stream.http_buffer_size", flac_cfg.stream.http_buffer_size);
	xmms_cfg_write_int(cfg, "flac", "stream.http_prebuffer", flac_cfg.stream.http_prebuffer);
	xmms_cfg_write_boolean(cfg, "flac", "stream.use_proxy", flac_cfg.stream.use_proxy);
	xmms_cfg_write_string(cfg, "flac", "stream.proxy_host", flac_cfg.stream.proxy_host);
	xmms_cfg_write_int(cfg, "flac", "stream.proxy_port", flac_cfg.stream.proxy_port);
	xmms_cfg_write_boolean(cfg, "flac", "stream.proxy_use_auth", flac_cfg.stream.proxy_use_auth);
	if(flac_cfg.stream.proxy_user)
		xmms_cfg_write_string(cfg, "flac", "stream.proxy_user", flac_cfg.stream.proxy_user);
	else
		xmms_cfg_remove_key(cfg, "flac", "stream.proxy_user");
	if(flac_cfg.stream.proxy_pass)
		xmms_cfg_write_string(cfg, "flac", "stream.proxy_pass", flac_cfg.stream.proxy_pass);
	else
		xmms_cfg_remove_key(cfg, "flac", "stream.proxy_pass");
	xmms_cfg_write_boolean(cfg, "flac", "stream.save_http_stream", flac_cfg.stream.save_http_stream);
	xmms_cfg_write_string(cfg, "flac", "stream.save_http_path", flac_cfg.stream.save_http_path);
#ifdef FLAC_ICECAST
	xmms_cfg_write_boolean(cfg, "flac", "stream.cast_title_streaming", flac_cfg.stream.cast_title_streaming);
	xmms_cfg_write_boolean(cfg, "flac", "stream.use_udp_channel", flac_cfg.stream.use_udp_channel);
#endif

	xmms_cfg_write_file(cfg, filename);
	xmms_cfg_free(cfg);
	g_free(filename);
	gtk_widget_destroy(flac_configurewin);
}

static void configure_destroy(GtkWidget *widget, gpointer data)
{
	(void)widget, (void)data; /* unused arguments */
}

static void title_tag_override_cb(GtkWidget *widget, gpointer data)
{
	(void)widget, (void)data; /* unused arguments */
	flac_cfg.title.tag_override = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(title_tag_override));

	gtk_widget_set_sensitive(title_tag_box, flac_cfg.title.tag_override);
	gtk_widget_set_sensitive(title_desc, flac_cfg.title.tag_override);

}

static void convert_char_set_cb(GtkWidget *widget, gpointer data)
{
	(void)widget, (void)data; /* unused arguments */
	flac_cfg.title.convert_char_set = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(convert_char_set));

	gtk_widget_set_sensitive(fileCharacterSetEntry, FALSE);
	gtk_widget_set_sensitive(userCharacterSetEntry, flac_cfg.title.convert_char_set);
}

static void replaygain_enable_cb(GtkWidget *widget, gpointer data)
{
	(void)widget, (void)data; /* unused arguments */
	flac_cfg.output.replaygain.enable = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(replaygain_enable));

	gtk_widget_set_sensitive(replaygain_album_mode, flac_cfg.output.replaygain.enable);
	gtk_widget_set_sensitive(replaygain_preamp_hscale, flac_cfg.output.replaygain.enable);
	gtk_widget_set_sensitive(replaygain_hard_limit, flac_cfg.output.replaygain.enable);
}

static void replaygain_album_mode_cb(GtkWidget *widget, gpointer data)
{
	(void)widget, (void)data; /* unused arguments */
	flac_cfg.output.replaygain.album_mode = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(replaygain_album_mode));
}

static void replaygain_hard_limit_cb(GtkWidget *widget, gpointer data)
{
	(void)widget, (void)data; /* unused arguments */
	flac_cfg.output.replaygain.hard_limit = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(replaygain_hard_limit));
}

static void replaygain_preamp_cb(GtkWidget *widget, gpointer data)
{
	GString *gstring = g_string_new("");
	(void)widget, (void)data; /* unused arguments */
	flac_cfg.output.replaygain.preamp = (int) floor(GTK_ADJUSTMENT(replaygain_preamp)->value + 0.5);

	g_string_sprintf(gstring, "%i dB", flac_cfg.output.replaygain.preamp);
	gtk_label_set_text(GTK_LABEL(replaygain_preamp_label), _(gstring->str));
}

static void resolution_normal_dither_24_to_16_cb(GtkWidget *widget, gpointer data)
{
	(void)widget, (void)data; /* unused arguments */
	flac_cfg.output.resolution.normal.dither_24_to_16 = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(resolution_normal_dither_24_to_16));
}

static void resolution_replaygain_dither_cb(GtkWidget *widget, gpointer data)
{
	(void)widget, (void)data; /* unused arguments */
	flac_cfg.output.resolution.replaygain.dither = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(resolution_replaygain_dither));

	gtk_widget_set_sensitive(resolution_replaygain_noise_shaping_frame, flac_cfg.output.resolution.replaygain.dither);
}

static void resolution_replaygain_noise_shaping_cb(GtkWidget *widget, gpointer data)
{
	(void)widget, (void)data; /* unused arguments */
	flac_cfg.output.resolution.replaygain.noise_shaping =
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(resolution_replaygain_noise_shaping_radio_none))? 0 :
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(resolution_replaygain_noise_shaping_radio_low))? 1 :
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(resolution_replaygain_noise_shaping_radio_medium))? 2 :
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(resolution_replaygain_noise_shaping_radio_high))? 3 :
		0
	;
}

static void resolution_replaygain_bps_out_cb(GtkWidget *widget, gpointer data)
{
	(void)widget, (void)data; /* unused arguments */
	flac_cfg.output.resolution.replaygain.bps_out =
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(resolution_replaygain_bps_out_radio_16bps))? 16 :
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(resolution_replaygain_bps_out_radio_24bps))? 24 :
		16
	;
}

static void proxy_use_cb(GtkWidget * w, gpointer data)
{
	gboolean use_proxy, use_proxy_auth;
	(void) w;
	(void) data;

	use_proxy = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(streaming_proxy_use));
	use_proxy_auth = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(streaming_proxy_auth_use));

	gtk_widget_set_sensitive(streaming_proxy_hbox, use_proxy);
	gtk_widget_set_sensitive(streaming_proxy_auth_use, use_proxy);
	gtk_widget_set_sensitive(streaming_proxy_auth_hbox, use_proxy && use_proxy_auth);
}

static void proxy_auth_use_cb(GtkWidget *w, gpointer data)
{
	gboolean use_proxy, use_proxy_auth;
	(void) w;
	(void) data;

	use_proxy = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(streaming_proxy_use));
	use_proxy_auth = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(streaming_proxy_auth_use));

	gtk_widget_set_sensitive(streaming_proxy_auth_hbox, use_proxy && use_proxy_auth);
}

static void streaming_save_dirbrowser_cb(gchar * dir)
{
	gtk_entry_set_text(GTK_ENTRY(streaming_save_entry), dir);
}

static void streaming_save_browse_cb(GtkWidget * w, gpointer data)
{
	(void) w;
	(void) data;
	if (!streaming_save_dirbrowser)
	{
		streaming_save_dirbrowser = xmms_create_dir_browser(_("Select the directory where you want to store the MPEG streams:"),
								    flac_cfg.stream.save_http_path, GTK_SELECTION_SINGLE, streaming_save_dirbrowser_cb);
		gtk_signal_connect(GTK_OBJECT(streaming_save_dirbrowser), "destroy", GTK_SIGNAL_FUNC(gtk_widget_destroyed), &streaming_save_dirbrowser);
		gtk_window_set_transient_for(GTK_WINDOW(streaming_save_dirbrowser), GTK_WINDOW(flac_configurewin));
		gtk_widget_show(streaming_save_dirbrowser);
	}
}

static void streaming_save_use_cb(GtkWidget * w, gpointer data)
{
	gboolean save_stream;
	(void) w;
	(void) data;

	save_stream = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(streaming_save_use));

	gtk_widget_set_sensitive(streaming_save_hbox, save_stream);
}


void FLAC_XMMS__configure(void)
{
	GtkWidget *title_frame, *title_tag_vbox, *title_tag_label;
	GtkWidget *replaygain_frame, *resolution_frame, *output_vbox, *resolution_normal_frame, *resolution_replaygain_frame;
	GtkWidget *replaygain_vbox, *resolution_hbox, *resolution_normal_vbox, *resolution_replaygain_vbox;
	GtkWidget *resolution_replaygain_noise_shaping_vbox;
	GtkWidget *resolution_replaygain_bps_out_vbox;
	GtkWidget *label, *hbox;
	GtkWidget *bbox, *ok, *cancel;
	GList *list;

	GtkWidget *streaming_vbox;
	GtkWidget *streaming_buf_frame, *streaming_buf_hbox;
	GtkWidget *streaming_size_box, *streaming_size_label, *streaming_size_spin;
	GtkWidget *streaming_pre_box, *streaming_pre_label, *streaming_pre_spin;
	GtkWidget *streaming_proxy_frame, *streaming_proxy_vbox;
	GtkWidget *streaming_proxy_port_label, *streaming_proxy_host_label;
	GtkWidget *streaming_save_frame, *streaming_save_vbox;
	GtkWidget *streaming_save_label, *streaming_save_browse;
#ifdef FLAC_ICECAST
	GtkWidget *streaming_cast_frame, *streaming_cast_vbox;
#endif
	char *temp;

	if (flac_configurewin != NULL) {
		gdk_window_raise(flac_configurewin->window);
		return;
	}
	flac_configurewin = gtk_window_new(GTK_WINDOW_DIALOG);
	gtk_signal_connect(GTK_OBJECT(flac_configurewin), "destroy", GTK_SIGNAL_FUNC(gtk_widget_destroyed), &flac_configurewin);
	gtk_signal_connect(GTK_OBJECT(flac_configurewin), "destroy", GTK_SIGNAL_FUNC(configure_destroy), &flac_configurewin);
	gtk_window_set_title(GTK_WINDOW(flac_configurewin), _("Flac Configuration"));
	gtk_window_set_policy(GTK_WINDOW(flac_configurewin), FALSE, FALSE, FALSE);
	gtk_container_border_width(GTK_CONTAINER(flac_configurewin), 10);

	vbox = gtk_vbox_new(FALSE, 10);
	gtk_container_add(GTK_CONTAINER(flac_configurewin), vbox);

	notebook = gtk_notebook_new();
	gtk_box_pack_start(GTK_BOX(vbox), notebook, TRUE, TRUE, 0);

	/* Title config.. */

	title_frame = gtk_frame_new(_("Tag Handling"));
	gtk_container_border_width(GTK_CONTAINER(title_frame), 5);

	title_tag_vbox = gtk_vbox_new(FALSE, 10);
	gtk_container_border_width(GTK_CONTAINER(title_tag_vbox), 5);
	gtk_container_add(GTK_CONTAINER(title_frame), title_tag_vbox);

	/* Convert Char Set */

	convert_char_set = gtk_check_button_new_with_label(_("Convert Character Set"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(convert_char_set), flac_cfg.title.convert_char_set);
	gtk_signal_connect(GTK_OBJECT(convert_char_set), "clicked", convert_char_set_cb, NULL);
	gtk_box_pack_start(GTK_BOX(title_tag_vbox), convert_char_set, FALSE, FALSE, 0);
	/*  Combo boxes... */
	hbox = gtk_hbox_new(FALSE,4);
	gtk_container_add(GTK_CONTAINER(title_tag_vbox),hbox);
	label = gtk_label_new(_("Convert character set from :"));
	gtk_box_pack_start(GTK_BOX(hbox),label,FALSE,FALSE,0);
	fileCharacterSetEntry = gtk_combo_new();
	gtk_box_pack_start(GTK_BOX(hbox),fileCharacterSetEntry,TRUE,TRUE,0);

	label = gtk_label_new (_("to :"));
	gtk_box_pack_start(GTK_BOX(hbox),label,FALSE,FALSE,0);
	userCharacterSetEntry = gtk_combo_new();
	gtk_box_pack_start(GTK_BOX(hbox),userCharacterSetEntry,TRUE,TRUE,0);

	gtk_entry_set_editable(GTK_ENTRY(GTK_COMBO(fileCharacterSetEntry)->entry),FALSE);
	gtk_entry_set_editable(GTK_ENTRY(GTK_COMBO(userCharacterSetEntry)->entry),FALSE);
	gtk_combo_set_value_in_list(GTK_COMBO(fileCharacterSetEntry),TRUE,FALSE);
	gtk_combo_set_value_in_list(GTK_COMBO(userCharacterSetEntry),TRUE,FALSE);

	list = Charset_Create_List();
	gtk_combo_set_popdown_strings(GTK_COMBO(fileCharacterSetEntry),Charset_Create_List_UTF8_Only());
	gtk_combo_set_popdown_strings(GTK_COMBO(userCharacterSetEntry),list);
	gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(userCharacterSetEntry)->entry),Charset_Get_Title_From_Name(flac_cfg.title.user_char_set));
	gtk_widget_set_sensitive(fileCharacterSetEntry, FALSE);
	gtk_widget_set_sensitive(userCharacterSetEntry, flac_cfg.title.convert_char_set);

	/* Override Tagging Format */

	title_tag_override = gtk_check_button_new_with_label(_("Override generic titles"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(title_tag_override), flac_cfg.title.tag_override);
	gtk_signal_connect(GTK_OBJECT(title_tag_override), "clicked", title_tag_override_cb, NULL);
	gtk_box_pack_start(GTK_BOX(title_tag_vbox), title_tag_override, FALSE, FALSE, 0);

	title_tag_box = gtk_hbox_new(FALSE, 5);
	gtk_widget_set_sensitive(title_tag_box, flac_cfg.title.tag_override);
	gtk_box_pack_start(GTK_BOX(title_tag_vbox), title_tag_box, FALSE, FALSE, 0);

	title_tag_label = gtk_label_new(_("Title format:"));
	gtk_box_pack_start(GTK_BOX(title_tag_box), title_tag_label, FALSE, FALSE, 0);

	title_tag_entry = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(title_tag_entry), flac_cfg.title.tag_format);
	gtk_box_pack_start(GTK_BOX(title_tag_box), title_tag_entry, TRUE, TRUE, 0);

	title_desc = xmms_titlestring_descriptions("pafFetnygc", 2);
	gtk_widget_set_sensitive(title_desc, flac_cfg.title.tag_override);
	gtk_box_pack_start(GTK_BOX(title_tag_vbox), title_desc, FALSE, FALSE, 0);

	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), title_frame, gtk_label_new(_("Title")));

	/* Output config.. */

	output_vbox = gtk_vbox_new(FALSE, 10);
	gtk_container_border_width(GTK_CONTAINER(output_vbox), 5);

	/* replaygain */

	replaygain_frame = gtk_frame_new(_("ReplayGain"));
	gtk_container_border_width(GTK_CONTAINER(replaygain_frame), 5);
	gtk_box_pack_start(GTK_BOX(output_vbox), replaygain_frame, TRUE, TRUE, 0);

	replaygain_vbox = gtk_vbox_new(FALSE, 10);
	gtk_container_border_width(GTK_CONTAINER(replaygain_vbox), 5);
	gtk_container_add(GTK_CONTAINER(replaygain_frame), replaygain_vbox);

	replaygain_enable = gtk_check_button_new_with_label(_("Enable ReplayGain processing"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(replaygain_enable), flac_cfg.output.replaygain.enable);
	gtk_signal_connect(GTK_OBJECT(replaygain_enable), "clicked", replaygain_enable_cb, NULL);
	gtk_box_pack_start(GTK_BOX(replaygain_vbox), replaygain_enable, FALSE, FALSE, 0);

	replaygain_album_mode = gtk_check_button_new_with_label(_("Album mode"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(replaygain_album_mode), flac_cfg.output.replaygain.album_mode);
	gtk_signal_connect(GTK_OBJECT(replaygain_album_mode), "clicked", replaygain_album_mode_cb, NULL);
	gtk_box_pack_start(GTK_BOX(replaygain_vbox), replaygain_album_mode, FALSE, FALSE, 0);

	hbox = gtk_hbox_new(FALSE,3);
	gtk_container_add(GTK_CONTAINER(replaygain_vbox),hbox);
	label = gtk_label_new(_("Preamp:"));
	gtk_box_pack_start(GTK_BOX(hbox),label,FALSE,FALSE,0);
	replaygain_preamp = gtk_adjustment_new(flac_cfg.output.replaygain.preamp, -24.0, +24.0, 1.0, 6.0, 0.0);
	gtk_signal_connect(GTK_OBJECT(replaygain_preamp), "value-changed", replaygain_preamp_cb, NULL);
	replaygain_preamp_hscale = gtk_hscale_new(GTK_ADJUSTMENT(replaygain_preamp));
	gtk_scale_set_draw_value(GTK_SCALE(replaygain_preamp_hscale), FALSE);
	gtk_box_pack_start(GTK_BOX(hbox),replaygain_preamp_hscale,TRUE,TRUE,0);
	replaygain_preamp_label = gtk_label_new(_("0 dB"));
	gtk_box_pack_start(GTK_BOX(hbox),replaygain_preamp_label,FALSE,FALSE,0);
	gtk_adjustment_value_changed(GTK_ADJUSTMENT(replaygain_preamp));

	replaygain_hard_limit = gtk_check_button_new_with_label(_("6dB hard limiting"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(replaygain_hard_limit), flac_cfg.output.replaygain.hard_limit);
	gtk_signal_connect(GTK_OBJECT(replaygain_hard_limit), "clicked", replaygain_hard_limit_cb, NULL);
	gtk_box_pack_start(GTK_BOX(replaygain_vbox), replaygain_hard_limit, FALSE, FALSE, 0);

	replaygain_enable_cb(replaygain_enable, NULL);

	/* resolution */

	resolution_frame = gtk_frame_new(_("Resolution"));
	gtk_container_border_width(GTK_CONTAINER(resolution_frame), 5);
	gtk_box_pack_start(GTK_BOX(output_vbox), resolution_frame, TRUE, TRUE, 0);

	resolution_hbox = gtk_hbox_new(FALSE, 10);
	gtk_container_border_width(GTK_CONTAINER(resolution_hbox), 5);
	gtk_container_add(GTK_CONTAINER(resolution_frame), resolution_hbox);

	resolution_normal_frame = gtk_frame_new(_("Without ReplayGain"));
	gtk_container_border_width(GTK_CONTAINER(resolution_normal_frame), 5);
	gtk_box_pack_start(GTK_BOX(resolution_hbox), resolution_normal_frame, TRUE, TRUE, 0);

	resolution_normal_vbox = gtk_vbox_new(FALSE, 10);
	gtk_container_border_width(GTK_CONTAINER(resolution_normal_vbox), 5);
	gtk_container_add(GTK_CONTAINER(resolution_normal_frame), resolution_normal_vbox);

	resolution_normal_dither_24_to_16 = gtk_check_button_new_with_label(_("Dither 24bps to 16bps"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(resolution_normal_dither_24_to_16), flac_cfg.output.resolution.normal.dither_24_to_16);
	gtk_signal_connect(GTK_OBJECT(resolution_normal_dither_24_to_16), "clicked", resolution_normal_dither_24_to_16_cb, NULL);
	gtk_box_pack_start(GTK_BOX(resolution_normal_vbox), resolution_normal_dither_24_to_16, FALSE, FALSE, 0);

	resolution_replaygain_frame = gtk_frame_new(_("With ReplayGain"));
	gtk_container_border_width(GTK_CONTAINER(resolution_replaygain_frame), 5);
	gtk_box_pack_start(GTK_BOX(resolution_hbox), resolution_replaygain_frame, TRUE, TRUE, 0);

	resolution_replaygain_vbox = gtk_vbox_new(FALSE, 10);
	gtk_container_border_width(GTK_CONTAINER(resolution_replaygain_vbox), 5);
	gtk_container_add(GTK_CONTAINER(resolution_replaygain_frame), resolution_replaygain_vbox);

	resolution_replaygain_dither = gtk_check_button_new_with_label(_("Enable dithering"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(resolution_replaygain_dither), flac_cfg.output.resolution.replaygain.dither);
	gtk_signal_connect(GTK_OBJECT(resolution_replaygain_dither), "clicked", resolution_replaygain_dither_cb, NULL);
	gtk_box_pack_start(GTK_BOX(resolution_replaygain_vbox), resolution_replaygain_dither, FALSE, FALSE, 0);

	hbox = gtk_hbox_new(FALSE, 10);
	gtk_container_border_width(GTK_CONTAINER(hbox), 5);
	gtk_box_pack_start(GTK_BOX(resolution_replaygain_vbox), hbox, TRUE, TRUE, 0);

	resolution_replaygain_noise_shaping_frame = gtk_frame_new(_("Noise shaping"));
	gtk_container_border_width(GTK_CONTAINER(resolution_replaygain_noise_shaping_frame), 5);
	gtk_box_pack_start(GTK_BOX(hbox), resolution_replaygain_noise_shaping_frame, TRUE, TRUE, 0);

	resolution_replaygain_noise_shaping_vbox = gtk_vbutton_box_new();
	gtk_container_border_width(GTK_CONTAINER(resolution_replaygain_noise_shaping_vbox), 5);
	gtk_container_add(GTK_CONTAINER(resolution_replaygain_noise_shaping_frame), resolution_replaygain_noise_shaping_vbox);

	resolution_replaygain_noise_shaping_radio_none = gtk_radio_button_new_with_label(NULL, _("none"));
	if(flac_cfg.output.resolution.replaygain.noise_shaping == 0)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(resolution_replaygain_noise_shaping_radio_none), TRUE);
	gtk_signal_connect(GTK_OBJECT(resolution_replaygain_noise_shaping_radio_none), "clicked", resolution_replaygain_noise_shaping_cb, NULL);
	gtk_container_add(GTK_CONTAINER(resolution_replaygain_noise_shaping_vbox), resolution_replaygain_noise_shaping_radio_none);

	resolution_replaygain_noise_shaping_radio_low = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(resolution_replaygain_noise_shaping_radio_none), _("low"));
	if(flac_cfg.output.resolution.replaygain.noise_shaping == 1)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(resolution_replaygain_noise_shaping_radio_low), TRUE);
	gtk_signal_connect(GTK_OBJECT(resolution_replaygain_noise_shaping_radio_low), "clicked", resolution_replaygain_noise_shaping_cb, NULL);
	gtk_container_add(GTK_CONTAINER(resolution_replaygain_noise_shaping_vbox), resolution_replaygain_noise_shaping_radio_low);

	resolution_replaygain_noise_shaping_radio_medium = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(resolution_replaygain_noise_shaping_radio_none), _("medium"));
	if(flac_cfg.output.resolution.replaygain.noise_shaping == 2)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(resolution_replaygain_noise_shaping_radio_medium), TRUE);
	gtk_signal_connect(GTK_OBJECT(resolution_replaygain_noise_shaping_radio_medium), "clicked", resolution_replaygain_noise_shaping_cb, NULL);
	gtk_container_add(GTK_CONTAINER(resolution_replaygain_noise_shaping_vbox), resolution_replaygain_noise_shaping_radio_medium);

	resolution_replaygain_noise_shaping_radio_high = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(resolution_replaygain_noise_shaping_radio_none), _("high"));
	if(flac_cfg.output.resolution.replaygain.noise_shaping == 3)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(resolution_replaygain_noise_shaping_radio_high), TRUE);
	gtk_signal_connect(GTK_OBJECT(resolution_replaygain_noise_shaping_radio_high), "clicked", resolution_replaygain_noise_shaping_cb, NULL);
	gtk_container_add(GTK_CONTAINER(resolution_replaygain_noise_shaping_vbox), resolution_replaygain_noise_shaping_radio_high);

	resolution_replaygain_bps_out_frame = gtk_frame_new(_("Dither to"));
	gtk_container_border_width(GTK_CONTAINER(resolution_replaygain_bps_out_frame), 5);
	gtk_box_pack_start(GTK_BOX(hbox), resolution_replaygain_bps_out_frame, FALSE, FALSE, 0);

	resolution_replaygain_bps_out_vbox = gtk_vbutton_box_new();
	gtk_container_border_width(GTK_CONTAINER(resolution_replaygain_bps_out_vbox), 0);
	gtk_container_add(GTK_CONTAINER(resolution_replaygain_bps_out_frame), resolution_replaygain_bps_out_vbox);

	resolution_replaygain_bps_out_radio_16bps = gtk_radio_button_new_with_label(NULL, _("16 bps"));
	if(flac_cfg.output.resolution.replaygain.bps_out == 16)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(resolution_replaygain_bps_out_radio_16bps), TRUE);
	gtk_signal_connect(GTK_OBJECT(resolution_replaygain_bps_out_radio_16bps), "clicked", resolution_replaygain_bps_out_cb, NULL);
	gtk_container_add(GTK_CONTAINER(resolution_replaygain_bps_out_vbox), resolution_replaygain_bps_out_radio_16bps);

	resolution_replaygain_bps_out_radio_24bps = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(resolution_replaygain_bps_out_radio_16bps), _("24 bps"));
	if(flac_cfg.output.resolution.replaygain.bps_out == 24)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(resolution_replaygain_bps_out_radio_24bps), TRUE);
	gtk_signal_connect(GTK_OBJECT(resolution_replaygain_bps_out_radio_24bps), "clicked", resolution_replaygain_bps_out_cb, NULL);
	gtk_container_add(GTK_CONTAINER(resolution_replaygain_bps_out_vbox), resolution_replaygain_bps_out_radio_24bps);

	resolution_replaygain_dither_cb(resolution_replaygain_dither, NULL);

	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), output_vbox, gtk_label_new(_("Output")));

	/* Streaming */

	streaming_vbox = gtk_vbox_new(FALSE, 0);

	streaming_buf_frame = gtk_frame_new(_("Buffering:"));
	gtk_container_set_border_width(GTK_CONTAINER(streaming_buf_frame), 5);
	gtk_box_pack_start(GTK_BOX(streaming_vbox), streaming_buf_frame, FALSE, FALSE, 0);

	streaming_buf_hbox = gtk_hbox_new(TRUE, 5);
	gtk_container_set_border_width(GTK_CONTAINER(streaming_buf_hbox), 5);
	gtk_container_add(GTK_CONTAINER(streaming_buf_frame), streaming_buf_hbox);

	streaming_size_box = gtk_hbox_new(FALSE, 5);
	/*gtk_table_attach_defaults(GTK_TABLE(streaming_buf_table),streaming_size_box,0,1,0,1); */
	gtk_box_pack_start(GTK_BOX(streaming_buf_hbox), streaming_size_box, TRUE, TRUE, 0);
	streaming_size_label = gtk_label_new(_("Buffer size (kb):"));
	gtk_box_pack_start(GTK_BOX(streaming_size_box), streaming_size_label, FALSE, FALSE, 0);
	streaming_size_adj = gtk_adjustment_new(flac_cfg.stream.http_buffer_size, 4, 4096, 4, 4, 4);
	streaming_size_spin = gtk_spin_button_new(GTK_ADJUSTMENT(streaming_size_adj), 8, 0);
	gtk_widget_set_usize(streaming_size_spin, 60, -1);
	gtk_box_pack_start(GTK_BOX(streaming_size_box), streaming_size_spin, FALSE, FALSE, 0);

	streaming_pre_box = gtk_hbox_new(FALSE, 5);
	/*gtk_table_attach_defaults(GTK_TABLE(streaming_buf_table),streaming_pre_box,1,2,0,1); */
	gtk_box_pack_start(GTK_BOX(streaming_buf_hbox), streaming_pre_box, TRUE, TRUE, 0);
	streaming_pre_label = gtk_label_new(_("Pre-buffer (percent):"));
	gtk_box_pack_start(GTK_BOX(streaming_pre_box), streaming_pre_label, FALSE, FALSE, 0);
	streaming_pre_adj = gtk_adjustment_new(flac_cfg.stream.http_prebuffer, 0, 90, 1, 1, 1);
	streaming_pre_spin = gtk_spin_button_new(GTK_ADJUSTMENT(streaming_pre_adj), 1, 0);
	gtk_widget_set_usize(streaming_pre_spin, 60, -1);
	gtk_box_pack_start(GTK_BOX(streaming_pre_box), streaming_pre_spin, FALSE, FALSE, 0);

 	/*
 	 * Proxy config.
 	 */
	streaming_proxy_frame = gtk_frame_new(_("Proxy:"));
	gtk_container_set_border_width(GTK_CONTAINER(streaming_proxy_frame), 5);
	gtk_box_pack_start(GTK_BOX(streaming_vbox), streaming_proxy_frame, FALSE, FALSE, 0);

	streaming_proxy_vbox = gtk_vbox_new(FALSE, 5);
	gtk_container_set_border_width(GTK_CONTAINER(streaming_proxy_vbox), 5);
	gtk_container_add(GTK_CONTAINER(streaming_proxy_frame), streaming_proxy_vbox);

	streaming_proxy_use = gtk_check_button_new_with_label(_("Use proxy"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(streaming_proxy_use), flac_cfg.stream.use_proxy);
	gtk_signal_connect(GTK_OBJECT(streaming_proxy_use), "clicked", GTK_SIGNAL_FUNC(proxy_use_cb), NULL);
	gtk_box_pack_start(GTK_BOX(streaming_proxy_vbox), streaming_proxy_use, FALSE, FALSE, 0);

	streaming_proxy_hbox = gtk_hbox_new(FALSE, 5);
	gtk_widget_set_sensitive(streaming_proxy_hbox, flac_cfg.stream.use_proxy);
	gtk_box_pack_start(GTK_BOX(streaming_proxy_vbox), streaming_proxy_hbox, FALSE, FALSE, 0);

	streaming_proxy_host_label = gtk_label_new(_("Host:"));
	gtk_box_pack_start(GTK_BOX(streaming_proxy_hbox), streaming_proxy_host_label, FALSE, FALSE, 0);

	streaming_proxy_host_entry = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(streaming_proxy_host_entry), flac_cfg.stream.proxy_host? flac_cfg.stream.proxy_host : "");
	gtk_box_pack_start(GTK_BOX(streaming_proxy_hbox), streaming_proxy_host_entry, TRUE, TRUE, 0);

	streaming_proxy_port_label = gtk_label_new(_("Port:"));
	gtk_box_pack_start(GTK_BOX(streaming_proxy_hbox), streaming_proxy_port_label, FALSE, FALSE, 0);

	streaming_proxy_port_entry = gtk_entry_new();
	gtk_widget_set_usize(streaming_proxy_port_entry, 50, -1);
	temp = g_strdup_printf("%d", flac_cfg.stream.proxy_port);
	gtk_entry_set_text(GTK_ENTRY(streaming_proxy_port_entry), temp);
	g_free(temp);
	gtk_box_pack_start(GTK_BOX(streaming_proxy_hbox), streaming_proxy_port_entry, FALSE, FALSE, 0);

	streaming_proxy_auth_use = gtk_check_button_new_with_label(_("Use authentication"));
	gtk_widget_set_sensitive(streaming_proxy_auth_use, flac_cfg.stream.use_proxy);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(streaming_proxy_auth_use), flac_cfg.stream.proxy_use_auth);
	gtk_signal_connect(GTK_OBJECT(streaming_proxy_auth_use), "clicked", GTK_SIGNAL_FUNC(proxy_auth_use_cb), NULL);
	gtk_box_pack_start(GTK_BOX(streaming_proxy_vbox), streaming_proxy_auth_use, FALSE, FALSE, 0);

	streaming_proxy_auth_hbox = gtk_hbox_new(FALSE, 5);
	gtk_widget_set_sensitive(streaming_proxy_auth_hbox, flac_cfg.stream.use_proxy && flac_cfg.stream.proxy_use_auth);
	gtk_box_pack_start(GTK_BOX(streaming_proxy_vbox), streaming_proxy_auth_hbox, FALSE, FALSE, 0);

	streaming_proxy_auth_user_label = gtk_label_new(_("Username:"));
	gtk_box_pack_start(GTK_BOX(streaming_proxy_auth_hbox), streaming_proxy_auth_user_label, FALSE, FALSE, 0);

	streaming_proxy_auth_user_entry = gtk_entry_new();
	if(flac_cfg.stream.proxy_user)
		gtk_entry_set_text(GTK_ENTRY(streaming_proxy_auth_user_entry), flac_cfg.stream.proxy_user);
	gtk_box_pack_start(GTK_BOX(streaming_proxy_auth_hbox), streaming_proxy_auth_user_entry, TRUE, TRUE, 0);

	streaming_proxy_auth_pass_label = gtk_label_new(_("Password:"));
	gtk_box_pack_start(GTK_BOX(streaming_proxy_auth_hbox), streaming_proxy_auth_pass_label, FALSE, FALSE, 0);

	streaming_proxy_auth_pass_entry = gtk_entry_new();
	if(flac_cfg.stream.proxy_pass)
		gtk_entry_set_text(GTK_ENTRY(streaming_proxy_auth_pass_entry), flac_cfg.stream.proxy_pass);
	gtk_entry_set_visibility(GTK_ENTRY(streaming_proxy_auth_pass_entry), FALSE);
	gtk_box_pack_start(GTK_BOX(streaming_proxy_auth_hbox), streaming_proxy_auth_pass_entry, TRUE, TRUE, 0);


	/*
	 * Save to disk config.
	 */
	streaming_save_frame = gtk_frame_new(_("Save stream to disk:"));
	gtk_container_set_border_width(GTK_CONTAINER(streaming_save_frame), 5);
	gtk_box_pack_start(GTK_BOX(streaming_vbox), streaming_save_frame, FALSE, FALSE, 0);

	streaming_save_vbox = gtk_vbox_new(FALSE, 5);
	gtk_container_set_border_width(GTK_CONTAINER(streaming_save_vbox), 5);
	gtk_container_add(GTK_CONTAINER(streaming_save_frame), streaming_save_vbox);

	streaming_save_use = gtk_check_button_new_with_label(_("Save stream to disk"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(streaming_save_use), flac_cfg.stream.save_http_stream);
	gtk_signal_connect(GTK_OBJECT(streaming_save_use), "clicked", GTK_SIGNAL_FUNC(streaming_save_use_cb), NULL);
	gtk_box_pack_start(GTK_BOX(streaming_save_vbox), streaming_save_use, FALSE, FALSE, 0);

	streaming_save_hbox = gtk_hbox_new(FALSE, 5);
	gtk_widget_set_sensitive(streaming_save_hbox, flac_cfg.stream.save_http_stream);
	gtk_box_pack_start(GTK_BOX(streaming_save_vbox), streaming_save_hbox, FALSE, FALSE, 0);

	streaming_save_label = gtk_label_new(_("Path:"));
	gtk_box_pack_start(GTK_BOX(streaming_save_hbox), streaming_save_label, FALSE, FALSE, 0);

	streaming_save_entry = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(streaming_save_entry), flac_cfg.stream.save_http_path? flac_cfg.stream.save_http_path : "");
	gtk_box_pack_start(GTK_BOX(streaming_save_hbox), streaming_save_entry, TRUE, TRUE, 0);

	streaming_save_browse = gtk_button_new_with_label(_("Browse"));
	gtk_signal_connect(GTK_OBJECT(streaming_save_browse), "clicked", GTK_SIGNAL_FUNC(streaming_save_browse_cb), NULL);
	gtk_box_pack_start(GTK_BOX(streaming_save_hbox), streaming_save_browse, FALSE, FALSE, 0);

#ifdef FLAC_ICECAST
	streaming_cast_frame = gtk_frame_new(_("SHOUT/Icecast:"));
	gtk_container_set_border_width(GTK_CONTAINER(streaming_cast_frame), 5);
	gtk_box_pack_start(GTK_BOX(streaming_vbox), streaming_cast_frame, FALSE, FALSE, 0);

	streaming_cast_vbox = gtk_vbox_new(5, FALSE);
	gtk_container_add(GTK_CONTAINER(streaming_cast_frame), streaming_cast_vbox);

	streaming_cast_title = gtk_check_button_new_with_label(_("Enable SHOUT/Icecast title streaming"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(streaming_cast_title), flac_cfg.stream.cast_title_streaming);
	gtk_box_pack_start(GTK_BOX(streaming_cast_vbox), streaming_cast_title, FALSE, FALSE, 0);

	streaming_udp_title = gtk_check_button_new_with_label(_("Enable Icecast Metadata UDP Channel"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(streaming_udp_title), flac_cfg.stream.use_udp_channel);
	gtk_box_pack_start(GTK_BOX(streaming_cast_vbox), streaming_udp_title, FALSE, FALSE, 0);
#endif

	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), streaming_vbox, gtk_label_new(_("Streaming")));

	/* Buttons */

	bbox = gtk_hbutton_box_new();
	gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_END);
	gtk_button_box_set_spacing(GTK_BUTTON_BOX(bbox), 5);
	gtk_box_pack_start(GTK_BOX(vbox), bbox, FALSE, FALSE, 0);

	ok = gtk_button_new_with_label(_("Ok"));
	gtk_signal_connect(GTK_OBJECT(ok), "clicked", GTK_SIGNAL_FUNC(flac_configurewin_ok), NULL);
	GTK_WIDGET_SET_FLAGS(ok, GTK_CAN_DEFAULT);
	gtk_box_pack_start(GTK_BOX(bbox), ok, TRUE, TRUE, 0);
	gtk_widget_grab_default(ok);

	cancel = gtk_button_new_with_label(_("Cancel"));
	gtk_signal_connect_object(GTK_OBJECT(cancel), "clicked", GTK_SIGNAL_FUNC(gtk_widget_destroy), GTK_OBJECT(flac_configurewin));
	GTK_WIDGET_SET_FLAGS(cancel, GTK_CAN_DEFAULT);
	gtk_box_pack_start(GTK_BOX(bbox), cancel, TRUE, TRUE, 0);

	gtk_widget_show_all(flac_configurewin);
}

void FLAC_XMMS__aboutbox(void)
{
	static GtkWidget *about_window;

	if (about_window)
		gdk_window_raise(about_window->window);

	about_window = xmms_show_message(
		_("About Flac Plugin"),
		_("Flac Plugin by Josh Coalson\n"
		  "contributions by\n"
		  "......\n"
		  "......\n"
		  "and\n"
		  "Daisuke Shimamura\n"
		  "Visit http://flac.sourceforge.net/"),
		_("Ok"), FALSE, NULL, NULL);
	gtk_signal_connect(GTK_OBJECT(about_window), "destroy",
			   GTK_SIGNAL_FUNC(gtk_widget_destroyed),
			   &about_window);
}

/*
 * Get text of an Entry or a ComboBox
 */
static gchar *gtk_entry_get_text_1 (GtkWidget *widget)
{
	if (GTK_IS_COMBO(widget))
	{
		return gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(widget)->entry));
	}else if (GTK_IS_ENTRY(widget))
	{
		return gtk_entry_get_text(GTK_ENTRY(widget));
	}else
	{
		return NULL;
	}
}
