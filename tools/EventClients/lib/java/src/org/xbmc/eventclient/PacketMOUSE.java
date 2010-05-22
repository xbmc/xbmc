/*
 *  Copyright (C) 2008-2009 Team XBMC http://www.xbmc.org
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

package org.xbmc.eventclient;
/**
 * XBMC Event Client Class
 * 
 * A MOUSE packets sets the mouse position in XBMC
 * @author Stefan Agner
 *
 */
public class PacketMOUSE extends Packet {
	
	protected final static byte MS_ABSOLUTE = 0x01;

	/**
	 * A MOUSE packets sets the mouse position in XBMC
	 * @param x horitontal position ranging from 0 to 65535
	 * @param y vertical position ranging from 0 to 65535
	 */
	public PacketMOUSE(int x, int y)
	{
		super(PT_MOUSE);
		byte flags = 0;
		flags |= MS_ABSOLUTE;
		appendPayload(flags);
		appendPayload((short)x);
		appendPayload((short)y);
	}
}
