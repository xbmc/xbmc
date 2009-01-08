package org.xbmc.eventclient;
/**
 * XBMC Event Client Class
 * 
 * A PING packet tells XBMC that the client is still alive. All valid
 * packets act as ping (not just this one). A client needs to ping
 * XBMC at least once in 60 seconds or it will time
 * @author Stefan Agner
 *
 */
public class PacketPING extends Packet {
	/**
	 * A PING packet tells XBMC that the client is still alive.
	 */
	public PacketPING()
	{
		super(PT_PING);
	}
}
