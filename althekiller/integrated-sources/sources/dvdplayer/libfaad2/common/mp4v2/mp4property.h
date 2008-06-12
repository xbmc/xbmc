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

#ifndef __MP4_PROPERTY_INCLUDED__
#define __MP4_PROPERTY_INCLUDED__

// forward declarations
class MP4Atom;

class MP4Descriptor;
MP4ARRAY_DECL(MP4Descriptor, MP4Descriptor*);

enum MP4PropertyType {
	Integer8Property,
	Integer16Property,
	Integer24Property,
	Integer32Property,
	Integer64Property,
	Float32Property,
	StringProperty,
	BytesProperty,
	TableProperty,
	DescriptorProperty,
};

class MP4Property {
public:
	MP4Property(const char *name = NULL);

	virtual ~MP4Property() { }

	MP4Atom* GetParentAtom() {
		return m_pParentAtom;
	}
	virtual void SetParentAtom(MP4Atom* pParentAtom) {
		m_pParentAtom = pParentAtom;
	}

	const char *GetName() {
		return m_name;
	}

	virtual MP4PropertyType GetType() = 0; 

	bool IsReadOnly() {
		return m_readOnly;
	}
	void SetReadOnly(bool value = true) {
		m_readOnly = value;
	}

	bool IsImplicit() {
		return m_implicit;
	}
	void SetImplicit(bool value = true) {
		m_implicit = value;
	}

	virtual u_int32_t GetCount() = 0;
	virtual void SetCount(u_int32_t count) = 0;

	virtual void Generate() { /* default is a no-op */ };

	virtual void Read(MP4File* pFile, u_int32_t index = 0) = 0;

	virtual void Write(MP4File* pFile, u_int32_t index = 0) = 0;

	virtual void Dump(FILE* pFile, u_int8_t indent,
		bool dumpImplicits, u_int32_t index = 0) = 0;

	virtual bool FindProperty(const char* name,
		MP4Property** ppProperty, u_int32_t* pIndex = NULL);

protected:
	MP4Atom* m_pParentAtom;
	const char* m_name;
	bool m_readOnly;
	bool m_implicit;
};

MP4ARRAY_DECL(MP4Property, MP4Property*);

class MP4IntegerProperty : public MP4Property {
protected:
	MP4IntegerProperty(char* name)
		: MP4Property(name) { };

public:
	u_int64_t GetValue(u_int32_t index = 0);

	void SetValue(u_int64_t value, u_int32_t index = 0);

	void InsertValue(u_int64_t value, u_int32_t index = 0);

	void DeleteValue(u_int32_t index = 0);

	void IncrementValue(int32_t increment = 1, u_int32_t index = 0);
};

#define MP4INTEGER_PROPERTY_DECL2(isize, xsize) \
	class MP4Integer##xsize##Property : public MP4IntegerProperty { \
	public: \
		MP4Integer##xsize##Property(char* name) \
			: MP4IntegerProperty(name) { \
			SetCount(1); \
			m_values[0] = 0; \
		} \
		\
		MP4PropertyType GetType() { \
			return Integer##xsize##Property; \
		} \
		\
		u_int32_t GetCount() { \
			return m_values.Size(); \
		} \
		void SetCount(u_int32_t count) { \
			m_values.Resize(count); \
		} \
		\
		u_int##isize##_t GetValue(u_int32_t index = 0) { \
			return m_values[index]; \
		} \
		\
		void SetValue(u_int##isize##_t value, u_int32_t index = 0) { \
			if (m_readOnly) { \
				throw new MP4Error(EACCES, "property is read-only", m_name); \
			} \
			m_values[index] = value; \
		} \
		void AddValue(u_int##isize##_t value) { \
			m_values.Add(value); \
		} \
		void InsertValue(u_int##isize##_t value, u_int32_t index) { \
			m_values.Insert(value, index); \
		} \
		void DeleteValue(u_int32_t index) { \
			m_values.Delete(index); \
		} \
		void IncrementValue(int32_t increment = 1, u_int32_t index = 0) { \
			m_values[index] += increment; \
		} \
		void Read(MP4File* pFile, u_int32_t index = 0) { \
			if (m_implicit) { \
				return; \
			} \
			m_values[index] = pFile->ReadUInt##xsize(); \
		} \
		\
		void Write(MP4File* pFile, u_int32_t index = 0) { \
			if (m_implicit) { \
				return; \
			} \
			pFile->WriteUInt##xsize(m_values[index]); \
		} \
		void Dump(FILE* pFile, u_int8_t indent, \
			bool dumpImplicits, u_int32_t index = 0); \
	\
	protected: \
		MP4Integer##isize##Array m_values; \
	};

#define MP4INTEGER_PROPERTY_DECL(size) \
	MP4INTEGER_PROPERTY_DECL2(size, size)

MP4INTEGER_PROPERTY_DECL(8);
MP4INTEGER_PROPERTY_DECL(16);
MP4INTEGER_PROPERTY_DECL2(32, 24);
MP4INTEGER_PROPERTY_DECL(32);
MP4INTEGER_PROPERTY_DECL(64);

class MP4BitfieldProperty : public MP4Integer64Property {
public:
	MP4BitfieldProperty(char* name, u_int8_t numBits)
		: MP4Integer64Property(name) {
		ASSERT(numBits != 0);
		ASSERT(numBits <= 64);
		m_numBits = numBits;
	}

	u_int8_t GetNumBits() {
		return m_numBits;
	}
	void SetNumBits(u_int8_t numBits) {
		m_numBits = numBits;
	}

	void Read(MP4File* pFile, u_int32_t index = 0);
	void Write(MP4File* pFile, u_int32_t index = 0);
	void Dump(FILE* pFile, u_int8_t indent,
		 bool dumpImplicits, u_int32_t index = 0);

protected:
	u_int8_t m_numBits;
};

class MP4Float32Property : public MP4Property {
public:
	MP4Float32Property(char* name)
		: MP4Property(name) {
		m_useFixed16Format = false;
		m_useFixed32Format = false;
		SetCount(1);
		m_values[0] = 0.0;
	}

	MP4PropertyType GetType() {
		return Float32Property;
	}

	u_int32_t GetCount() {
		return m_values.Size();
	}
	void SetCount(u_int32_t count) {
		m_values.Resize(count);
	}

	float GetValue(u_int32_t index = 0) {
		return m_values[index];
	}

	void SetValue(float value, u_int32_t index = 0) {
		if (m_readOnly) {
			throw new MP4Error(EACCES, "property is read-only", m_name);
		}
		m_values[index] = value;
	}

	void AddValue(float value) {
		m_values.Add(value);
	}

	void InsertValue(float value, u_int32_t index) {
		m_values.Insert(value, index);
	}

	bool IsFixed16Format() {
		return m_useFixed16Format;
	}

	void SetFixed16Format(bool useFixed16Format = true) {
		m_useFixed16Format = useFixed16Format;
	}

	bool IsFixed32Format() {
		return m_useFixed32Format;
	}

	void SetFixed32Format(bool useFixed32Format = true) {
		m_useFixed32Format = useFixed32Format;
	}

	void Read(MP4File* pFile, u_int32_t index = 0);
	void Write(MP4File* pFile, u_int32_t index = 0);
	void Dump(FILE* pFile, u_int8_t indent,
		 bool dumpImplicits, u_int32_t index = 0);

protected:
	bool m_useFixed16Format;
	bool m_useFixed32Format;
	MP4Float32Array m_values;
};

class MP4StringProperty : public MP4Property {
public:
	MP4StringProperty(char* name, 
	  bool useCountedFormat = false, bool useUnicode = false);

	~MP4StringProperty();

	MP4PropertyType GetType() {
		return StringProperty;
	}

	u_int32_t GetCount() {
		return m_values.Size();
	}

	void SetCount(u_int32_t count);

	const char* GetValue(u_int32_t index = 0) {
		return m_values[index];
	}

	void SetValue(const char* value, u_int32_t index = 0);

	void AddValue(char* value) {
		u_int32_t count = GetCount();
		SetCount(count + 1); 
		SetValue(value, count);
	}

	bool IsCountedFormat() {
		return m_useCountedFormat;
	}

	void SetCountedFormat(bool useCountedFormat) {
		m_useCountedFormat = useCountedFormat;
	}

	bool IsExpandedCountedFormat() {
		return m_useExpandedCount;
	}

	void SetExpandedCountedFormat(bool useExpandedCount) {
		m_useExpandedCount = useExpandedCount;
	}

	bool IsUnicode() {
		return m_useUnicode;
	}

	void SetUnicode(bool useUnicode) {
		m_useUnicode = useUnicode;
	}

	u_int32_t GetFixedLength() {
		return m_fixedLength;
	}

	void SetFixedLength(u_int32_t fixedLength) {
		m_fixedLength = fixedLength;
	}

	void Read(MP4File* pFile, u_int32_t index = 0);
	void Write(MP4File* pFile, u_int32_t index = 0);
	void Dump(FILE* pFile, u_int8_t indent,
		 bool dumpImplicits, u_int32_t index = 0);

protected:
	bool m_useCountedFormat;
	bool m_useExpandedCount;
	bool m_useUnicode;
	u_int32_t m_fixedLength;

	MP4StringArray m_values;
};

class MP4BytesProperty : public MP4Property {
public:
	MP4BytesProperty(char* name, u_int32_t valueSize = 0);

	~MP4BytesProperty();

	MP4PropertyType GetType() {
		return BytesProperty;
	}

	u_int32_t GetCount() {
		return m_values.Size();
	}

	void SetCount(u_int32_t count);

	void GetValue(u_int8_t** ppValue, u_int32_t* pValueSize, 
	  u_int32_t index = 0) {
		// N.B. caller must free memory
		*ppValue = (u_int8_t*)MP4Malloc(m_valueSizes[index]);
		memcpy(*ppValue, m_values[index], m_valueSizes[index]);
		*pValueSize = m_valueSizes[index];
	}

	void CopyValue(u_int8_t* pValue, u_int32_t index = 0) {
		// N.B. caller takes responsbility for valid pointer
		// and sufficient memory at the destination
		memcpy(pValue, m_values[index], m_valueSizes[index]);
	}

	void SetValue(const u_int8_t* pValue, u_int32_t valueSize, 
		u_int32_t index = 0);

	void AddValue(u_int8_t* pValue, u_int32_t valueSize) {
		u_int32_t count = GetCount();
		SetCount(count + 1); 
		SetValue(pValue, valueSize, count);
	}

	u_int32_t GetValueSize(u_int32_t valueSize, u_int32_t index = 0) {
		return m_valueSizes[index];
	}

	void SetValueSize(u_int32_t valueSize, u_int32_t index = 0);

	u_int32_t GetFixedSize() {
		return m_fixedValueSize;
	}

	void SetFixedSize(u_int32_t fixedSize);

	void Read(MP4File* pFile, u_int32_t index = 0);
	void Write(MP4File* pFile, u_int32_t index = 0);
	void Dump(FILE* pFile, u_int8_t indent,
		 bool dumpImplicits, u_int32_t index = 0);

protected:
	u_int32_t			m_fixedValueSize;
	MP4Integer32Array	m_valueSizes;
	MP4BytesArray		m_values;
};

class MP4TableProperty : public MP4Property {
public:
	MP4TableProperty(char* name, MP4Property* pCountProperty);

	~MP4TableProperty();

	MP4PropertyType GetType() {
		return TableProperty;
	}

	void SetParentAtom(MP4Atom* pParentAtom) {
		m_pParentAtom = pParentAtom;
		for (u_int32_t i = 0; i < m_pProperties.Size(); i++) {
			m_pProperties[i]->SetParentAtom(pParentAtom);
		}
	}

	void AddProperty(MP4Property* pProperty);

	MP4Property* GetProperty(u_int32_t index) {
		return m_pProperties[index];
	}

	u_int32_t GetCount() {
		if (m_pCountProperty->GetType() == Integer8Property) {
			return ((MP4Integer8Property*)m_pCountProperty)->GetValue();
		} else {
			return ((MP4Integer32Property*)m_pCountProperty)->GetValue();
		} 
	}
	void SetCount(u_int32_t count) {
		if (m_pCountProperty->GetType() == Integer8Property) {
			((MP4Integer8Property*)m_pCountProperty)->SetValue(count);
		} else {
			((MP4Integer32Property*)m_pCountProperty)->SetValue(count);
		} 
	}

	void Read(MP4File* pFile, u_int32_t index = 0);
	void Write(MP4File* pFile, u_int32_t index = 0);
	void Dump(FILE* pFile, u_int8_t indent,
		 bool dumpImplicits, u_int32_t index = 0);

	bool FindProperty(const char* name,
		MP4Property** ppProperty, u_int32_t* pIndex = NULL);

protected:
	virtual void ReadEntry(MP4File* pFile, u_int32_t index);
	virtual void WriteEntry(MP4File* pFile, u_int32_t index);

	bool FindContainedProperty(const char* name,
		MP4Property** ppProperty, u_int32_t* pIndex);

protected:
	MP4Property*		m_pCountProperty;
	MP4PropertyArray	m_pProperties;
};

class MP4DescriptorProperty : public MP4Property {
public:
	MP4DescriptorProperty(char* name = NULL, 
	  u_int8_t tagsStart = 0, u_int8_t tagsEnd = 0,
	  bool mandatory = false, bool onlyOne = false);

	~MP4DescriptorProperty();

	MP4PropertyType GetType() {
		return DescriptorProperty;
	}

	void SetParentAtom(MP4Atom* pParentAtom);

	void SetSizeLimit(u_int64_t sizeLimit) {
		m_sizeLimit = sizeLimit;
	}

	u_int32_t GetCount() {
		return m_pDescriptors.Size();
	}
	void SetCount(u_int32_t count) {
		m_pDescriptors.Resize(count);
	}

	void SetTags(u_int8_t tagsStart, u_int8_t tagsEnd = 0) {
		m_tagsStart = tagsStart;
		m_tagsEnd = tagsEnd ? tagsEnd : tagsStart;
	}

	MP4Descriptor* AddDescriptor(u_int8_t tag);

	void AppendDescriptor(MP4Descriptor* pDescriptor) {
		m_pDescriptors.Add(pDescriptor);
	}

	void DeleteDescriptor(u_int32_t index);

	void Generate();
	void Read(MP4File* pFile, u_int32_t index = 0);
	void Write(MP4File* pFile, u_int32_t index = 0);
	void Dump(FILE* pFile, u_int8_t indent,
		 bool dumpImplicits, u_int32_t index = 0);

	bool FindProperty(const char* name,
		MP4Property** ppProperty, u_int32_t* pIndex = NULL);

protected:
	virtual MP4Descriptor* CreateDescriptor(u_int8_t tag);

	bool FindContainedProperty(const char* name,
		MP4Property** ppProperty, u_int32_t* pIndex);

protected:
	u_int8_t			m_tagsStart;
	u_int8_t			m_tagsEnd;
	u_int64_t			m_sizeLimit;
	bool				m_mandatory;
	bool				m_onlyOne;
	MP4DescriptorArray	m_pDescriptors;
};

class MP4QosQualifierProperty : public MP4DescriptorProperty {
public:
	MP4QosQualifierProperty(char* name = NULL, 
	  u_int8_t tagsStart = 0, u_int8_t tagsEnd = 0,
	  bool mandatory = false, bool onlyOne = false) :
	MP4DescriptorProperty(name, tagsStart, tagsEnd, mandatory, onlyOne) { }

protected:
	MP4Descriptor* CreateDescriptor(u_int8_t tag);
};

#endif /* __MP4_PROPERTY_INCLUDED__ */
