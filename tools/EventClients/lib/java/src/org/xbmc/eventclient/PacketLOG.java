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
