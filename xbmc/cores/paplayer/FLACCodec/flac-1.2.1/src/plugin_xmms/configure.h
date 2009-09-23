/* libxmms-flac - XMMS FLAC input plugin
 * Copyright (C) 2002,2003,2004,2005,2006  Daisuke Shimamura
 *
 * Based on mpg123 plugin
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

#ifndef FLAC__PLUGIN_XMMS__CONFIGURE_H
#define FLAC__PLUGIN_XMMS__CONFIGURE_H

#include <glib.h>

typedef struct {
	struct {
		gboolean tag_override;
		gchar *tag_format;
		gboolean convert_char_set;
		gchar *user_char_set;
	} title;

	struct {
		gint http_buffer_size;
		gint http_prebuffer;
		gboolean use_proxy; 
		gchar *proxy_host;
		gint proxy_port;
		gboolean proxy_use_auth;
		gchar *proxy_user;
		gchar *proxy_pass;
		gboolean save_http_stream;
		gchar *save_http_path;
		gboolean cast_title_streaming;
		gboolean use_udp_channel;
	} stream;

	struct {
		struct {
			gboolean enable;
			gboolean album_mode;
			gint preamp;
			gboolean hard_limit;
		} replaygain;
		struct {
			struct {
				gboolean dither_24_to_16;
			} normal;
			struct {
				gboolean dither;
				gint noise_shaping; /* value must be one of NoiseShaping enum, c.f. plugin_common/replaygain_synthesis.h */
				gint bps_out;
			} replaygain;
		} resolution;
	} output;
} flac_config_t;

extern flac_config_t flac_cfg;

extern void FLAC_XMMS__configure(void);
extern void FLAC_XMMS__aboutbox(void);

#endif



