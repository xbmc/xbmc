/**********
This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the
Free Software Foundation; either version 2.1 of the License, or (at your
option) any later version. (See <http://www.gnu.org/copyleft/lesser.html>.)

This library is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
**********/
// "mTunnel" multicast access service
// Copyright (c) 1996-2010 Live Networks, Inc.  All rights reserved.
// Network Addresses
// C++ header

#ifndef _NET_ADDRESS_HH
#define _NET_ADDRESS_HH

#ifndef _HASH_TABLE_HH
#include "HashTable.hh"
#endif

#ifndef _NET_COMMON_H
#include "NetCommon.h"
#endif

#ifndef _USAGE_ENVIRONMENT_HH
#include "UsageEnvironment.hh"
#endif

// Definition of a type representing a low-level network address.
// At present, this is 32-bits, for IPv4.  Later, generalize it,
// to allow for IPv6.
typedef u_int32_t netAddressBits;

class NetAddress {
    public:
	NetAddress(u_int8_t const* data,
		   unsigned length = 4 /* default: 32 bits */);
	NetAddress(unsigned length = 4); // sets address data to all-zeros
	NetAddress(NetAddress const& orig);
	NetAddress& operator=(NetAddress const& rightSide);
	virtual ~NetAddress();

	unsigned length() const { return fLength; }
	u_int8_t const* data() const // always in network byte order
		{ return fData; }

    private:
	void assign(u_int8_t const* data, unsigned length);
	void clean();

	unsigned fLength;
	u_int8_t* fData;
};

class NetAddressList {
    public:
	NetAddressList(char const* hostname);
	NetAddressList(NetAddressList const& orig);
	NetAddressList& operator=(NetAddressList const& rightSide);
	virtual ~NetAddressList();

	unsigned numAddresses() const { return fNumAddresses; }

	NetAddress const* firstAddress() const;

	// Used to iterate through the addresses in a list:
	class Iterator {
	    public:
		Iterator(NetAddressList const& addressList);
		NetAddress const* nextAddress(); // NULL iff none
	    private:
		NetAddressList const& fAddressList;
		unsigned fNextIndex;
	};

    private:
	void assign(netAddressBits numAddresses, NetAddress** addressArray);
	void clean();

	friend class Iterator;
	unsigned fNumAddresses;
	NetAddress** fAddressArray;
};

typedef u_int16_t portNumBits;

class Port {
    public:
	Port(portNumBits num /* in host byte order */);

	portNumBits num() const // in network byte order
		{ return fPortNum; }

    private:
	portNumBits fPortNum; // stored in network byte order
#ifdef IRIX
	portNumBits filler; // hack to overcome a bug in IRIX C++ compiler
#endif
};

UsageEnvironment& operator<<(UsageEnvironment& s, const Port& p);


// A generic table for looking up objects by (address1, address2, port)
class AddressPortLookupTable {
    public:
	AddressPortLookupTable();
	virtual ~AddressPortLookupTable();

	void* Add(netAddressBits address1, netAddressBits address2,
		  Port port, void* value);
		// Returns the old value if different, otherwise 0
	Boolean Remove(netAddressBits address1, netAddressBits address2,
		       Port port);
	void* Lookup(netAddressBits address1, netAddressBits address2,
		     Port port);
		// Returns 0 if not found

	// Used to iterate through the entries in the table
	class Iterator {
	    public:
		Iterator(AddressPortLookupTable& table);
		virtual ~Iterator();

		void* next(); // NULL iff none

	    private:
		HashTable::Iterator* fIter;
	};

    private:
	friend class Iterator;
	HashTable* fTable;
};


Boolean IsMulticastAddress(netAddressBits address);

#endif
