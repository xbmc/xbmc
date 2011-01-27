/*
 * ASAP_ModuleInfo.java - file information
 *
 * Copyright (C) 2007-2009  Piotr Fusik
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

package net.sf.asap;

/** Information about a file recognized by ASAP. */
public class ASAP_ModuleInfo
{
	/** Music author's name. */
	public String author;
	/** Music title. */
	public String name;
	/** Music creation date. */
	public String date;
	/** 1 for mono or 2 for stereo. */
	public int channels;
	/** Number of subsongs. */
	public int songs;
	/** 0-based index of the "main" subsong. */
	public int default_song;
	/** Lengths of songs, in milliseconds, -1 = unspecified. */
	public final int[] durations = new int[32];
	/** Whether songs repeat or not. */
	public final boolean[] loops = new boolean[32];

	int type;
	int fastplay;
	int music;
	int init;
	int player;
	int covox_addr;
	int header_len;
	final byte[] song_pos = new byte[128];
}
