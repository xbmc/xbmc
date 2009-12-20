/*
 * ASAP_ModuleInfo.as - file information
 *
 * Copyright (C) 2009  Piotr Fusik
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

package
{
	public class ASAP_ModuleInfo
	{
		public var author : String;
		public var name : String;
		public var date : String;
		public var channels : int;
		public var songs : int;
		public var default_song : int;
		public const durations : Array = new Array(32);
		public const loops : Array = new Array(32);
		internal var type : int;
		internal var fastplay : int;
		internal var music : int;
		internal var init : int;
		internal var player : int;
		internal var covox_addr : int;
		internal var header_len : int;
		internal const song_pos : Array = new Array(128);
	}
}
