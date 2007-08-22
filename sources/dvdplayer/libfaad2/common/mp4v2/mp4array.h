/*
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 * 
 * The Original Code is MPEG4IP.
 * 
 * The Initial Developer of the Original Code is Cisco Systems Inc.
 * Portions created by Cisco Systems Inc. are
 * Copyright (C) Cisco Systems Inc. 2001.  All Rights Reserved.
 * 
 * Contributor(s): 
 *		Dave Mackie		dmackie@cisco.com
 */

#ifndef __MP4_ARRAY_INCLUDED__
#define __MP4_ARRAY_INCLUDED__

typedef u_int32_t MP4ArrayIndex;

class MP4Array {
public:
	MP4Array() {
		m_numElements = 0;
		m_maxNumElements = 0;
	}

	inline bool ValidIndex(MP4ArrayIndex index) {
		if (m_numElements == 0 || index > m_numElements - 1) {
			return false;
		}
		return true;
	}

	inline MP4ArrayIndex Size(void) {
		return m_numElements;
	}

	inline MP4ArrayIndex MaxSize(void) {
		return m_maxNumElements;
	}

protected:
	MP4ArrayIndex	m_numElements;
	MP4ArrayIndex	m_maxNumElements;
};

// macro to generate subclasses
// we use this as an alternative to templates
// due to the excessive compile time price of extensive template usage

#define MP4ARRAY_DECL(name, type) \
	class name##Array : public MP4Array { \
	public: \
		name##Array() { \
			m_elements = NULL; \
		} \
		\
		~name##Array() { \
			MP4Free(m_elements); \
		} \
		\
		inline void Add(type newElement) { \
			Insert(newElement, m_numElements); \
		} \
		\
		void Insert(type newElement, MP4ArrayIndex newIndex) { \
			if (newIndex > m_numElements) { \
				throw new MP4Error(ERANGE, "MP4Array::Insert"); \
			} \
			if (m_numElements == m_maxNumElements) { \
				m_maxNumElements = MAX(m_maxNumElements, 1) * 2; \
				m_elements = (type*)MP4Realloc(m_elements, \
					m_maxNumElements * sizeof(type)); \
			} \
			memmove(&m_elements[newIndex + 1], &m_elements[newIndex], \
				(m_numElements - newIndex) * sizeof(type)); \
			m_elements[newIndex] = newElement; \
			m_numElements++; \
		} \
		\
		void Delete(MP4ArrayIndex index) { \
			if (!ValidIndex(index)) { \
				throw new MP4Error(ERANGE, "MP4Array::Delete"); \
			} \
			memmove(&m_elements[index], &m_elements[index + 1], \
				(m_numElements - index) * sizeof(type)); \
			m_numElements--; \
		} \
		void Resize(MP4ArrayIndex newSize) { \
			m_numElements = newSize; \
			m_maxNumElements = newSize; \
			m_elements = (type*)MP4Realloc(m_elements, \
				m_maxNumElements * sizeof(type)); \
		} \
		\
		type& operator[](MP4ArrayIndex index) { \
			if (!ValidIndex(index)) { \
				throw new MP4Error(ERANGE, "MP4Array::[]"); \
			} \
			return m_elements[index]; \
		} \
		\
	protected: \
		type*	m_elements; \
	};

MP4ARRAY_DECL(MP4Integer8, u_int8_t)

MP4ARRAY_DECL(MP4Integer16, u_int16_t)

MP4ARRAY_DECL(MP4Integer32, u_int32_t)

MP4ARRAY_DECL(MP4Integer64, u_int64_t)

MP4ARRAY_DECL(MP4Float32, float)

MP4ARRAY_DECL(MP4Float64, double)

MP4ARRAY_DECL(MP4String, char*)

MP4ARRAY_DECL(MP4Bytes, u_int8_t*)

#endif /* __MP4_ARRAY_INCLUDED__ */
