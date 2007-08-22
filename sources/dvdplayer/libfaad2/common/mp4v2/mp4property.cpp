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

#include "mp4common.h"

MP4Property::MP4Property(const char* name)
{
	m_name = name;
	m_pParentAtom = NULL;
	m_readOnly = false;
	m_implicit = false;
}

bool MP4Property::FindProperty(const char* name, 
	MP4Property** ppProperty, u_int32_t* pIndex) 
{
	if (name == NULL) {
		return false;
	}

	if (!strcasecmp(m_name, name)) {
		if (m_pParentAtom) {
			VERBOSE_FIND(m_pParentAtom->GetFile()->GetVerbosity(),
				printf("FindProperty: matched %s\n", name));
		}

		*ppProperty = this;
		return true;
	}
	return false;
}

// Integer Property

u_int64_t MP4IntegerProperty::GetValue(u_int32_t index)
{
	switch (this->GetType()) {
	case Integer8Property:
		return ((MP4Integer8Property*)this)->GetValue(index);
	case Integer16Property:
		return ((MP4Integer16Property*)this)->GetValue(index);
	case Integer24Property:
		return ((MP4Integer24Property*)this)->GetValue(index);
	case Integer32Property:
		return ((MP4Integer32Property*)this)->GetValue(index);
	case Integer64Property:
		return ((MP4Integer64Property*)this)->GetValue(index);
	default:
		ASSERT(FALSE);
	}
	return (0);
}

void MP4IntegerProperty::SetValue(u_int64_t value, u_int32_t index)
{
	switch (this->GetType()) {
	case Integer8Property:
		((MP4Integer8Property*)this)->SetValue(value, index);
		break;
	case Integer16Property:
		((MP4Integer16Property*)this)->SetValue(value, index);
		break;
	case Integer24Property:
		((MP4Integer24Property*)this)->SetValue(value, index);
		break;
	case Integer32Property:
		((MP4Integer32Property*)this)->SetValue(value, index);
		break;
	case Integer64Property:
		((MP4Integer64Property*)this)->SetValue(value, index);
		break;
	default:
		ASSERT(FALSE);
	}
}

void MP4IntegerProperty::InsertValue(u_int64_t value, u_int32_t index)
{
	switch (this->GetType()) {
	case Integer8Property:
		((MP4Integer8Property*)this)->InsertValue(value, index);
		break;
	case Integer16Property:
		((MP4Integer16Property*)this)->InsertValue(value, index);
		break;
	case Integer24Property:
		((MP4Integer24Property*)this)->InsertValue(value, index);
		break;
	case Integer32Property:
		((MP4Integer32Property*)this)->InsertValue(value, index);
		break;
	case Integer64Property:
		((MP4Integer64Property*)this)->InsertValue(value, index);
		break;
	default:
		ASSERT(FALSE);
	}
}

void MP4IntegerProperty::DeleteValue(u_int32_t index)
{
	switch (this->GetType()) {
	case Integer8Property:
		((MP4Integer8Property*)this)->DeleteValue(index);
		break;
	case Integer16Property:
		((MP4Integer16Property*)this)->DeleteValue(index);
		break;
	case Integer24Property:
		((MP4Integer24Property*)this)->DeleteValue(index);
		break;
	case Integer32Property:
		((MP4Integer32Property*)this)->DeleteValue(index);
		break;
	case Integer64Property:
		((MP4Integer64Property*)this)->DeleteValue(index);
		break;
	default:
		ASSERT(FALSE);
	}
}

void MP4IntegerProperty::IncrementValue(int32_t increment, u_int32_t index)
{
	SetValue(GetValue() + increment);
}

void MP4Integer8Property::Dump(FILE* pFile, u_int8_t indent,
	bool dumpImplicits, u_int32_t index)
{
	if (m_implicit && !dumpImplicits) {
		return;
	}
	Indent(pFile, indent);
	fprintf(pFile, "%s = %u (0x%02x)\n", 
		m_name, m_values[index], m_values[index]);
}

void MP4Integer16Property::Dump(FILE* pFile, u_int8_t indent,
	bool dumpImplicits, u_int32_t index)
{
	if (m_implicit && !dumpImplicits) {
		return;
	}
	Indent(pFile, indent);
	fprintf(pFile, "%s = %u (0x%04x)\n", 
		m_name, m_values[index], m_values[index]);
}

void MP4Integer24Property::Dump(FILE* pFile, u_int8_t indent,
	bool dumpImplicits, u_int32_t index)
{
	if (m_implicit && !dumpImplicits) {
		return;
	}
	Indent(pFile, indent);
	fprintf(pFile, "%s = %u (0x%06x)\n", 
		m_name, m_values[index], m_values[index]);
}

void MP4Integer32Property::Dump(FILE* pFile, u_int8_t indent,
	bool dumpImplicits, u_int32_t index)
{
	if (m_implicit && !dumpImplicits) {
		return;
	}
	Indent(pFile, indent);
	fprintf(pFile, "%s = %u (0x%08x)\n", 
		m_name, m_values[index], m_values[index]);
}

void MP4Integer64Property::Dump(FILE* pFile, u_int8_t indent,
	bool dumpImplicits, u_int32_t index)
{
	if (m_implicit && !dumpImplicits) {
		return;
	}
	Indent(pFile, indent);
	fprintf(pFile, 
#ifdef _WIN32
		"%s = "LLU" (0x%016I64x)\n", 
#else
		"%s = "LLU" (0x%016llx)\n", 
#endif
		m_name, m_values[index], m_values[index]);
}

// MP4BitfieldProperty

void MP4BitfieldProperty::Read(MP4File* pFile, u_int32_t index)
{
	if (m_implicit) {
		return;
	}
	m_values[index] = pFile->ReadBits(m_numBits);
}

void MP4BitfieldProperty::Write(MP4File* pFile, u_int32_t index)
{
	if (m_implicit) {
		return;
	}
	pFile->WriteBits(m_values[index], m_numBits);
}

void MP4BitfieldProperty::Dump(FILE* pFile, u_int8_t indent,
	bool dumpImplicits, u_int32_t index)
{
	if (m_implicit && !dumpImplicits) {
		return;
	}
	Indent(pFile, indent);

	u_int8_t hexWidth = m_numBits / 4;
	if (hexWidth == 0 || (m_numBits % 4)) {
		hexWidth++;
	}
	fprintf(pFile, 
#ifdef _WIN32
		"%s = "LLU" (0x%0*I64x) <%u bits>\n", 
#else
		"%s = "LLU" (0x%0*llx) <%u bits>\n", 
#endif
		m_name, m_values[index], (int)hexWidth, m_values[index], m_numBits);
}

// MP4Float32Property

void MP4Float32Property::Read(MP4File* pFile, u_int32_t index) 
{
	if (m_implicit) {
		return;
	}
	if (m_useFixed16Format) {
		m_values[index] = pFile->ReadFixed16();
	} else if (m_useFixed32Format) {
		m_values[index] = pFile->ReadFixed32();
	} else {
		m_values[index] = pFile->ReadFloat();
	}
}

void MP4Float32Property::Write(MP4File* pFile, u_int32_t index) 
{
	if (m_implicit) {
		return;
	}
	if (m_useFixed16Format) {
		pFile->WriteFixed16(m_values[index]);
	} else if (m_useFixed32Format) {
		pFile->WriteFixed32(m_values[index]);
	} else {
		pFile->WriteFloat(m_values[index]);
	}
}

void MP4Float32Property::Dump(FILE* pFile, u_int8_t indent,
	bool dumpImplicits, u_int32_t index)
{
	if (m_implicit && !dumpImplicits) {
		return;
	}
	Indent(pFile, indent);
	fprintf(pFile, "%s = %f\n", 
		m_name, m_values[index]);
}

// MP4StringProperty

MP4StringProperty::MP4StringProperty(char* name, 
	bool useCountedFormat, bool useUnicode)
	: MP4Property(name)
{
	SetCount(1);
	m_values[0] = NULL;
	m_useCountedFormat = useCountedFormat;
	m_useExpandedCount = false;
	m_useUnicode = useUnicode;
	m_fixedLength = 0;	// length not fixed
}

MP4StringProperty::~MP4StringProperty() 
{
	u_int32_t count = GetCount();
	for (u_int32_t i = 0; i < count; i++) {
		MP4Free(m_values[i]);
	}
}

void MP4StringProperty::SetCount(u_int32_t count) 
{
	u_int32_t oldCount = m_values.Size();

	m_values.Resize(count);

	for (u_int32_t i = oldCount; i < count; i++) {
		m_values[i] = NULL;
	}
}

void MP4StringProperty::SetValue(const char* value, u_int32_t index) 
{
	if (m_readOnly) {
		throw new MP4Error(EACCES, "property is read-only", m_name);
	}

	MP4Free(m_values[index]);

	if (m_fixedLength) {
		m_values[index] = (char*)MP4Calloc(m_fixedLength + 1);
		if (value) {
			strncpy(m_values[index], value, m_fixedLength);
		}
	} else {
		if (value) {
			m_values[index] = MP4Stralloc(value);
		} else {
			m_values[index] = NULL;
		}
	}
}

void MP4StringProperty::Read(MP4File* pFile, u_int32_t index)
{
	if (m_implicit) {
		return;
	}
	if (m_useCountedFormat) {
		m_values[index] = pFile->ReadCountedString(
			(m_useUnicode ? 2 : 1), m_useExpandedCount);
	} else if (m_fixedLength) {
		MP4Free(m_values[index]);
		m_values[index] = (char*)MP4Calloc(m_fixedLength + 1);
		pFile->ReadBytes((u_int8_t*)m_values[index], m_fixedLength);
	} else {
		m_values[index] = pFile->ReadString();
	}
}

void MP4StringProperty::Write(MP4File* pFile, u_int32_t index)
{
	if (m_implicit) {
		return;
	}
	if (m_useCountedFormat) {
		pFile->WriteCountedString(m_values[index],
			(m_useUnicode ? 2 : 1), m_useExpandedCount);
	} else if (m_fixedLength) {
		pFile->WriteBytes((u_int8_t*)m_values[index], m_fixedLength);
	} else {
		pFile->WriteString(m_values[index]);
	}
}

void MP4StringProperty::Dump(FILE* pFile, u_int8_t indent,
	bool dumpImplicits, u_int32_t index)
{
	if (m_implicit && !dumpImplicits) {
		return;
	}
	Indent(pFile, indent);
	if (m_useUnicode) {
		fprintf(pFile, "%s = %ls\n", m_name, (wchar_t*)m_values[index]);
	} else {
		fprintf(pFile, "%s = %s\n", m_name, m_values[index]);
	}
}

// MP4BytesProperty

MP4BytesProperty::MP4BytesProperty(char* name, u_int32_t valueSize)
	: MP4Property(name)
{
	SetCount(1);
	m_values[0] = (u_int8_t*)MP4Calloc(valueSize);
	m_valueSizes[0] = valueSize;
	m_fixedValueSize = 0;
}

MP4BytesProperty::~MP4BytesProperty() 
{
	u_int32_t count = GetCount();
	for (u_int32_t i = 0; i < count; i++) {
		MP4Free(m_values[i]);
	}
}
  
void MP4BytesProperty::SetCount(u_int32_t count) 
{
	u_int32_t oldCount = m_values.Size();

	m_values.Resize(count);
	m_valueSizes.Resize(count);

	for (u_int32_t i = oldCount; i < count; i++) {
		m_values[i] = NULL;
		m_valueSizes[i] = 0;
	}
}

void MP4BytesProperty::SetValue(const u_int8_t* pValue, u_int32_t valueSize, 
	u_int32_t index) 
{
	if (m_readOnly) {
		throw new MP4Error(EACCES, "property is read-only", m_name);
	}
	if (m_fixedValueSize) {
		if (valueSize > m_fixedValueSize) {
			throw new MP4Error("value size exceeds fixed value size",
				"MP4BytesProperty::SetValue");
		}
		if (m_values[index] == NULL) {
			m_values[index] = (u_int8_t*)MP4Calloc(m_fixedValueSize);
			m_valueSizes[index] = m_fixedValueSize;
		}
		if (pValue) {
			memcpy(m_values[index], pValue, valueSize);
		}
	} else {
		MP4Free(m_values[index]);
		if (pValue) {
			m_values[index] = (u_int8_t*)MP4Malloc(valueSize);
			memcpy(m_values[index], pValue, valueSize);
			m_valueSizes[index] = valueSize;
		} else {
			m_values[index] = NULL;
			m_valueSizes[index] = 0;
		}
	}
}

void MP4BytesProperty::SetValueSize(u_int32_t valueSize, u_int32_t index) 
{
	if (m_fixedValueSize) {
		throw new MP4Error("can't change size of fixed sized property",
			"MP4BytesProperty::SetValueSize");
	}
	if (m_values[index] != NULL) {
		m_values[index] = (u_int8_t*)MP4Realloc(m_values[index], valueSize);
	}
	m_valueSizes[index] = valueSize;
}

void MP4BytesProperty::SetFixedSize(u_int32_t fixedSize) 
{
	m_fixedValueSize = 0;
	for (u_int32_t i = 0; i < GetCount(); i++) {
		SetValueSize(fixedSize, i);
	}
	m_fixedValueSize = fixedSize;
}

void MP4BytesProperty::Read(MP4File* pFile, u_int32_t index)
{
	if (m_implicit) {
		return;
	}
	MP4Free(m_values[index]);
	m_values[index] = (u_int8_t*)MP4Malloc(m_valueSizes[index]);
	pFile->ReadBytes(m_values[index], m_valueSizes[index]);
}

void MP4BytesProperty::Write(MP4File* pFile, u_int32_t index)
{
	if (m_implicit) {
		return;
	}
	pFile->WriteBytes(m_values[index], m_valueSizes[index]);
}

void MP4BytesProperty::Dump(FILE* pFile, u_int8_t indent,
	bool dumpImplicits, u_int32_t index)
{
	if (m_implicit && !dumpImplicits) {
		return;
	}
	Indent(pFile, indent);
	fprintf(pFile, "%s = <%u bytes> ", m_name, m_valueSizes[index]);
	for (u_int32_t i = 0; i < m_valueSizes[index]; i++) {
		if ((i % 16) == 0 && m_valueSizes[index] > 16) {
			fprintf(pFile, "\n");
			Indent(pFile, indent);
		}
		fprintf(pFile, "%02x ", m_values[index][i]);
	}
	fprintf(pFile, "\n");
}

// MP4TableProperty

MP4TableProperty::MP4TableProperty(char* name, MP4Property* pCountProperty)
	: MP4Property(name) 
{
	ASSERT(pCountProperty->GetType() == Integer8Property
		|| pCountProperty->GetType() == Integer32Property);
	m_pCountProperty = pCountProperty;
	m_pCountProperty->SetReadOnly();
}

MP4TableProperty::~MP4TableProperty()
{
	for (u_int32_t i = 0; i < m_pProperties.Size(); i++) {
		delete m_pProperties[i];
	}
}

void MP4TableProperty::AddProperty(MP4Property* pProperty) 
{
	ASSERT(pProperty);
	ASSERT(pProperty->GetType() != TableProperty);
	ASSERT(pProperty->GetType() != DescriptorProperty);
	m_pProperties.Add(pProperty);
	pProperty->SetParentAtom(m_pParentAtom);
	pProperty->SetCount(0);
}

bool MP4TableProperty::FindProperty(const char *name,
	MP4Property** ppProperty, u_int32_t* pIndex)
{
	ASSERT(m_name);

	// check if first component of name matches ourselves
	if (!MP4NameFirstMatches(m_name, name)) {
		return false;
	}

	// check if the specified table entry exists
	u_int32_t index;
	bool haveIndex = MP4NameFirstIndex(name, &index);
	if (haveIndex) {
		if (index >= GetCount()) {
			return false;
		}
		if (pIndex) {
			*pIndex = index;
		}
	}

	VERBOSE_FIND(m_pParentAtom->GetFile()->GetVerbosity(),
		printf("FindProperty: matched %s\n", name));

	// get name of table property
	const char *tablePropName = MP4NameAfterFirst(name);
	if (tablePropName == NULL) {
		if (!haveIndex) {
			*ppProperty = this;
			return true;
		}
		return false;
	}

	// check if this table property exists
	return FindContainedProperty(tablePropName, ppProperty, pIndex);
}

bool MP4TableProperty::FindContainedProperty(const char *name,
	MP4Property** ppProperty, u_int32_t* pIndex)
{
	u_int32_t numProperties = m_pProperties.Size();

	for (u_int32_t i = 0; i < numProperties; i++) {
		if (m_pProperties[i]->FindProperty(name, ppProperty, pIndex)) {
			return true;
		}
	}
	return false;
}

void MP4TableProperty::Read(MP4File* pFile, u_int32_t index)
{
	ASSERT(index == 0);

	if (m_implicit) {
		return;
	}

	u_int32_t numProperties = m_pProperties.Size();

	if (numProperties == 0) {
		WARNING(numProperties == 0);
		return;
	}

	u_int32_t numEntries = GetCount();

	/* for each property set size */
	for (u_int32_t j = 0; j < numProperties; j++) {
		m_pProperties[j]->SetCount(numEntries);
	}

	for (u_int32_t i = 0; i < numEntries; i++) {
		ReadEntry(pFile, i);
	}
}

void MP4TableProperty::ReadEntry(MP4File* pFile, u_int32_t index)
{
	for (u_int32_t j = 0; j < m_pProperties.Size(); j++) {
		m_pProperties[j]->Read(pFile, index);
	}
}

void MP4TableProperty::Write(MP4File* pFile, u_int32_t index)
{
	ASSERT(index == 0);

	if (m_implicit) {
		return;
	}

	u_int32_t numProperties = m_pProperties.Size();

	if (numProperties == 0) {
		WARNING(numProperties == 0);
		return;
	}

	u_int32_t numEntries = GetCount();

	ASSERT(m_pProperties[0]->GetCount() == numEntries);

	for (u_int32_t i = 0; i < numEntries; i++) {
		WriteEntry(pFile, i);
	}
}

void MP4TableProperty::WriteEntry(MP4File* pFile, u_int32_t index)
{
	for (u_int32_t j = 0; j < m_pProperties.Size(); j++) {
		m_pProperties[j]->Write(pFile, index);
	}
}

void MP4TableProperty::Dump(FILE* pFile, u_int8_t indent,
	bool dumpImplicits, u_int32_t index)
{
	ASSERT(index == 0);

	// implicit tables just can't be dumped
	if (m_implicit) {
		return;
	}

	u_int32_t numProperties = m_pProperties.Size();

	if (numProperties == 0) {
		WARNING(numProperties == 0);
		return;
	}

	u_int32_t numEntries = GetCount();

	for (u_int32_t i = 0; i < numEntries; i++) {
		for (u_int32_t j = 0; j < numProperties; j++) {
			m_pProperties[j]->Dump(pFile, indent + 1, dumpImplicits, i);
		}
	}
}

// MP4DescriptorProperty
  
MP4DescriptorProperty::MP4DescriptorProperty(char* name, 
	u_int8_t tagsStart, u_int8_t tagsEnd, bool mandatory, bool onlyOne)
	: MP4Property(name) 
{ 
	SetTags(tagsStart, tagsEnd);
	m_sizeLimit = 0;
	m_mandatory = mandatory;
	m_onlyOne = onlyOne;
}

MP4DescriptorProperty::~MP4DescriptorProperty() 
{
	for (u_int32_t i = 0; i < m_pDescriptors.Size(); i++) {
		delete m_pDescriptors[i];
	}
}

void MP4DescriptorProperty::SetParentAtom(MP4Atom* pParentAtom) {
	m_pParentAtom = pParentAtom;
	for (u_int32_t i = 0; i < m_pDescriptors.Size(); i++) {
		m_pDescriptors[i]->SetParentAtom(pParentAtom);
	}
}

MP4Descriptor* MP4DescriptorProperty::AddDescriptor(u_int8_t tag)
{
	// check that tag is in expected range
	ASSERT(tag >= m_tagsStart && tag <= m_tagsEnd);

	MP4Descriptor* pDescriptor = CreateDescriptor(tag);
	ASSERT(pDescriptor);

	m_pDescriptors.Add(pDescriptor);
	pDescriptor->SetParentAtom(m_pParentAtom);

	return pDescriptor;
}

void MP4DescriptorProperty::DeleteDescriptor(u_int32_t index)
{
	delete m_pDescriptors[index];
	m_pDescriptors.Delete(index);
}

void MP4DescriptorProperty::Generate()
{
	// generate a default descriptor
	// if it is mandatory, and single
	if (m_mandatory && m_onlyOne) {
		MP4Descriptor* pDescriptor = 
			AddDescriptor(m_tagsStart);
		pDescriptor->Generate();
	}
}

bool MP4DescriptorProperty::FindProperty(const char *name,
	MP4Property** ppProperty, u_int32_t* pIndex)
{
	// we're unnamed, so just check contained properties
	if (m_name == NULL || !strcmp(m_name, "")) {
		return FindContainedProperty(name, ppProperty, pIndex);
	}

	// check if first component of name matches ourselves
	if (!MP4NameFirstMatches(m_name, name)) {
		return false;
	}

	// check if the specific descriptor entry exists
	u_int32_t descrIndex;
	bool haveDescrIndex = MP4NameFirstIndex(name, &descrIndex);

	if (haveDescrIndex && descrIndex >= GetCount()) {
		return false;
	}

	if (m_pParentAtom) {
		VERBOSE_FIND(m_pParentAtom->GetFile()->GetVerbosity(),
			printf("FindProperty: matched %s\n", name));
	}

	// get name of descriptor property
	name = MP4NameAfterFirst(name);
	if (name == NULL) {
		if (!haveDescrIndex) {
			*ppProperty = this;
			return true;
		}
		return false;
	}

	/* check rest of name */
	if (haveDescrIndex) {
		return m_pDescriptors[descrIndex]->FindProperty(name, 
			ppProperty, pIndex); 
	} else {
		return FindContainedProperty(name, ppProperty, pIndex);
	}
}

bool MP4DescriptorProperty::FindContainedProperty(const char *name,
	MP4Property** ppProperty, u_int32_t* pIndex)
{
	for (u_int32_t i = 0; i < m_pDescriptors.Size(); i++) {
		if (m_pDescriptors[i]->FindProperty(name, ppProperty, pIndex)) {
			return true;
		}
	}
	return false;
}

void MP4DescriptorProperty::Read(MP4File* pFile, u_int32_t index)
{
	ASSERT(index == 0);

	if (m_implicit) {
		return;
	}

	u_int64_t start = pFile->GetPosition();

	while (true) {
		// enforce size limitation
		if (m_sizeLimit && pFile->GetPosition() >= start + m_sizeLimit) {
			break;
		}

		u_int8_t tag;
		try {
			pFile->PeekBytes(&tag, 1);
		}
		catch (MP4Error* e) {
			if (pFile->GetPosition() >= pFile->GetSize()) {
				// EOF
				delete e;
				break;
			}
			throw e;
		}

		// check if tag is in desired range
		if (tag < m_tagsStart || tag > m_tagsEnd) {
			break;
		}

		MP4Descriptor* pDescriptor = 
			AddDescriptor(tag);

		pDescriptor->Read(pFile);
	}

	// warnings
	if (m_mandatory && m_pDescriptors.Size() == 0) {
		VERBOSE_READ(pFile->GetVerbosity(),
			printf("Warning: Mandatory descriptor 0x%02x missing\n",
				m_tagsStart));
	} else if (m_onlyOne && m_pDescriptors.Size() > 1) {
		VERBOSE_READ(pFile->GetVerbosity(),
			printf("Warning: Descriptor 0x%02x has more than one instance\n",
				m_tagsStart));
	}
}

void MP4DescriptorProperty::Write(MP4File* pFile, u_int32_t index)
{
	ASSERT(index == 0);

	if (m_implicit) {
		return;
	}

	for (u_int32_t i = 0; i < m_pDescriptors.Size(); i++) {
		m_pDescriptors[i]->Write(pFile);
	}
}

void MP4DescriptorProperty::Dump(FILE* pFile, u_int8_t indent,
	bool dumpImplicits, u_int32_t index)
{
	ASSERT(index == 0);

	if (m_implicit && !dumpImplicits) {
		return;
	}

	if (m_name) {
		Indent(pFile, indent);
		fprintf(pFile, "%s\n", m_name);
		indent++;
	}

	for (u_int32_t i = 0; i < m_pDescriptors.Size(); i++) {
		m_pDescriptors[i]->Dump(pFile, indent, dumpImplicits);
	}
}

