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
// Copyright (c) 1996-2010 Live Networks, Inc.  All rights reserved.
// Generic Hash Table
// C++ header

#ifndef _HASH_TABLE_HH
#define _HASH_TABLE_HH

#ifndef _BOOLEAN_HH
#include "Boolean.hh"
#endif

class HashTable {
    public:
	virtual ~HashTable();

	// The following must be implemented by a particular
	// implementation (subclass):
	static HashTable* create(int keyType);

	virtual void* Add(char const* key, void* value) = 0;
		// Returns the old value if different, otherwise 0
	virtual Boolean Remove(char const* key) = 0;
	virtual void* Lookup(char const* key) const = 0;
		// Returns 0 if not found
        virtual unsigned numEntries() const = 0;
        Boolean IsEmpty() const { return numEntries() == 0; }

	// Used to iterate through the members of the table:
        class Iterator {
	    public:
		// The following must be implemented by a particular
		// implementation (subclass):
		static Iterator* create(HashTable& hashTable);

		virtual ~Iterator();

		virtual void* next(char const*& key) = 0; // returns 0 if none

	    protected:
		Iterator(); // abstract base class
	};

        // A shortcut that can be used to successively remove each of
        // the entries in the table (e.g., so that their values can be
        // deleted, if they happen to be pointers to allocated memory).
        void* RemoveNext();

    protected:
	HashTable(); // abstract base class
};

// Warning: The following are deliberately the same as in
// Tcl's hash table implementation
int const STRING_HASH_KEYS = 0;
int const ONE_WORD_HASH_KEYS = 1;

#endif
