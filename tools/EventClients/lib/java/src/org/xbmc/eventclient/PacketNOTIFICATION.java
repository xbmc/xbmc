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
 * This packet displays a notification window in XBMC. It can contain
 * a caption, a message and an icon.
 * @author Stefan Agner
 *
 */
public class PacketNOTIFICATION extends Packet {

	/**
	 * This packet displays a notification window in XBMC.
	 * @param title Message title
	 * @param message The actual message
	 * @param iconType Type of the icon (Packet.ICON_PNG, Packet.ICON_JPEG or Packet.ICON_GIF)
	 * @param iconData The icon as a Byte-Array
	 */
	public PacketNOTIFICATION(String title, String message, byte iconType, byte[] iconData)
	{
		super(PT_NOTIFICATION);
		appendPayload(title, message, iconType, iconData);
	}

	/**
	 * This packet displays a notification window in XBMC.
	 * @param title Message title
	 * @param message The actual message
	 */
	public PacketNOTIFICATION(String title, String message)
	{
		super(PT_NOTIFICATION);
		appendPayload(title, message, Packet.ICON_NONE, null);
	}

	/**
	 * Appends the payload to the packet...
	 * @param title Message title
	 * @param message The actual message
	 * @param iconType Type of the icon (Packet.ICON_PNG, Packet.ICON_JPEG or Packet.ICON_GIF)
	 * @param iconData The icon as a Byte-Array
	 */
	private void appendPayload(String title, String message, byte iconType, byte[] iconData)
	{
		appendPayload(title);
		appendPayload(message);
		appendPayload(iconType);
		appendPayload(0); // reserved
		if(iconData!=null)
			appendPayload(iconData);

	}
}
