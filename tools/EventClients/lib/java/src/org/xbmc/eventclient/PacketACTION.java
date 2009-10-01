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
