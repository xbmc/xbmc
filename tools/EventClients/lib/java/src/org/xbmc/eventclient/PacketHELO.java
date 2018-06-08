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
import java.nio.charset.Charset;

/**
 * XBMC Event Client Class
 *
 * A HELO packet establishes a valid connection to XBMC. It is the
 * first packet that should be sent.
 * @author Stefan Agner
 *
 */
public class PacketHELO extends Packet {


	/**
	 * A HELO packet establishes a valid connection to XBMC.
	 * @param devicename Name of the device which connects to XBMC
	 */
	public PacketHELO(String devicename)
	{
		super(PT_HELO);
		this.appendPayload(devicename);
		this.appendPayload(ICON_NONE);
		this.appendPayload((short)0); // port no
		this.appendPayload(0); // reserved1
		this.appendPayload(0); // reserved2
	}

	/**
	 * A HELO packet establishes a valid connection to XBMC.
	 * @param devicename Name of the device which connects to XBMC
	 * @param iconType Type of the icon (Packet.ICON_PNG, Packet.ICON_JPEG or Packet.ICON_GIF)
	 * @param iconData The icon as a Byte-Array
	 */
	public PacketHELO(String devicename, byte iconType, byte[] iconData)
	{
		super(PT_HELO);
		this.appendPayload(devicename);
		this.appendPayload(iconType);
		this.appendPayload((short)0); // port no
		this.appendPayload(0); // reserved1
		this.appendPayload(0); // reserved2
		this.appendPayload(iconData); // reserved2
	}

}
