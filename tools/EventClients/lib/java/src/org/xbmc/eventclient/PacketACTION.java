/*
 *  Copyright (C) 2008-2013 Team XBMC
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
 * An ACTION packet tells XBMC to do the action specified, based on the type it knows were it needs to be sent.
 * The idea is that this will be as in scripting/skining and keymapping, just triggered from afar.
 * @author Stefan Agner
 *
 */
public class PacketACTION extends Packet {

	public final static byte ACTION_EXECBUILTIN = 0x01;
	public final static byte ACTION_BUTTON      = 0x02;


	/**
	 * An ACTION packet tells XBMC to do the action specified, based on the type it knows were it needs to be sent.
	 * @param actionmessage Actionmessage (as in scripting/skinning)
	 */
	public PacketACTION(String actionmessage)
	{
		super(PT_ACTION);
		byte actiontype = ACTION_EXECBUILTIN;
		appendPayload(actionmessage, actiontype);
	}

	/**
	 * An ACTION packet tells XBMC to do the action specified, based on the type it knows were it needs to be sent.
	 * @param actionmessage Actionmessage (as in scripting/skinning)
	 * @param actiontype Actiontype (ACTION_EXECBUILTIN or ACTION_BUTTON)
	 */
	public PacketACTION(String actionmessage, byte actiontype)
	{
		super(PT_ACTION);
		appendPayload(actionmessage, actiontype);
	}

	private void appendPayload(String actionmessage, byte actiontype)
	{
		appendPayload(actiontype);
		appendPayload(actionmessage);
	}
}
