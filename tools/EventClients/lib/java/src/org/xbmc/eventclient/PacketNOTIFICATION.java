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
