#!/usr/bin/env python
# -*- coding: utf-8 -*-

#   Copyright (C) 2008-2013 Team XBMC
#
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License along
#   with this program; if not, write to the Free Software Foundation, Inc.,
#   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

"""
Simple wrapper around Avahi
"""

__author__  = "d4rk@xbmc.org"
__version__ = "0.1"

try:
    import time
    import dbus, gobject, avahi
    from dbus import DBusException
    from dbus.mainloop.glib import DBusGMainLoop
except Exception, e:
    print "Zeroconf support disabled. To enable, install the following Python modules:"
    print "    dbus, gobject, avahi"
    pass

SERVICE_FOUND  = 1
SERVICE_LOST   = 2

class Browser:
    """ Simple Zeroconf Browser """

    def __init__( self, service_types = {} ):
        """
        service_types - dictionary of services => handlers
        """
        self._stop = False
        self.loop = DBusGMainLoop()
        self.bus = dbus.SystemBus( mainloop=self.loop )
        self.server = dbus.Interface( self.bus.get_object( avahi.DBUS_NAME, '/' ),
                                 'org.freedesktop.Avahi.Server')
        self.handlers = {}

        for type in service_types.keys():
            self.add_service( type, service_types[ type ] )


    def add_service( self, type, handler = None ):
        """
        Add a service that the browser should watch for
        """
        self.sbrowser = dbus.Interface(
            self.bus.get_object(
                avahi.DBUS_NAME,
                self.server.ServiceBrowserNew(
                    avahi.IF_UNSPEC,
                    avahi.PROTO_UNSPEC,
                    type,
                    'local',
                    dbus.UInt32(0)
                    )
                ),
            avahi.DBUS_INTERFACE_SERVICE_BROWSER)
        self.handlers[ type ] = handler
        self.sbrowser.connect_to_signal("ItemNew", self._new_item_handler)
        self.sbrowser.connect_to_signal("ItemRemove", self._remove_item_handler)


    def run(self):
        """
        Run the gobject event loop
        """
        # Don't use loop.run() because Python's GIL will block all threads
        loop = gobject.MainLoop()
        context = loop.get_context()
        while not self._stop:
            if context.pending():
                context.iteration( True )
            else:
                time.sleep(1)

    def stop(self):
        """
        Stop the gobject event loop
        """
        self._stop = True


    def _new_item_handler(self, interface, protocol, name, stype, domain, flags):
        if flags & avahi.LOOKUP_RESULT_LOCAL:
            # local service, skip
            pass

        self.server.ResolveService(
            interface,
            protocol,
            name,
            stype,
            domain,
            avahi.PROTO_UNSPEC,
            dbus.UInt32(0),
            reply_handler = self._service_resolved_handler,
            error_handler = self._error_handler
            )
        return


    def _remove_item_handler(self, interface, protocol, name, stype, domain, flags):
        if self.handlers[ stype ]:
            # FIXME: more details needed here
            try:
                self.handlers[ stype ]( SERVICE_LOST, { 'type' : stype, 'name' : name } )
            except:
                pass


    def _service_resolved_handler( self, *args ):
        service = {}
        service['type']     = str( args[3] )
        service['name']     = str( args[2] )
        service['address']  = str( args[7] )
        service['hostname'] = str( args[5] )
        service['port']     = int( args[8] )

        # if the service type has a handler call it
        try:
            if self.handlers[ args[3] ]:
                self.handlers[ args[3] ]( SERVICE_FOUND, service )
        except:
            pass


    def _error_handler( self, *args ):
        print 'ERROR: %s ' % str( args[0] )


if __name__ == "__main__":
    def service_handler( found, service ):
        print "---------------------"
        print ['Found Service', 'Lost Service'][found-1]
        for key in service.keys():
            print key+" : "+str( service[key] )

    browser = Browser( {
            '_xbmc-events._udp' : service_handler,
            '_xbmc-web._tcp'    : service_handler
            } )
    browser.run()

