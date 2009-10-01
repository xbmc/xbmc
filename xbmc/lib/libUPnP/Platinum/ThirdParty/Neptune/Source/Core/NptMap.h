/*****************************************************************
|
|   Neptune - Maps
|
| Copyright (c) 2002-2008, Axiomatic Systems, LLC.
| All rights reserved.
|
| Redistribution and use in source and binary forms, with or without
| modification, are permitted provided that the following conditions are met:
|     * Redistributions of source code must retain the above copyright
|       notice, this list of conditions and the following disclaimer.
|     * Redistributions in binary form must reproduce the above copyright
|       notice, this list of conditions and the following disclaimer in the
|       documentation and/or other materials provided with the distribution.
|     * Neither the name of Axiomatic Systems nor the
|       names of its contributors may be used to endorse or promote products
|       derived from this software without specific prior written permission.
|
| THIS SOFTWARE IS PROVIDED BY AXIOMATIC SYSTEMS ''AS IS'' AND ANY
| EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
| WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
| DISCLAIMED. IN NO EVENT SHALL AXIOMATIC SYSTEMS BE LIABLE FOR ANY
| DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
| (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
| LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
| ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
| (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
| SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
|
****************************************************************/

#ifndef _NPT_MAP_H_
#define _NPT_MAP_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "NptTypes.h"
#include "NptResults.h"
#include "NptList.h"

/*----------------------------------------------------------------------
|   NPT_Map
+---------------------------------------------------------------------*/
template <typename K, typename V> 
class NPT_Map 
{
public:
    // types
    class Entry {
    public:
        // constructor
        Entry(const K& key, const V& value) : m_Key(key), m_Value(value) {}
        Entry(const K& key) : m_Key(key) {}
        
        // accessors
        const K& GetKey()   const { return m_Key;   }
        const V& GetValue() const { return m_Value; }

        // operators 
        bool operator==(const Entry& other) const {
            return m_Key == other.m_Key && m_Value == other.m_Value;
        }

    protected:
        // methods
        void SetValue(const V& value) { m_Value = value; }

        // members
        K m_Key;
        V m_Value;

        // friends
        friend class NPT_Map<K,V>;
    };

    class EntryValueDeleter {
    public:
        void operator()(Entry* entry) const {
            delete entry->GetValue();
        }
    };

    // constructors
    NPT_Map<K,V>() {}
    NPT_Map<K,V>(const NPT_Map<K,V>& copy);

    // destructor
    ~NPT_Map<K,V>();

    // methods
    NPT_Result   Put(const K& key, const V& value);
    NPT_Result   Get(const K& key, V*& value) const;
    bool         HasKey(const K& key) const { return GetEntry(key) != NULL; }
    bool         HasValue(const V& value) const;
    NPT_Result   Erase(const K& key);
    NPT_Cardinal GetEntryCount() const         { return m_Entries.GetItemCount(); }
    const NPT_List<Entry*>& GetEntries() const { return m_Entries; }
    NPT_Result   Clear();

    // operators
    V&                  operator[](const K& key);
    const NPT_Map<K,V>& operator=(const NPT_Map<K,V>& copy);
    bool                operator==(const NPT_Map<K,V>& other) const;
    bool                operator!=(const NPT_Map<K,V>& other) const;

private:
    // types
    typedef typename NPT_List<Entry*>::Iterator ListIterator;

    // methods
    Entry* GetEntry(const K& key) const;

    // members
    NPT_List<Entry*> m_Entries;
};

/*----------------------------------------------------------------------
|   NPT_Map<K,V>::NPT_Map<K,V>
+---------------------------------------------------------------------*/
template <typename K, typename V>
NPT_Map<K,V>::NPT_Map(const NPT_Map<K,V>& copy)
{
    *this = copy;
}

/*----------------------------------------------------------------------
|   NPT_Map<K,V>::~NPT_Map<K,V>
+---------------------------------------------------------------------*/
template <typename K, typename V>
inline
NPT_Map<K,V>::~NPT_Map()
{
    // call Clear to ensure we delete all entry objects
    Clear();
}

/*----------------------------------------------------------------------
|   NPT_Map<K,V>::Clear
+---------------------------------------------------------------------*/
template <typename K, typename V>
inline
NPT_Result
NPT_Map<K,V>::Clear()
{
    m_Entries.Apply(NPT_ObjectDeleter<Entry>());
    m_Entries.Clear();

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_Map<K,V>::GetEntry
+---------------------------------------------------------------------*/
template <typename K, typename V>
typename NPT_Map<K,V>::Entry*
NPT_Map<K,V>::GetEntry(const K& key) const
{
    typename NPT_List<Entry*>::Iterator entry = m_Entries.GetFirstItem();
    while (entry) {
        if ((*entry)->GetKey() == key) {
            return *entry;
        }
        ++entry;
    }

    return NULL;
}

/*----------------------------------------------------------------------
|   NPT_Map<K,V>::Put
+---------------------------------------------------------------------*/
template <typename K, typename V>
inline
NPT_Result
NPT_Map<K,V>::Put(const K& key, const V& value)
{
    Entry* entry = GetEntry(key);
    if (entry == NULL) {
        // no existing entry for that key, create one
        m_Entries.Add(new Entry(key, value));
    } else {
        // replace the existing entry for that key
        entry->SetValue(value);
    }

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_Map<K,V>::Get
+---------------------------------------------------------------------*/
template <typename K, typename V>
inline
NPT_Result
NPT_Map<K,V>::Get(const K& key, V*& value) const
{
    Entry* entry = GetEntry(key);
    if (entry == NULL) {
        // no existing entry for that key
        value = NULL;
        return NPT_ERROR_NO_SUCH_ITEM;
    } else {
        // found an entry with that key
        value = &entry->m_Value;
        return NPT_SUCCESS;
    }
}

/*----------------------------------------------------------------------
|   NPT_Map<K,V>::HasValue
+---------------------------------------------------------------------*/
template <typename K, typename V>
bool
NPT_Map<K,V>::HasValue(const V& value) const
{
    ListIterator entry = m_Entries.GetFirstItem();
    while (entry) {
        if (value == (*entry)->m_Value) {
            return true;
        }
        ++entry;
    }

    return false;
}

/*----------------------------------------------------------------------
|   NPT_Map<K,V>::operator=
+---------------------------------------------------------------------*/
template <typename K, typename V>
const NPT_Map<K,V>&
NPT_Map<K,V>::operator=(const NPT_Map<K,V>& copy)
{
    // do nothing if we're assigning to ourselves
    if (this == &copy) return copy;

    // destroy all entries
    Clear();

    // copy all entries one by one
    ListIterator entry = copy.m_Entries.GetFirstItem();
    while (entry) {
        m_Entries.Add(new Entry((*entry)->GetKey(), (*entry)->GetValue()));
        ++entry;
    }

    return *this;
}

/*----------------------------------------------------------------------
|   NPT_Map<K,V>::Erase
+---------------------------------------------------------------------*/
template <typename K, typename V>
inline
NPT_Result
NPT_Map<K,V>::Erase(const K& key)
{
    ListIterator entry = m_Entries.GetFirstItem();
    while (entry) {
        if ((*entry)->GetKey() == key) {
            delete *entry; // do this before removing the entry from the
                           // list, because Erase() will invalidate the
                           // iterator item
            m_Entries.Erase(entry);
            return NPT_SUCCESS;
        }
        ++entry;
    }

    return NPT_ERROR_NO_SUCH_ITEM;
}

/*----------------------------------------------------------------------
|   NPT_Map<K,V>::operator==
+---------------------------------------------------------------------*/
template <typename K, typename V>
bool
NPT_Map<K,V>::operator==(const NPT_Map<K,V>& other) const
{
    // quick test
    if (m_Entries.GetItemCount() != other.m_Entries.GetItemCount()) return false;

    // compare all entries to all other entries
    ListIterator entry = m_Entries.GetFirstItem();
    while (entry) {
        V* value;
        if (NPT_SUCCEEDED(other.Get((*entry)->m_Key, value))) {
            // the other map has an entry for this key, check the value
            if (!(*value == (*entry)->m_Value)) return false;
        } else {
            // the other map does not have an entry for this key
            return false;
        }
        ++entry;
    }

    return true;
}

/*----------------------------------------------------------------------
|   NPT_Map<K,V>::operator!=
+---------------------------------------------------------------------*/
template <typename K, typename V>
inline
bool
NPT_Map<K,V>::operator!=(const NPT_Map<K,V>& other) const
{
    return !(*this == other);
}

/*----------------------------------------------------------------------
|   NPT_Map<K,V>::operator[]
+---------------------------------------------------------------------*/
template <typename K, typename V>
inline
V&
NPT_Map<K,V>::operator[](const K& key)
{
    Entry* entry = GetEntry(key);
    if (entry == NULL) {
        // create a new "default" entry for this key
        entry = new Entry(key);
        m_Entries.Add(entry);
    }
     
    return entry->m_Value;
}

#endif // _NPT_MAP_H_
