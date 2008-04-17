BLUEZ=0

try:
    import bluetooth
    BLUEZ=1
except:
    try:
        import lightblue
    except:
        print "ERROR: You need to have either LightBlue or PyBluez installed\n"\
            "       in order to use this program."
        print "- PyBluez (Linux / Windows XP) http://org.csail.mit.edu/pybluez/"
        print "- LightBlue (Mac OS X / Linux) http://lightblue.sourceforge.net/"
        exit()

def bt_create_socket():
    if BLUEZ:
        sock = bluetooth.BluetoothSocket(bluetooth.L2CAP)
    else:
        sock = lightblue.socket(lightblue.L2CAP)
    return sock

def bt_create_rfcomm_socket():
    if BLUEZ:
        sock = bluetooth.BluetoothSocket( bluetooth.RFCOMM )
        sock.bind(("",bluetooth.PORT_ANY))
    else:
        sock = lightblue.socket(lightblue.RFCOMM)
        sock.bind(("",lightblue.PORT_ANY))
    return sock

def bt_discover_devices():
    if BLUEZ:
        nearby = bluetooth.discover_devices()
    else:
        nearby = lightblue.finddevices()
    return nearby

def bt_lookup_name(bdaddr):
    if BLUEZ:
        bname = bluetooth.lookup_name( bdaddr )
    else:
        bname = bdaddr[1]
    return bname

def bt_lookup_addr(bdaddr):
    if BLUEZ:
        return bdaddr
    else:
        return bdaddr[0]

def bt_advertise(name, uuid, socket):
    if BLUEZ:
        bluetooth.advertise_service( socket, name,
                           service_id = uuid,
                           service_classes = [ uuid, bluetooth.SERIAL_PORT_CLASS ],
                           profiles = [ bluetooth.SERIAL_PORT_PROFILE ] )
    else:
        lightblue.advertise(name, socket, uuid)

def bt_stop_advertising(socket):
    if BLUEZ:
        stop_advertising(socket)
    else:
        stopadvertise(socket)
