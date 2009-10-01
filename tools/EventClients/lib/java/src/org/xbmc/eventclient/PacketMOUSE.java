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
