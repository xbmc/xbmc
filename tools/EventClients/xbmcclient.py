#!/usr/bin/python

"""
Implementation of XBMC's UDP based input system.

A set of classes that abstract the various packets that the event server
currently supports. The basic workflow involves:

1. Send a HELO packet
2. Send x number of valid packets
3. Send a BYE packet

IMPORTANT NOTE ABOUT TIMEOUTS:
A client is considered to be timed out if XBMC doesn't received a packet
at least once every 60 seconds. To "ping" XBMC with an empty packet use
PacketPING, see its documentation for details.
"""

__author__  = "d4rk@xbmc.org"
__version__ = "0.0.1"

from struct import pack

MAX_PACKET_SIZE  = 1024
HEADER_SIZE      = 32
MAX_PAYLOAD_SIZE = MAX_PACKET_SIZE - HEADER_SIZE

PT_HELO          = 0x01
PT_BYE           = 0x02
PT_BUTTON        = 0x03
PT_MOUSE         = 0x04
PT_PING          = 0x05
PT_BROADCAST     = 0x06
PT_NOTIFICATION  = 0x07
PT_BLOB          = 0x08
PT_DEBUG         = 0xFF

ICON_NONE = 0x00
ICON_JPEG = 0x01
ICON_PNG  = 0x02
ICON_GIF  = 0x03

BT_USE_NAME   = 0x01
BT_DOWN       = 0x02
BT_UP         = 0x04
BT_USE_AMOUNT = 0x08
BT_QUEUE      = 0x10
BT_NO_REPEAT  = 0x20

def format_string(msg):
    """ """
    return msg + "\0"

def format_uint32(num):
    """ """
    return pack ("!I", num)

def format_uint16(num):
    """ """
    return pack ("!H", num)


class Packet:
    """Base class that implements a single event packet.

     - Generic packet structure (maximum 1024 bytes per packet)
     - Header is 32 bytes long, so 992 bytes available for payload
     - large payloads can be split into multiple packets using H4 and H5
       H5 should contain total no. of packets in such a case
     - H6 contains length of P1, which is limited to 992 bytes
     - if H5 is 0 or 1, then H4 will be ignored (single packet msg)
     - H7 must be set to zeros for now

         -----------------------------
         | -H1 Signature ("XBMC")    | - 4  x CHAR                4B
         | -H2 Version (eg. 2.0)     | - 2  x UNSIGNED CHAR       2B
         | -H3 PacketType            | - 1  x UNSIGNED SHORT      2B
         | -H4 Sequence number       | - 1  x UNSIGNED LONG       4B
         | -H5 No. of packets in msg | - 1  x UNSIGNED LONG       4B
         | -H6 Payload size          | - 1  x UNSIGNED SHORT      2B
         | -H7 Reserved              | - 14 x UNSIGNED CHAR      14B
         |---------------------------|
         | -P1 payload               | -
         -----------------------------
    """
    def __init__(self):
        self.sig = "XBMC"
        self.minver = 0
        self.majver = 2
        self.seq = 1
        self.maxseq = 1
        self.payloadsize = 0
        self.reserved = "\0" * 14
        self.payload = ""
        return


    def append_payload(self, blob):
        """Append to existing payload

        Arguments:
        blob -- binary data to append to the current payload
        """
        self.set_payload(self.payload + blob)


    def set_payload(self, payload):
        """Set the payload for this packet

        Arguments:
        payload -- binary data that contains the payload
        """
        self.payload = payload
        self.payloadsize = len(self.payload)
        self.maxseq = int((self.payloadsize + (MAX_PAYLOAD_SIZE - 1)) / MAX_PAYLOAD_SIZE)


    def num_packets(self):
        """ Return the number of packets required for payload """
        return self.maxseq

    def get_header(self, packettype=-1, seq=1, maxseq=1, payload_size=0):
        """Construct a header and return as string

        Keyword arguments:
        packettype -- valid packet types are PT_HELO, PT_BYE, PT_BUTTON,
                      PT_MOUSE, PT_PING, PT_BORADCAST, PT_NOTIFICATION,
                      PT_BLOB, PT_DEBUG
        seq -- the sequence of this packet for a multi packet message
               (default 1)
        maxseq -- the total number of packets for a multi packet message
                  (default 1)
        payload_size -- the size of the payload of this packet (default 0)
        """
        if packettype < 0:
            packettype = self.packettype
        header = self.sig
        header += chr(self.majver)
        header += chr(self.minver)
        header += format_uint16(packettype)
        header += format_uint32(seq)
        header += format_uint32(maxseq)
        header += format_uint16(payload_size)
        header += self.reserved
        return header

    def get_payload_size(self, seq):
        """Returns the calculated payload size for the particular packet

        Arguments:
        seq -- the sequence number
        """
        if self.maxseq == 1:
            return self.payloadsize

        if seq < self.maxseq:
            return MAX_PAYLOAD_SIZE

        return self.payloadsize % MAX_PAYLOAD_SIZE


    def get_udp_message(self, packetnum=1):
        """Construct the UDP message for the specified packetnum and return
        as string

        Keyword arguments:
        packetnum -- the packet no. for which to construct the message
                     (default 1)
        """
        if packetnum > self.num_packets() or packetnum < 1:
            return ""
        header = ""
        if packetnum==1:
            header = self.get_header(self.packettype, packetnum, self.maxseq,
                                     self.get_payload_size(packetnum))
        else:
            header = self.get_header(PT_BLOB, packetnum, self.maxseq,
                                     self.get_payload_size(packetnum))

        payload = self.payload[ (packetnum-1) * MAX_PAYLOAD_SIZE :
                                (packetnum-1) * MAX_PAYLOAD_SIZE+
                                self.get_payload_size(packetnum) ]
        return header + payload

    def send(self, sock, addr):
        """Send the entire message to the specified socket and address.

        Arguments:
        sock -- datagram socket object (socket.socket)
        addr -- address, port pair (eg: ("127.0.0.1", 9777) )
        """
        for a in range ( 0, self.num_packets() ):
            sock.sendto(self.get_udp_message(a+1), addr)


class PacketHELO (Packet):
    """A HELO packet

    A HELO packet establishes a valid connection to XBMC. It is the
    first packet that should be sent.
    """
    def __init__(self, devicename=None, icon_type=ICON_NONE, icon_file=None):
        """
        Keyword arguments:
        devicename -- the string that identifies the client
        icon_type -- one of ICON_NONE, ICON_JPEG, ICON_PNG, ICON_GIF
        icon_file -- location of icon file with respect to current working
                     directory if icon_type is not ICON_NONE
        """
        Packet.__init__(self)
        self.packettype = PT_HELO
        self.icontype = icon_type
        self.set_payload ( format_string(devicename)[0:128] )
        self.append_payload( chr (icon_type) )
        self.append_payload( format_uint16 (0) ) # port no
        self.append_payload( format_uint32 (0) ) # reserved1
        self.append_payload( format_uint32 (0) ) # reserved2
        if icon_type != ICON_NONE and icon_file:
            self.append_payload( file(icon_file).read() )

class PacketNOTIFICATION (Packet):
    """A NOTIFICATION packet

    This packet displays a notification window in XBMC. It can contain
    a caption, a message and an icon.
    """
    def __init__(self, title, message, icon_type=ICON_NONE, icon_file=None):
        """
        Keyword arguments:
        title -- the notification caption / title
        message -- the main text of the notification
        icon_type -- one of ICON_NONE, ICON_JPEG, ICON_PNG, ICON_GIF
        icon_file -- location of icon file with respect to current working
                     directory if icon_type is not ICON_NONE
        """
        Packet.__init__(self)
        self.packettype = PT_NOTIFICATION
        self.title = title
        self.message = message
        self.set_payload ( format_string(title) )
        self.append_payload( format_string(message) )
        self.append_payload( chr (icon_type) )
        self.append_payload( format_uint32 (0) ) # reserved
        if icon_type != ICON_NONE and icon_file:
            self.append_payload( file(icon_file).read() )

class PacketBUTTON (Packet):
    """A BUTTON packet

    A button packet send a key press or release event to XBMC
    """
    def __init__(self, code=0, repeat=1, down=1, queue=0,
                 map_name="", button_name="", amount=None):
        """
        Keyword arguments:
        code -- raw button code (default: 0)
        repeat -- this key press should repeat until released (default: 1)
                  Note that queued pressed cannot repeat.
        down -- if this is 1, it implies a press event, 0 implies a release
                event. (default: 1)
        queue -- a queued key press means that the button event is
                 executed just once after which the next key press is
                 processed. It can be used for macros. Currently there
                 is no support for time delays between queued presses.
                 (default: 0)
        map_name -- a combination of map_name and button_name refers to a
                    mapping in the user's Keymap.xml or Lircmap.xml.
                    map_name can be one of the following:
                    "KB" => standard keyboard map ( <keyboard> section )
                    "XG" => xbox gamepad map ( <gamepad> section )
                    "R1" => xbox remote map ( <remote> section )
                    "R2" => xbox universal remote map ( <universalremote>
                            section )
                    "LI:devicename" => LIRC remote map where 'devicename' is the
                    actual device's name
        button_name -- a button name defined in the map specified in map_name.
                       For example, if map_name is "KB" refering to the
                       <keyboard> section in Keymap.xml then, valid
                       button_names include "printscreen", "minus", "x", etc.
        amount -- unimplemented for now; in the future it will be used for
                  specifying magnitude of analog key press events
        """
        Packet.__init__(self)
        self.flags = 0
        self.packettype = PT_BUTTON
        if type (code ) == str:
            code = ord(code)

        # assign code only if we don't have a map and button name
        if not (map_name and button_name):
            self.code = code
        else:
            self.flags |= BT_USE_NAME
            self.code = 0
        if amount:
            self.flags |= BT_USE_AMOUNT
            self.amount = int(amount)
        else:
            self.amount = 0
        if down:
            self.flags |= BT_DOWN
        else:
            self.flags |= BT_UP
        if not repeat:
            self.flags |= BT_NO_REPEAT
        if queue:
            self.flags |= BT_QUEUE
        self.set_payload ( format_uint16(self.code) )
        self.append_payload( format_uint16(self.flags) )
        self.append_payload( format_uint16(self.amount) )
        self.append_payload( format_string (map_name) )
        self.append_payload( format_string (button_name) )


class PacketBYE (Packet):
    """A BYE packet
    
    A BYE packet terminates the connection to XBMC.
    """
    def __init__(self):
        Packet.__init__(self)
        self.packettype = PT_BYE


class PacketPING (Packet):
    """A PING packet

    A PING packet tells XBMC that the client is still alive. All valid
    packets act as ping (not just this one). A client needs to ping
    XBMC at least once in 60 seconds or it will time out.
    """
    def __init__(self):
        Packet.__init__(self)
        self.packettype = PT_PING


