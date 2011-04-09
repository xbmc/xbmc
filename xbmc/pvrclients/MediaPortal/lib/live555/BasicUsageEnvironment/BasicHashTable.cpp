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
// Basic Hash Table implementation
// Implementation

#include "BasicHashTable.hh"
#include "strDup.hh"

#if defined(__WIN32__) || defined(_WIN32)
#else
#include <stddef.h>
#endif
#include <string.h>
#include <stdio.h>

// When there are this many entries per bucket, on average, rebuild
// the table to increase the number of buckets
#define REBUILD_MULTIPLIER 3

BasicHashTable::BasicHashTable(int keyType)
  : fBuckets(fStaticBuckets), fNumBuckets(SMALL_HASH_TABLE_SIZE),
    fNumEntries(0), fRebuildSize(SMALL_HASH_TABLE_SIZE*REBUILD_MULTIPLIER),
    fDownShift(28), fMask(0x3), fKeyType(keyType) {
  for (unsigned i = 0; i < SMALL_HASH_TABLE_SIZE; ++i) {
    fStaticBuckets[i] = NULL;
  }
}

BasicHashTable::~BasicHashTable() {
  // Free all the entries in the table:
  for (unsigned i = 0; i < fNumBuckets; ++i) {
    TableEntry* entry;
    while ((entry = fBuckets[i]) != NULL) {
      deleteEntry(i, entry);
    }
  }

  // Also free the bucket array, if it was dynamically allocated:
  if (fBuckets != fStaticBuckets) delete[] fBuckets;
}

void* BasicHashTable::Add(char const* key, void* value) {
  void* oldValue;
  unsigned index;
  TableEntry* entry = lookupKey(key, index);
  if (entry != NULL) {
    // There's already an item with this key
    oldValue = entry->value;
  } else {
    // There's no existing entry; create a new one:
    entry = insertNewEntry(index, key);
    oldValue = NULL;
  }
  entry->value = value;

  // If the table has become too large, rebuild it with more buckets:
  if (fNumEntries >= fRebuildSize) rebuild();

  return oldValue;
}

Boolean BasicHashTable::Remove(char const* key) {
  unsigned index;
  TableEntry* entry = lookupKey(key, index);
  if (entry == NULL) return False; // no such entry

  deleteEntry(index, entry);

  return True;
}

void* BasicHashTable::Lookup(char const* key) const {
  unsigned index;
  TableEntry* entry = lookupKey(key, index);
  if (entry == NULL) return NULL; // no such entry

  return entry->value;
}

unsigned BasicHashTable::numEntries() const {
  return fNumEntries;
}

BasicHashTable::Iterator::Iterator(BasicHashTable& table)
  : fTable(table), fNextIndex(0), fNextEntry(NULL) {
}

void* BasicHashTable::Iterator::next(char const*& key) {
  while (fNextEntry == NULL) {
    if (fNextIndex >= fTable.fNumBuckets) return NULL;

    fNextEntry = fTable.fBuckets[fNextIndex++];
  }

  BasicHashTable::TableEntry* entry = fNextEntry;
  fNextEntry = entry->fNext;

  key = entry->key;
  return entry->value;
}

////////// Implementation of HashTable creation functions //////////

HashTable* HashTable::create(int keyType) {
  return new BasicHashTable(keyType);
}

HashTable::Iterator* HashTable::Iterator::create(HashTable& hashTable) {
  // "hashTable" is assumed to be a BasicHashTable
  return new BasicHashTable::Iterator((BasicHashTable&)hashTable);
}

////////// Implementation of internal member functions //////////

BasicHashTable::TableEntry* BasicHashTable
::lookupKey(char const* key, unsigned& index) const {
  TableEntry* entry;
  index = hashIndexFromKey(key);

  for (entry = fBuckets[index]; entry != NULL; entry = entry->fNext) {
    if (keyMatches(key, entry->key)) break;
  }

  return entry;
}

Boolean BasicHashTable
::keyMatches(char const* key1, char const* key2) const {
  // The way we check the keys for a match depends upon their type:
  if (fKeyType == STRING_HASH_KEYS) {
    return (strcmp(key1, key2) == 0);
  } else if (fKeyType == ONE_WORD_HASH_KEYS) {
    return (key1 == key2);
  } else {
    unsigned* k1 = (unsigned*)key1;
    unsigned* k2 = (unsigned*)key2;

    for (int i = 0; i < fKeyType; ++i) {
      if (k1[i] != k2[i]) return False; // keys differ
    }
    return True;
  }
}

BasicHashTable::TableEntry* BasicHashTable
::insertNewEntry(unsigned index, char const* key) {
  TableEntry* entry = new TableEntry();
  entry->fNext = fBuckets[index];
  fBuckets[index] = entry;

  ++fNumEntries;
  assignKey(entry, key);

  return entry;
}

void BasicHashTable::assignKey(TableEntry* entry, char const* key) {
  // The way we assign the key depends upon its type:
  if (fKeyType == STRING_HASH_KEYS) {
    entry->key = strDup(key);
  } else if (fKeyType == ONE_WORD_HASH_KEYS) {
    entry->key = key;
  } else if (fKeyType > 0) {
    unsigned* keyFrom = (unsigned*)key;
    unsigned* keyTo = new unsigned[fKeyType];
    for (int i = 0; i < fKeyType; ++i) keyTo[i] = keyFrom[i];

    entry->key = (char const*)keyTo;
  }
}

void BasicHashTable::deleteEntry(unsigned index, TableEntry* entry) {
  TableEntry** ep = &fBuckets[index];

  Boolean foundIt = False;
  while (*ep != NULL) {
    if (*ep == entry) {
      foundIt = True;
      *ep = entry->fNext;
      break;
    }
    ep = &((*ep)->fNext);
  }

  if (!foundIt) { // shouldn't happen
#ifdef DEBUG
    fprintf(stderr, "BasicHashTable[%p]::deleteEntry(%d,%p): internal error - not found (first entry %p", this, index, entry, fBuckets[index]);
    if (fBuckets[index] != NULL) fprintf(stderr, ", next entry %p", fBuckets[index]->fNext);
    fprintf(stderr, ")\n");
#endif
  }

  --fNumEntries;
  deleteKey(entry);
  delete entry;
}

void BasicHashTable::deleteKey(TableEntry* entry) {
  // The way we delete the key depends upon its type:
  if (fKeyType == ONE_WORD_HASH_KEYS) {
    entry->key = NULL;
  } else {
    delete[] (char*)entry->key;
    entry->key = NULL;
  }
}

void BasicHashTable::rebuild() {
  // Remember the existing table size:
  unsigned oldSize = fNumBuckets;
  TableEntry** oldBuckets = fBuckets;

  // Create the new sized table:
  fNumBuckets *= 4;
  fBuckets = new TableEntry*[fNumBuckets];
  for (unsigned i = 0; i < fNumBuckets; ++i) {
    fBuckets[i] = NULL;
  }
  fRebuildSize *= 4;
  fDownShift -= 2;
  fMask = (fMask<<2)|0x3;

  // Rehash the existing entries into the new table:
  for (TableEntry** oldChainPtr = oldBuckets; oldSize > 0;
       --oldSize, ++oldChainPtr) {
    for (TableEntry* hPtr = *oldChainPtr; hPtr != NULL;
	 hPtr = *oldChainPtr) {
      *oldChainPtr = hPtr->fNext;

      unsigned index = hashIndexFromKey(hPtr->key);

      hPtr->fNext = fBuckets[index];
      fBuckets[index] = hPtr;
    }
  }

  // Free the old bucket array, if it was dynamically allocated:
  if (oldBuckets != fStaticBuckets) delete[] oldBuckets;
}

unsigned BasicHashTable::hashIndexFromKey(char const* key) const {
  unsigned result = 0;

  if (fKeyType == STRING_HASH_KEYS) {
    while (1) {
      char c = *key++;
      if (c == 0) break;
      result += (result<<3) + (unsigned)c;
    }
    result &= fMask;
  } else if (fKeyType == ONE_WORD_HASH_KEYS) {
    result = randomIndex((unsigned long)key);
  } else {
    unsigned* k = (unsigned*)key;
    unsigned long sum = 0;
    for (int i = 0; i < fKeyType; ++i) {
      sum += k[i];
    }
    result = randomIndex(sum);
  }

  return result;
}

