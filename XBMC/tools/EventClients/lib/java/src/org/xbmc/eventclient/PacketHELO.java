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
