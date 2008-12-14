///
///	@file 	hash.cpp
/// @brief 	Fast hashing lookup module
/// @overview This hash is a fast key lookup mechanism that is 
///		ideal for symbol tables. Keys are strings and the value 
///		entries can be any sub-class of HashEntry.  The keys are hashed 
///		into a series of buckets which then have a chain of hash entries 
///		using the standard doubly linked list classes (List/Link). The 
///		chain in in collating sequence so search time through the chain 
///		is on average (N/hashSize)/2.
///	@remarks This module is not thread-safe. It is the callers responsibility
///	to perform all thread synchronization.
//
//	FUTURE -- rename collection
//
////////////////////////////////// Copyright ///////////////////////////////////
//
//	@copy	default
//	
//	Copyright (c) Mbedthis Software LLC, 2003-2007. All Rights Reserved.
//	
//	This software is distributed under commercial and open source licenses.
//	You may use the GPL open source license described below or you may acquire 
//	a commercial license from Mbedthis Software. You agree to be fully bound 
//	by the terms of either license. Consult the LICENSE.TXT distributed with 
//	this software for full details.
//	
//	This software is open source; you can redistribute it and/or modify it 
//	under the terms of the GNU General Public License as published by the 
//	Free Software Foundation; either version 2 of the License, or (at your 
//	option) any later version. See the GNU General Public License for more 
//	details at: http://www.mbedthis.com/downloads/gplLicense.html
//	
//	This program is distributed WITHOUT ANY WARRANTY; without even the 
//	implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
//	
//	This GPL license does NOT permit incorporating this software into 
//	proprietary programs. If you are unable to comply with the GPL, you must
//	acquire a commercial license to use this software. Commercial licenses 
//	for this software and support services are available from Mbedthis 
//	Software at http://www.mbedthis.com 
//	
//	@end
//
////////////////////////////////// Includes ////////////////////////////////////

#include	"mpr.h"

//////////////////////////////////// Code //////////////////////////////////////
//
//	Create a new hash table of a given size. Caller should provide a size that
//	is a prime number for the greatest efficiency.
//

MprHashTable::MprHashTable(int size)
{
	if (size < 7) {
		size = 7;
	}

	count = 0;
	this->size = size;
	buckets = new MprList[size];
}

////////////////////////////////////////////////////////////////////////////////
//
//	Delete the hash table symbol table.
//

MprHashTable::~MprHashTable()
{
	removeAll();
	delete[] buckets;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Insert an entry into the hash table. If the entry already exists, update 
//	its value. 
//

int MprHashTable::insert(MprHashEntry *value)
{
	MprList			*bp;
	MprHashEntry		*ep;

	if ((ep = lookupInner(value->key, &bp)) != 0) {
		//
		//	Already exists. Delete the old and insert the new. Insert
		//	first to make sure we get the right position
		//
		ep->insertAfter(value);
		value->bucket = bp;
		bp->remove(ep);
		delete ep;
		return 0;
	}
	//
	//	New entry
	//
	bp->insert(value);
	value->bucket = bp;
	count++;
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Delete an entry from the table
//

int MprHashTable::remove(char *key)
{
	MprList			*bp;
	MprHashEntry		*ep;

	if ((ep = lookupInner(key, &bp)) == 0) {
		return MPR_ERR_NOT_FOUND;
	}
	bp->remove(ep);
	count--;
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Delete an entry from the table
//

int MprHashTable::remove(MprHashEntry *ep)
{
	ep->bucket->remove(ep);
	count--;
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

int MprHashTable::removeAll()
{
	MprList			*bp;
	MprHashEntry	*ep, *nextEp;
	int				i;

	for (i = 0; i < size; i++) {
		bp = &buckets[i];
		ep = (MprHashEntry*) bp->getFirst();
		while (ep) {
			nextEp = (MprHashEntry*) bp->getNext(ep);
			bp->remove(ep);
			delete ep;
			ep = nextEp;
			count--;
		}
	}
	mprAssert(count == 0);
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Lookup a key and return the hash entry
//

MprHashEntry *MprHashTable::lookup(char *key)
{
	mprAssert(key);

	return lookupInner(key, 0);
}

////////////////////////////////////////////////////////////////////////////////

MprHashEntry *MprHashTable::lookupInner(char *key, MprList **bucket)
{
	MprList			*bp;
	MprHashEntry	*ep;
	int				index, rc;

	mprAssert(key);

	index = hashIndex(key);
	bp = &buckets[index];
	ep = (MprHashEntry*) bp->getFirst();
	if (bucket) {
		*bucket = bp;
	}

	while (ep) {
		rc = strcmp(ep->key, key);
		if (rc == 0) {
			return ep;
		}
		ep = (MprHashEntry*) bp->getNext(ep);
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Return the first entry in the table.
//

MprHashEntry *MprHashTable::getFirst()
{
	MprHashEntry	*ep;
	int				i;

	for (i = 0; i < size; i++) {
		if ((ep = (MprHashEntry*) buckets[i].getFirst()) != 0) {
			return ep;
		}
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Return the next entry in the table
//

MprHashEntry *MprHashTable::getNext(MprHashEntry *ep)
{
	MprList			*bp;
	MprHashEntry	*np;

	mprAssert(ep);

	bp = ep->bucket;
	np = (MprHashEntry*) bp->getNext(ep);
	if (np) {
		return np;
	}

	for (++bp; bp < &buckets[size]; bp++) {
		if ((ep = (MprHashEntry*) bp->getFirst()) != 0) {
			return ep;
		}
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Hash the key to produce a hash index. 
//

int MprHashTable::hashIndex(char *key)
{
	uint		sum;

	sum = 0;
	while (*key) {
		sum += (sum * 33) + *key++;
	}

	return sum % size;
}

////////////////////////////////////////////////////////////////////////////////
///////////////////////////////// MprHashEntry /////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

MprHashEntry::MprHashEntry()
{
	this->key = 0;
	bucket = 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Construct a new hash entry. Always duplicate the key.
//

MprHashEntry::MprHashEntry(char *key)
{
	this->key = mprStrdup(key);
	bucket = 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Virtual destructor. Free the key
//

MprHashEntry::~MprHashEntry()
{
	mprFree(key);
}

////////////////////////////////////////////////////////////////////////////////

char *MprHashEntry::getKey()
{
	return key;
}

////////////////////////////////////////////////////////////////////////////////

void MprHashEntry::setKey(char *newKey)
{
	mprFree(key);
	key = mprStrdup(newKey);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////// MprStringHashEntry //////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
//	Construct a new string hash entry. Always duplicate the value.
//

MprStringHashEntry::MprStringHashEntry(char *key, char *value) : 
	MprHashEntry(key)
{
	this->value = mprStrdup(value);
}

////////////////////////////////////////////////////////////////////////////////
//
//	Virtual destructor. Free the value
//

MprStringHashEntry::~MprStringHashEntry()
{
	mprFree(value);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////// MprStaticHashEntry //////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
//	Construct a new hash entry
//

MprStaticHashEntry::MprStaticHashEntry(char *key, char *value) : 
	MprHashEntry(key)
{
	this->value = value;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Virtual destructor
//

MprStaticHashEntry::~MprStaticHashEntry()
{
}

////////////////////////////////////////////////////////////////////////////////

//
// Local variables:
// tab-width: 4
// c-basic-offset: 4
// End:
// vim: sw=4 ts=4 
//
