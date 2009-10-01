from bluetooth import *

class HID:
    def __init__(self, bdaddress=None):
        self.cport = 0x11  # HID's control PSM
        self.iport = 0x13  # HID' interrupt PSM
        self.backlog = 1

        self.address = ""
        if bdaddress:
            self.address = bdaddress

        # create the HID control socket
        self.csock = BluetoothSocket( L2CAP )
        self.csock.bind((self.address, self.cport))
        set_l2cap_mtu(self.csock, 64)
        self.csock.settimeout(2)
        self.csock.listen(self.backlog)

        # create the HID interrupt socket
        self.isock = BluetoothSocket( L2CAP )
        self.isock.bind((self.address, self.iport))
        set_l2cap_mtu(self.isock, 64)
        self.isock.settimeout(2)
        self.isock.listen(self.backlog)

        self.connected = False


    def listen(self):
        try:
            (self.client_csock, self.caddress) = self.csock.accept()
            print "Accepted Control connection from %s" % self.caddress[0]
            (self.client_isock, self.iaddress) = self.isock.accept()
            print "Accepted Interrupt connection from %s" % self.iaddress[0]
            self.connected = True
            return True
        except Exception, e:
            self.connected = False
            return False


    def get_control_socket(self):
        if self.connected:
            return (self.client_csock, self.caddress)
        else:
            return None


    def get_interrupt_socket(self):
        if self.connected:
            return (self.client_isock, self.iaddress)
        else:
            return None
