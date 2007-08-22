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

MP4Container::~MP4Container()
{
	for (u_int32_t i = 0; i < m_pProperties.Size(); i++) {
		delete m_pProperties[i];
	}
}

void MP4Container::AddProperty(MP4Property* pProperty) 
{
	ASSERT(pProperty);
	m_pProperties.Add(pProperty);
}

bool MP4Container::FindProperty(const char *name,
	MP4Property** ppProperty, u_int32_t* pIndex)
{
	if (pIndex) {
		*pIndex = 0;	// set the default answer for index
	}

	u_int32_t numProperties = m_pProperties.Size();

	for (u_int32_t i = 0; i < numProperties; i++) {
		if (m_pProperties[i]->FindProperty(name, ppProperty, pIndex)) { 
			return true;
		}
	}
	return false;
}

void MP4Container::FindIntegerProperty(const char* name, 
	MP4Property** ppProperty, u_int32_t* pIndex)
{
	if (!FindProperty(name, ppProperty, pIndex)) {
		throw new MP4Error("no such property", 
			"MP4Container::FindIntegerProperty");
	}

	switch ((*ppProperty)->GetType()) {
	case Integer8Property:
	case Integer16Property:
	case Integer24Property:
	case Integer32Property:
	case Integer64Property:
		break;
	default:
		throw new MP4Error("type mismatch", 
			"MP4Container::FindIntegerProperty");
	}
}

u_int64_t MP4Container::GetIntegerProperty(const char* name)
{
	MP4Property* pProperty;
	u_int32_t index;

	FindIntegerProperty(name, &pProperty, &index);

	return ((MP4IntegerProperty*)pProperty)->GetValue(index);
}

void MP4Container::SetIntegerProperty(const char* name, u_int64_t value)
{
	MP4Property* pProperty = NULL;
	u_int32_t index = 0;

	FindIntegerProperty(name, &pProperty, &index);

	((MP4IntegerProperty*)pProperty)->SetValue(value, index);
}

void MP4Container::FindFloatProperty(const char* name, 
	MP4Property** ppProperty, u_int32_t* pIndex)
{
	if (!FindProperty(name, ppProperty, pIndex)) {
		throw new MP4Error("no such property",
			 "MP4Container::FindFloatProperty");
	}
	if ((*ppProperty)->GetType() != Float32Property) {
		throw new MP4Error("type mismatch", 
			"MP4Container::FindFloatProperty");
	}
}

float MP4Container::GetFloatProperty(const char* name)
{
	MP4Property* pProperty;
	u_int32_t index;

	FindFloatProperty(name, &pProperty, &index);

	return ((MP4Float32Property*)pProperty)->GetValue(index);
}

void MP4Container::SetFloatProperty(const char* name, float value)
{
	MP4Property* pProperty;
	u_int32_t index;

	FindFloatProperty(name, &pProperty, &index);

	((MP4Float32Property*)pProperty)->SetValue(value, index);
}

void MP4Container::FindStringProperty(const char* name, 
	MP4Property** ppProperty, u_int32_t* pIndex)
{
	if (!FindProperty(name, ppProperty, pIndex)) {
		throw new MP4Error("no such property",
			"MP4Container::FindStringProperty");
	}
	if ((*ppProperty)->GetType() != StringProperty) {
		throw new MP4Error("type mismatch", 
			"MP4Container::FindStringProperty");
	}
}

const char* MP4Container::GetStringProperty(const char* name)
{
	MP4Property* pProperty;
	u_int32_t index;

	FindStringProperty(name, &pProperty, &index);

	return ((MP4StringProperty*)pProperty)->GetValue(index);
}

void MP4Container::SetStringProperty(const char* name, const char* value)
{
	MP4Property* pProperty;
	u_int32_t index;

	FindStringProperty(name, &pProperty, &index);

	((MP4StringProperty*)pProperty)->SetValue(value, index);
}

void MP4Container::FindBytesProperty(const char* name, 
	MP4Property** ppProperty, u_int32_t* pIndex)
{
	if (!FindProperty(name, ppProperty, pIndex)) {
		throw new MP4Error("no such property",
			"MP4Container::FindBytesProperty");
	}
	if ((*ppProperty)->GetType() != BytesProperty) {
		throw new MP4Error("type mismatch",
			"MP4Container::FindBytesProperty");
	}
}

void MP4Container::GetBytesProperty(const char* name, 
	u_int8_t** ppValue, u_int32_t* pValueSize)
{
	MP4Property* pProperty;
	u_int32_t index;

	FindBytesProperty(name, &pProperty, &index);

	((MP4BytesProperty*)pProperty)->GetValue(ppValue, pValueSize, index);
}

void MP4Container::SetBytesProperty(const char* name, 
	const u_int8_t* pValue, u_int32_t valueSize)
{
	MP4Property* pProperty;
	u_int32_t index;

	FindBytesProperty(name, &pProperty, &index);

	((MP4BytesProperty*)pProperty)->SetValue(pValue, valueSize, index);
}

void MP4Container::Read(MP4File* pFile)
{
	u_int32_t numProperties = m_pProperties.Size();

	for (u_int32_t i = 0; i < numProperties; i++) {
		m_pProperties[i]->Read(pFile);
	}
}

void MP4Container::Write(MP4File* pFile)
{
	u_int32_t numProperties = m_pProperties.Size();

	if (numProperties == 0) {
		WARNING(numProperties == 0);
		return;
	}

	for (u_int32_t i = 0; i < numProperties; i++) {
		m_pProperties[i]->Write(pFile);
	}
}

void MP4Container::Dump(FILE* pFile, u_int8_t indent, bool dumpImplicits)
{
	u_int32_t numProperties = m_pProperties.Size();

	for (u_int32_t i = 0; i < numProperties; i++) {
		m_pProperties[i]->Dump(pFile, indent, dumpImplicits);
	}
}

