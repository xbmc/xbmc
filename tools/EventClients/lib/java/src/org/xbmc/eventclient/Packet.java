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
import java.io.IOException;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;

/**
 * XBMC Event Client Class
 * <p>
 * Implementation of XBMC's UDP based input system.
 * A set of classes that abstract the various packets that the event server
 * currently supports. In addition, there's also a class, XBMCClient, that
 * provides functions that sends the various packets. Use XBMCClient if you
 * don't need complete control over packet structure.
 * </p>
 * <p>
 * The basic workflow involves:
 * <ol>
 * <li>Send a HELO packet</li>
 * <li>Send x number of valid packets</li>
 * <li>Send a BYE packet</li>
 * </ol>
 * </p>
 * <p>
 * IMPORTANT NOTE ABOUT TIMEOUTS:
 * A client is considered to be timed out if XBMC doesn't received a packet
 * at least once every 60 seconds. To "ping" XBMC with an empty packet use
 * PacketPING or XBMCClient.ping(). See the documentation for details.
 * </p>
 * <p>
 * Base class that implements a single event packet.
 * - Generic packet structure (maximum 1024 bytes per packet)
 * - Header is 32 bytes long, so 992 bytes available for payload
 * - large payloads can be split into multiple packets using H4 and H5
 *   H5 should contain total no. of packets in such a case
 * - H6 contains length of P1, which is limited to 992 bytes
 * - if H5 is 0 or 1, then H4 will be ignored (single packet msg)
 * - H7 must be set to zeros for now
 * </p>
 * <pre>
 *   -----------------------------
 *   | -H1 Signature ("XBMC")    | - 4  x CHAR                4B
 *   | -H2 Version (eg. 2.0)     | - 2  x UNSIGNED CHAR       2B
 *   | -H3 PacketType            | - 1  x UNSIGNED SHORT      2B
 *   | -H4 Sequence number       | - 1  x UNSIGNED LONG       4B
 *   | -H5 No. of packets in msg | - 1  x UNSIGNED LONG       4B
 *   | -H6 Payloadsize of packet | - 1  x UNSIGNED SHORT      2B
 *   | -H7 Client's unique token | - 1  x UNSIGNED LONG       4B
 *   | -H8 Reserved              | - 10 x UNSIGNED CHAR      10B
 *   |---------------------------|
 *   | -P1 payload               | -
 *   -----------------------------
 * </pre>
 * @author Stefan Agner
 *
 */
public abstract class Packet {

	private byte[] sig;
	private byte[] payload = new byte[0];
	private byte minver;
	private byte majver;

	private short packettype;


	private final static short MAX_PACKET_SIZE  = 1024;
	private final static short HEADER_SIZE      = 32;
	private final static short MAX_PAYLOAD_SIZE = MAX_PACKET_SIZE - HEADER_SIZE;

	protected final static byte PT_HELO          = 0x01;
	protected final static byte PT_BYE           = 0x02;
	protected final static byte PT_BUTTON        = 0x03;
	protected final static byte PT_MOUSE         = 0x04;
	protected final static byte PT_PING          = 0x05;
	protected final static byte PT_BROADCAST     = 0x06;
	protected final static byte PT_NOTIFICATION  = 0x07;
	protected final static byte PT_BLOB          = 0x08;
	protected final static byte PT_LOG           = 0x09;
	protected final static byte PT_ACTION        = 0x0A;
	protected final static byte PT_DEBUG         = (byte)0xFF;

	public final static byte ICON_NONE = 0x00;
	public final static byte ICON_JPEG = 0x01;
	public final static byte ICON_PNG  = 0x02;
	public final static byte ICON_GIF  = 0x03;

	private static int uid = (int)(Math.random()*Integer.MAX_VALUE);

	/**
	 * This is an Abstract class and cannot be instanced. Please use one of the Packet implementation Classes
	 * (PacketXXX).
	 *
	 * Implements an XBMC Event Client Packet. Type is to be specified at creation time, Payload can be added
	 * with the various appendPayload methods. Packet can be sent through UDP-Socket with method "send".
     * @param packettype Type of Packet (PT_XXX)
	 */
	protected Packet(short packettype)
	{
		sig = new byte[] {'X', 'B', 'M', 'C' };
        minver = 0;
        majver = 2;
        this.packettype = packettype;
	}

	/**
	 * Appends a String to the payload (terminated with 0x00)
	 * @param payload Payload as String
	 */
	protected void appendPayload(String payload)
	{
		byte[] payloadarr = payload.getBytes();
		int oldpayloadsize = this.payload.length;
		byte[] oldpayload = this.payload;
		this.payload = new byte[oldpayloadsize+payloadarr.length+1]; // Create new Array with more place (+1 for string terminator)
		System.arraycopy(oldpayload, 0, this.payload, 0, oldpayloadsize);
		System.arraycopy(payloadarr, 0, this.payload, oldpayloadsize, payloadarr.length);
	}

	/**
	 * Appends a single Byte to the payload
	 * @param payload Payload
	 */
	protected void appendPayload(byte payload)
	{
		appendPayload(new byte[] { payload });
	}

	/**
	 * Appends a Byte-Array to the payload
	 * @param payloadarr Payload
	 */
	protected void appendPayload(byte[] payloadarr)
	{
		int oldpayloadsize = this.payload.length;
		byte[] oldpayload = this.payload;
		this.payload = new byte[oldpayloadsize+payloadarr.length];
		System.arraycopy(oldpayload, 0, this.payload, 0, oldpayloadsize);
		System.arraycopy(payloadarr, 0, this.payload, oldpayloadsize, payloadarr.length);
	}

	/**
	 * Appends an integer to the payload
	 * @param i Payload
	 */
	protected void appendPayload(int i) {
		appendPayload(intToByteArray(i));
	}

	/**
	 * Appends a short to the payload
	 * @param s Payload
	 */
	protected void appendPayload(short s) {
		appendPayload(shortToByteArray(s));
	}

	/**
	 * Get Number of Packets which will be sent with current Payload...
	 * @return Number of Packets
	 */
	public int getNumPackets()
	{
		return (int)((payload.length + (MAX_PAYLOAD_SIZE - 1)) / MAX_PAYLOAD_SIZE);
	}

	/**
	 * Get Header for a specific Packet in this sequence...
	 * @param seq Current sequence number
	 * @param maxseq Maximal sequence number
	 * @param actpayloadsize Payloadsize of this packet
	 * @return Byte-Array with Header information (currently 32-Byte long, see HEADER_SIZE)
	 */
	private byte[] getHeader(int seq, int maxseq, short actpayloadsize)
	{
		byte[] header = new byte[HEADER_SIZE];
		System.arraycopy(sig, 0, header, 0, 4);
		header[4] = majver;
		header[5] = minver;
		byte[] packettypearr = shortToByteArray(this.packettype);
		System.arraycopy(packettypearr, 0, header, 6, 2);
		byte[] seqarr = intToByteArray(seq);
		System.arraycopy(seqarr, 0, header, 8, 4);
		byte[] maxseqarr = intToByteArray(maxseq);
		System.arraycopy(maxseqarr, 0, header, 12, 4);
		byte[] payloadsize = shortToByteArray(actpayloadsize);
		System.arraycopy(payloadsize, 0, header, 16, 2);
		byte[] uid = intToByteArray(Packet.uid);
		System.arraycopy(uid, 0, header, 18, 4);
		byte[] reserved = new byte[10];
		System.arraycopy(reserved, 0, header, 22, 10);

		return header;
	}

	/**
	 * Generates the whole UDP-Message with Header and Payload of a specific Packet in sequence
	 * @param seq Current sequence number
	 * @return Byte-Array with UDP-Message
	 */
	private byte[] getUDPMessage(int seq)
	{
		int maxseq = (int)((payload.length + (MAX_PAYLOAD_SIZE - 1)) / MAX_PAYLOAD_SIZE);
		if(seq > maxseq)
			return null;

		short actpayloadsize;

		if(seq == maxseq)
			actpayloadsize = (short)(payload.length%MAX_PAYLOAD_SIZE);

		else
			actpayloadsize = (short)MAX_PAYLOAD_SIZE;

		byte[] pack = new byte[HEADER_SIZE+actpayloadsize];

		System.arraycopy(getHeader(seq, maxseq, actpayloadsize), 0, pack, 0, HEADER_SIZE);
		System.arraycopy(payload, (seq-1)*MAX_PAYLOAD_SIZE, pack, HEADER_SIZE, actpayloadsize);

		return pack;
	}

	/**
	 * Sends this packet to the EventServer
	 * @param adr Address of the EventServer
	 * @param port Port of the EventServer
	 * @throws IOException
	 */
	public void send(InetAddress adr, int port) throws IOException
	{
		int maxseq = getNumPackets();
		DatagramSocket s = new DatagramSocket();

		// For each Packet in Sequence...
		for(int seq=1;seq<=maxseq;seq++)
		{
			// Get Message and send them...
			byte[] pack = getUDPMessage(seq);
			DatagramPacket p = new DatagramPacket(pack, pack.length);
			p.setAddress(adr);
			p.setPort(port);
			s.send(p);
		}
	}

	/**
	 * Helper Method to convert an integer to a Byte array
	 * @param value
	 * @return Byte-Array
	 */
	private static final byte[] intToByteArray(int value) {
	          return new byte[] {
	                  (byte)(value >>> 24),
	                  (byte)(value >>> 16),
	                  (byte)(value >>> 8),
	                  (byte)value};
	}

	/**
	 * Helper Method to convert an short to a Byte array
	 * @param value
	 * @return Byte-Array
	 */
	private static final byte[] shortToByteArray(short value) {
        return new byte[] {
                (byte)(value >>> 8),
                (byte)value};
	}


}
