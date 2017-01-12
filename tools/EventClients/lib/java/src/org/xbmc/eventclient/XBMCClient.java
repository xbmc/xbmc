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

import java.io.FileInputStream;
import java.io.IOException;
import java.net.InetAddress;

/**
 * XBMC Event Client Class
 * 
 * Implements an XBMC-Client. This class can be used to implement your own application which
 * should act as a Input device for XBMC. Also starts a Ping-Thread, which tells the XBMC EventServer
 * that the client is alive. Therefore if you close your application you SHOULD call stopClient()! 
 * @author Stefan Agner
 *
 */
public class XBMCClient 
{
	private boolean hasIcon = false;
	private String deviceName;
	private PingThread oPingThread;
	private byte iconType = Packet.ICON_PNG;
	private byte[] iconData;
	private InetAddress hostAddress;
	private int hostPort;
	
	/**
	 * Starts a XBMC EventClient. 
	 * @param hostAddress Address of the Host running XBMC
	 * @param hostPort Port of the Host running XBMC (default 9777)
	 * @param deviceName Name of the Device
	 * @param iconFile Path to the Iconfile (PNG, JPEG or GIF)
	 * @throws IOException
	 */
	public XBMCClient(InetAddress hostAddress, int hostPort, String deviceName, String iconFile) throws IOException
	{
		byte iconType = Packet.ICON_PNG;
		// Assume png as icon type
		if(iconFile.toLowerCase().endsWith(".jpeg"))
			iconType = Packet.ICON_JPEG;
		if(iconFile.toLowerCase().endsWith(".jpg"))
			iconType = Packet.ICON_JPEG;
		if(iconFile.toLowerCase().endsWith(".gif"))
			iconType = Packet.ICON_GIF;

		// Read the icon file to the byte array...
		FileInputStream iconFileStream = new FileInputStream(iconFile);
		byte[] iconData = new byte[iconFileStream.available()];
		iconFileStream.read(iconData);
		
		hasIcon = true;
		
		// Call start-Method...
		startClient(hostAddress, hostPort, deviceName, iconType, iconData);
	}
	

	/**
	 * Starts a XBMC EventClient. 
	 * @param hostAddress Address of the Host running XBMC
	 * @param hostPort Port of the Host running XBMC (default 9777)
	 * @param deviceName Name of the Device
	 * @param iconType Type of the icon file (see Packet.ICON_PNG, Packet.ICON_JPEG or Packet.ICON_GIF)
	 * @param iconData The icon itself as a Byte-Array 
	 * @throws IOException
	 */
	public XBMCClient(InetAddress hostAddress, int hostPort, String deviceName, byte iconType, byte[] iconData) throws IOException
	{
		hasIcon = true;
		startClient(hostAddress, hostPort, deviceName, iconType, iconData);
	}
	
	/**
	 * Starts a XBMC EventClient without an icon. 
	 * @param hostAddress Address of the Host running XBMC
	 * @param hostPort Port of the Host running XBMC (default 9777)
	 * @param deviceName Name of the Device
	 * @throws IOException
	 */
	public XBMCClient(InetAddress hostAddress, int hostPort, String deviceName) throws IOException
	{
		hasIcon = false;
		byte iconType = Packet.ICON_NONE;
		byte[] iconData = null;
		startClient(hostAddress, hostPort, deviceName, iconType, iconData);
	}


	/**
	 * Starts a XBMC EventClient. 
	 * @param hostAddress Address of the Host running XBMC
	 * @param hostPort Port of the Host running XBMC (default 9777)
	 * @param deviceName Name of the Device
	 * @param iconType Type of the icon file (see Packet.ICON_PNG, Packet.ICON_JPEG or Packet.ICON_GIF)
	 * @param iconData The icon itself as a Byte-Array 
	 * @throws IOException
	 */
	private void startClient(InetAddress hostAddress, int hostPort, String deviceName, byte iconType, byte[] iconData) throws IOException
	{
		// Save host address and port
		this.hostAddress = hostAddress;
		this.hostPort = hostPort;
		this.deviceName = deviceName;
		
		this.iconType = iconType;
		this.iconData = iconData;

		// Send Hello Packet...
		PacketHELO p;
		if(hasIcon)
			p = new PacketHELO(deviceName, iconType, iconData);
		else
			p = new PacketHELO(deviceName);
		
		p.send(hostAddress, hostPort);
		
		// Start Thread (for Ping packets...)
		oPingThread = new PingThread(hostAddress, hostPort, 20000);
		oPingThread.start();
	}
	
	/**
	 * Stops the XBMC EventClient (especially the Ping-Thread)
	 * @throws IOException
	 */
	public void stopClient() throws IOException
	{
		// Stop Ping-Thread...
		oPingThread.giveup();
		oPingThread.interrupt();
		
		PacketBYE p = new PacketBYE();
		p.send(hostAddress, hostPort);
	}
	

	/**
	 * Displays a notification window in XBMC.
	 * @param title Message title
	 * @param message The actual message
	 */
	public void sendNotification(String title, String message) throws IOException
	{
		PacketNOTIFICATION p;
		if(hasIcon)
			p = new PacketNOTIFICATION(title, message, iconType, iconData);
		else
			p = new PacketNOTIFICATION(title, message);
		p.send(hostAddress, hostPort);
	}

	/**
	 * Sends a Button event
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
	public void sendButton(short code, boolean repeat, boolean down, boolean queue, short amount, byte axis) throws IOException
	{
		PacketBUTTON p = new PacketBUTTON(code, repeat, down, queue, amount, axis);
		p.send(hostAddress, hostPort);
	}

	/**
	 * Sends a Button event
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
     * For example, if map_name is "KB" referring to the <keyboard> section in Keymap.xml 
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
	public void sendButton(String map_name, String button_name, boolean repeat, boolean down, boolean queue, short amount, byte axis) throws IOException
	{
		PacketBUTTON p = new PacketBUTTON(map_name, button_name, repeat, down, queue, amount, axis);
		p.send(hostAddress, hostPort);
	}

	/**
	 * Sets the mouse position in XBMC
	 * @param x horizontal position ranging from 0 to 65535
	 * @param y vertical position ranging from 0 to 65535
	 */
	public void sendMouse(int x, int y) throws IOException
	{
		PacketMOUSE p = new PacketMOUSE(x, y);
		p.send(hostAddress, hostPort);
	}
	
	/**
	 * Sends a ping to the XBMC EventServer
	 * @throws IOException
	 */
	public void ping() throws IOException
	{
		PacketPING p = new PacketPING();
		p.send(hostAddress, hostPort);
	}

	/**
	 * Tells XBMC to log the message to xbmc.log with the loglevel as specified.
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
	public void sendLog(byte loglevel, String logmessage) throws IOException
	{
		PacketLOG p = new PacketLOG(loglevel, logmessage);
		p.send(hostAddress, hostPort);
	}
	
	/**
	 * Tells XBMC to do the action specified, based on the type it knows were it needs to be sent.
	 * @param actionmessage Actionmessage (as in scripting/skinning)
	 */
	public void sendAction(String actionmessage) throws IOException
	{
		PacketACTION p = new PacketACTION(actionmessage);
		p.send(hostAddress, hostPort);
	}
	
	/**
	 * Implements a PingThread which tells XBMC EventServer that the Client is alive (this should
	 * be done at least every 60 seconds!
	 * @author Stefan Agner
	 *
	 */
	class PingThread extends Thread
	{
		private InetAddress hostAddress;
		private int hostPort;
		private int sleepTime;
		private boolean giveup = false;
		
		public PingThread(InetAddress hostAddress, int hostPort, int sleepTime)
		{
			super("XBMC EventClient Ping-Thread");
			this.hostAddress = hostAddress;
			this.hostPort = hostPort;
			this.sleepTime = sleepTime;
		}
		
		public void giveup()
		{
			giveup = true;
		}
		
		public void run()
		{
			while(!giveup)
			{
				try {
					PacketPING p = new PacketPING();
					p.send(hostAddress, hostPort);
				} catch (IOException e) {
					
					e.printStackTrace();
				}
				
				try {
					Thread.sleep(sleepTime);
				} catch (InterruptedException e) {
				}
			}
		}
	}
}
