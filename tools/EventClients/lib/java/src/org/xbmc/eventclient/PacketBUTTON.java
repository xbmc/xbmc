/*
 *  Copyright (C) 2008-2009 Team XBMC http://www.xbmc.org
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
 * A button packet send a key press or release event to XBMC
 * @author Stefan Agner
 *
 */
public class PacketBUTTON extends Packet {
	
	protected final static byte BT_USE_NAME   = 0x01;
	protected final static byte BT_DOWN       = 0x02;
	protected final static byte BT_UP         = 0x04;
	protected final static byte BT_USE_AMOUNT = 0x08;
	protected final static byte BT_QUEUE      = 0x10;
	protected final static byte BT_NO_REPEAT  = 0x20;
	protected final static byte BT_VKEY       = 0x40;
	protected final static byte BT_AXIS       = (byte)0x80;
	protected final static byte BT_AXISSINGLE = (byte)0x100;

	/**
	 * A button packet send a key press or release event to XBMC
	 * @param code raw button code (default: 0)
	 * @param repeat this key press should repeat until released (default: 1)
     * Note that queued pressed cannot repeat.
	 * @param down if this is 1, it implies a press event, 0 implies a release
     * event. (default: 1)
	 * @param queue a queued key press means that the button event is
     * executed just once after which the next key press is processed. 
     * It can be used for macros. Currently there is no support for
     * time delays between queued presses. (default: 0)
	 * @param amount unimplemented for now; in the future it will be used for
     * specifying magnitude of analog key press events
	 * @param axis 
	 */
	public PacketBUTTON(short code, boolean repeat, boolean down, boolean queue, short amount, byte axis)
	{
		super(PT_BUTTON);
		String map_name = "";
		String button_name = "";
		short flags = 0;
		appendPayload(code, map_name, button_name, repeat, down, queue, amount, axis, flags);
	}
	
	/**
	 * A button packet send a key press or release event to XBMC
	 * @param map_name a combination of map_name and button_name refers to a
     * mapping in the user's Keymap.xml or Lircmap.xml.
     * map_name can be one of the following:
     * <ul>
     * <li>"KB" => standard keyboard map ( <keyboard> section )</li>
     * <li>"XG" => xbox gamepad map ( <gamepad> section )</li>
     * <li>"R1" => xbox remote map ( <remote> section )</li>
     * <li>"R2" => xbox universal remote map ( <universalremote> section )</li>
     * <li>"LI:devicename" => LIRC remote map where 'devicename' is the
     * actual device's name</li></ul>
	 * @param button_name a button name defined in the map specified in map_name.
     * For example, if map_name is "KB" refering to the <keyboard> section in Keymap.xml 
     * then, valid button_names include "printscreen", "minus", "x", etc.
	 * @param repeat this key press should repeat until released (default: 1)
     * Note that queued pressed cannot repeat.
	 * @param down if this is 1, it implies a press event, 0 implies a release
     * event. (default: 1)
	 * @param queue a queued key press means that the button event is
     * executed just once after which the next key press is processed. 
     * It can be used for macros. Currently there is no support for
     * time delays between queued presses. (default: 0)
	 * @param amount unimplemented for now; in the future it will be used for
     * specifying magnitude of analog key press events
	 * @param axis 
	 */
	public PacketBUTTON(String map_name, String button_name, boolean repeat, boolean down, boolean queue, short amount, byte axis)
	{
		super(PT_BUTTON);
		short code = 0;
		short flags = BT_USE_NAME;
		appendPayload(code, map_name, button_name, repeat, down, queue, amount, axis, flags);
	}
	
	/**
	 * Appends Payload for a Button Packet (this method is used by the different Constructors of this Packet)
	 * @param code raw button code (default: 0)
	 * @param map_name a combination of map_name and button_name refers to a
     * mapping in the user's Keymap.xml or Lircmap.xml.
     * map_name can be one of the following:
     * <ul>
     * <li>"KB" => standard keyboard map ( <keyboard> section )</li>
     * <li>"XG" => xbox gamepad map ( <gamepad> section )</li>
     * <li>"R1" => xbox remote map ( <remote> section )</li>
     * <li>"R2" => xbox universal remote map ( <universalremote> section )</li>
     * <li>"LI:devicename" => LIRC remote map where 'devicename' is the
     * actual device's name</li></ul>
	 * @param button_name a button name defined in the map specified in map_name.
     * For example, if map_name is "KB" refering to the <keyboard> section in Keymap.xml 
     * then, valid button_names include "printscreen", "minus", "x", etc.
	 * @param repeat this key press should repeat until released (default: 1)
     * Note that queued pressed cannot repeat.
	 * @param down if this is 1, it implies a press event, 0 implies a release
     * event. (default: 1)
	 * @param queue a queued key press means that the button event is
     * executed just once after which the next key press is processed. 
     * It can be used for macros. Currently there is no support for
     * time delays between queued presses. (default: 0)
	 * @param amount unimplemented for now; in the future it will be used for
     * specifying magnitude of analog key press events
	 * @param axis 
	 * @param flags Packet specific flags
	 */
	private void appendPayload(short code, String map_name, String button_name, boolean repeat, boolean down, boolean queue, short amount, byte axis, short flags)
	{
		if(amount>0)
			flags |= BT_USE_AMOUNT;
		else
			amount = 0;
		
        if(down)
            flags |= BT_DOWN;
        else
            flags |= BT_UP;
        
        if(!repeat)
            flags |= BT_NO_REPEAT;
        
        if(queue)
            flags |= BT_QUEUE;
        
        if(axis == 1)
            flags |= BT_AXISSINGLE;
        else if (axis == 2)
            flags |= BT_AXIS;

		
		appendPayload(code);
		appendPayload(flags);
		appendPayload(amount);
		appendPayload(map_name);
		appendPayload(button_name);
	}
}
