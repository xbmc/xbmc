package org.xbmc.eventclient;
/**
 * XBMC Event Client Class
 * 
 * A BYE packet terminates the connection to XBMC.
 * @author Stefan Agner
 *
 */
public class PacketBYE extends Packet 
{

	/**
	 * A BYE packet terminates the connection to XBMC.
	 */
	public PacketBYE()
	{
		super(PT_BYE);
	}
}
