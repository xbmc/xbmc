/*
   AngelCode Scripting Library
   Copyright (c) 2003-2009 Andreas Jonsson

   This software is provided 'as-is', without any express or implied
   warranty. In no event will the authors be held liable for any
   damages arising from the use of this software.

   Permission is granted to anyone to use this software for any
   purpose, including commercial applications, and to alter it and
   redistribute it freely, subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you
      must not claim that you wrote the original software. If you use
      this software in a product, an acknowledgment in the product
      documentation would be appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and
      must not be misrepresented as being the original software.

   3. This notice may not be removed or altered from any source
      distribution.

   The original version of this library can be located at:
   http://www.angelcode.com/angelscript/

   Andreas Jonsson
   andreas@angelcode.com
*/

#ifndef AS_ARRAY_H
#define AS_ARRAY_H

#if !defined(AS_NO_MEMORY_H)
#include <memory.h>
#endif
#include <string.h> // some compilers declare memcpy() here

#ifdef _MSC_VER
#pragma warning(disable:4345) // warning about a change in how the code is handled in this version
#endif

BEGIN_AS_NAMESPACE

template <class T> class asCArray
{
public:
	asCArray();
	asCArray(const asCArray<T> &);
	asCArray(int reserve);
	~asCArray();

	void   Allocate(size_t numElements, bool keepData);
	size_t GetCapacity() const;

	void PushLast(const T &element);
	T    PopLast();

	void   SetLength(size_t numElements);
	size_t GetLength() const;

	void Copy(const T*, size_t count);
	asCArray<T> &operator =(const asCArray<T> &);

	const T &operator [](size_t index) const;
	T &operator [](size_t index);
	T *AddressOf();

	void Concatenate(const asCArray<T> &);
	void Concatenate(T*, unsigned int count);

	bool Exists(const T &element);
	int  IndexOf(const T &element);
	void RemoveIndex(size_t index);     // Removes the entry without reordering the array
	void RemoveValue(const T &element);

	bool operator==(const asCArray<T> &) const;
	bool operator!=(const asCArray<T> &) const;

protected:
	T      *array;
	size_t  length;
	size_t  maxLength;
	char    buf[8];
};

// Implementation

template <class T>
T *asCArray<T>::AddressOf()
{
	return array;
}

template <class T>
asCArray<T>::asCArray(void)
{
	array     = 0;
	length    = 0;
	maxLength = 0;
}

template <class T>
asCArray<T>::asCArray(const asCArray<T> &copy)
{
	array     = 0;
	length    = 0;
	maxLength = 0;

	*this = copy;
}

template <class T>
asCArray<T>::asCArray(int reserve)
{
	array     = 0;
	length    = 0;
	maxLength = 0;

	Allocate(reserve, false);
}

template <class T>
asCArray<T>::~asCArray(void)
{
	// Allocating a zero length array will free all memory
	Allocate(0,0);
}

template <class T>
size_t asCArray<T>::GetLength() const
{
	return length;
}

template <class T>
const T &asCArray<T>::operator [](size_t index) const
{
	asASSERT(index < length);

	return array[index];
}

template <class T>
T &asCArray<T>::operator [](size_t index)
{
	asASSERT(index < length);

	return array[index];
}

template <class T>
void asCArray<T>::PushLast(const T &element)
{
	if( length == maxLength )
	{
		if( maxLength == 0 )
			Allocate(1, false);
		else
			Allocate(2*maxLength, true);
	}

	array[length++] = element;
}

template <class T>
T asCArray<T>::PopLast()
{
	asASSERT(length > 0);

	return array[--length];
}

template <class T>
void asCArray<T>::Allocate(size_t numElements, bool keepData)
{
	// We have 4 situations
	// 1. The previous array is 8 bytes or smaller and the new array is also 8 bytes or smaller
	// 2. The previous array is 8 bytes or smaller and the new array is larger than 8 bytes
	// 3. The previous array is larger than 8 bytes and the new array is 8 bytes or smaller
	// 4. The previous array is larger than 8 bytes and the new array is also larger than 8 bytes

	T *tmp = 0;
	if( numElements )
	{
		if( sizeof(T)*numElements <= 8 )
			// Use the internal buffer
			tmp = (T*)buf;
		else
			// Allocate the array and construct each of the elements
			tmp = asNEWARRAY(T,numElements);

		if( array == tmp )
		{
			// Construct only the newly allocated elements
			for( size_t n = length; n < numElements; n++ )
				new (&tmp[n]) T();
		}
		else
		{
			// Construct all elements
			for( size_t n = 0; n < numElements; n++ )
				new (&tmp[n]) T();
		}
	}

	if( array )
	{
		size_t oldLength = length;

		if( array == tmp )
		{
			if( keepData )
			{
				if( length > numElements )
					length = numElements;
			}
			else
				length = 0;

			// Call the destructor for elements that are no longer used
			for( size_t n = length; n < oldLength; n++ )
				array[n].~T();
		}
		else
		{
			if( keepData )
			{
				if( length > numElements )
					length = numElements;

				for( size_t n = 0; n < length; n++ )
					tmp[n] = array[n];
			}
			else
				length = 0;

			// Call the destructor for all elements
			for( size_t n = 0; n < oldLength; n++ )
				array[n].~T();

			if( array != (T*)buf )
				asDELETEARRAY(array);
		}
	}

	array = tmp;
	maxLength = numElements;
}

template <class T>
size_t asCArray<T>::GetCapacity() const
{
	return maxLength;
}

template <class T>
void asCArray<T>::SetLength(size_t numElements)
{
	if( numElements > maxLength )
		Allocate(numElements, true);

	length = numElements;
}

template <class T>
void asCArray<T>::Copy(const T *data, size_t count)
{
	if( maxLength < count )
		Allocate(count, false);

	for( size_t n = 0; n < count; n++ )
		array[n] = data[n];

	length = count;
}

template <class T>
asCArray<T> &asCArray<T>::operator =(const asCArray<T> &copy)
{
	Copy(copy.array, copy.length);

	return *this;
}

template <class T>
bool asCArray<T>::operator ==(const asCArray<T> &other) const
{
	if( length != other.length ) return false;

	for( asUINT n = 0; n < length; n++ )
		if( array[n] != other.array[n] )
			return false;

	return true;
}

template <class T>
bool asCArray<T>::operator !=(const asCArray<T> &other) const
{
	return !(*this == other);
}

template <class T>
void asCArray<T>::Concatenate(const asCArray<T> &other)
{
	if( maxLength < length + other.length )
		Allocate(length + other.length, true);

	for( size_t n = 0; n < other.length; n++ )
		array[length+n] = other.array[n];

	length += other.length;
}

template <class T>
void asCArray<T>::Concatenate(T* array, unsigned int count)
{
	for( unsigned int c = 0; c < count; c++ )
		PushLast(array[c]);
}

template <class T>
bool asCArray<T>::Exists(const T &e)
{
	return IndexOf(e) == -1 ? false : true;
}

template <class T>
int asCArray<T>::IndexOf(const T &e)
{
	for( size_t n = 0; n < length; n++ )
		if( array[n] == e ) return (int)n;

	return -1;
}

template <class T>
void asCArray<T>::RemoveIndex(size_t index)
{
	if( index < length )
	{
		for( size_t n = index; n < length-1; n++ )
			array[n] = array[n+1];

		PopLast();
	}
}

template <class T>
void asCArray<T>::RemoveValue(const T &e)
{
	for( size_t n = 0; n < length; n++ )
	{
		if( array[n] == e )
		{
			RemoveIndex(n);
			break;
		}
	}
}

END_AS_NAMESPACE

#endif
