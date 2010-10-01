/*
   AngelCode Scripting Library
   Copyright (c) 2003-2006 Andreas Jönsson

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

   Andreas Jönsson
   andreas@angelcode.com
*/

#ifndef AS_ARRAY_H
#define AS_ARRAY_H

#include <memory.h>
#include <assert.h>

BEGIN_AS_NAMESPACE

template <class T> class asCArray
{
public:
	asCArray();
	asCArray(const asCArray<T> &);
	~asCArray();

	void   Allocate(asUINT numElements, bool keepData);
	asUINT GetCapacity() const;

	void PushLast(const T &element);
	T    PopLast();

	void   SetLength(asUINT numElements);
	asUINT GetLength() const;

	void Copy(const T*, asUINT count);
	asCArray<T> &operator =(const asCArray<T> &);

	T &operator [](asUINT index) const;
	T *AddressOf();

	void Concatenate(const asCArray<T> &);

	T* Find(const T &element);

protected:
	T   *array;
	asUINT  length;
	asUINT  maxLength;
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
asCArray<T>::~asCArray(void)
{
	if( array )
	{
		delete[] array;
		array = 0;
	}
}

template <class T>
asUINT asCArray<T>::GetLength() const
{
	return length;
}

template <class T>
T &asCArray<T>::operator [](asUINT index) const
{
	assert(index < length);

	return array[index];
}

template <class T>
void asCArray<T>::PushLast(const T &element)
{
	if( length == maxLength )
		Allocate(int(maxLength*1.5f) + 1, true);

	array[length++] = element;
}

template <class T>
T asCArray<T>::PopLast()
{
	assert(length > 0);

	return array[--length];
}

template <class T>
void asCArray<T>::Allocate(asUINT numElements, bool keepData)
{
	T *tmp = new T[numElements];

	if( array )
	{
		if( keepData )
		{
			if( length > numElements )
				length = numElements;

			for( asUINT n = 0; n < length; n++ )
				tmp[n] = array[n];
		}
		else
			length = 0;

		delete[] array;
	}

	array = tmp;
	maxLength = numElements;
}

template <class T>
asUINT asCArray<T>::GetCapacity() const
{
	return maxLength;
}

template <class T>
void asCArray<T>::SetLength(asUINT numElements)
{
	if( numElements > maxLength )
		Allocate(numElements, true);

	length = numElements;
}

template <class T>
void asCArray<T>::Copy(const T *data, asUINT count)
{
	if( maxLength < count )
		Allocate(count, false);

	for( asUINT n = 0; n < count; n++ )
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
void asCArray<T>::Concatenate(const asCArray<T> &other)
{
	if( maxLength < length + other.length )
		Allocate(length + other.length, true);

	for( asUINT n = 0; n < other.length; n++ )
		array[length+n] = other.array[n];

	length += other.length;
}

template <class T>
T *asCArray<T>::Find(const T &e)
{
	for( asUINT n = 0; n < length; n++ )
		if( array[n] == e ) return &array[n];

	return 0;
}

END_AS_NAMESPACE

#endif
