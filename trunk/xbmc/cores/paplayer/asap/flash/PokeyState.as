/*
 * PokeyState.as - state of sound part of the POKEY chip
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
	internal class PokeyState
	{
		internal var audctl : int;
		internal var init : Boolean;
		internal var poly_index : int;
		internal var div_cycles : int;
		internal var mute1 : int;
		internal var mute2 : int;
		internal var mute3 : int;
		internal var mute4 : int;
		internal var audf1 : int;
		internal var audf2 : int;
		internal var audf3 : int;
		internal var audf4 : int;
		internal var audc1 : int;
		internal var audc2 : int;
		internal var audc3 : int;
		internal var audc4 : int;
		internal var tick_cycle1 : int;
		internal var tick_cycle2 : int;
		internal var tick_cycle3 : int;
		internal var tick_cycle4 : int;
		internal var period_cycles1 : int;
		internal var period_cycles2 : int;
		internal var period_cycles3 : int;
		internal var period_cycles4 : int;
		internal var reload_cycles1 : int;
		internal var reload_cycles3 : int;
		internal var out1 : int;
		internal var out2 : int;
		internal var out3 : int;
		internal var out4 : int;
		internal var delta1 : int;
		internal var delta2 : int;
		internal var delta3 : int;
		internal var delta4 : int;
		internal var skctl : int;
		internal const delta_buffer : Array = new Array(888);
	}
}
