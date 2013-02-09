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
 * A LOG packet tells XBMC to log the message to xbmc.log with the loglevel as specified.
 * @author Stefan Agner
 *
 */
public class PacketLOG extends Packet {
	
	/**
	 * A LOG packet tells XBMC to log the message to xbmc.log with the loglevel as specified.
	 * @param loglevel the loglevel, follows XBMC standard.
	 * <ul>
	 * <li>0 = DEBUG</li>
	 * <li>1 = INFO</li>
	 * <li>2 = NOTICE</li>
	 * <li>3 = WARNING</li>
	 * <li>4 = ERROR</li>
	 * <li>5 = SEVERE</li>
	 * </ul>
	 * @param logmessage the message to log
	 */
	public PacketLOG(byte loglevel, String logmessage)
	{
		super(PT_LOG);
		appendPayload(loglevel);
		appendPayload(logmessage);
	}
}
