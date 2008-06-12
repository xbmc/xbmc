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

MP4ContentClassDescriptor::MP4ContentClassDescriptor()
	: MP4Descriptor()
{
	AddProperty( /* 0 */
		new MP4Integer32Property("classificationEntity"));
	AddProperty( /* 1 */
		new MP4Integer16Property("classificationTable"));
	AddProperty( /* 2 */
		new MP4BytesProperty("contentClassificationData"));
}

void MP4ContentClassDescriptor::Read(MP4File* pFile)
{
	ReadHeader(pFile);

	/* byte properties need to know how long they are before reading */
	((MP4BytesProperty*)m_pProperties[2])->SetValueSize(m_size - 6);

	ReadProperties(pFile);
}

MP4KeywordDescriptor::MP4KeywordDescriptor()
	: MP4Descriptor()
{
	AddProperty( /* 0 */
		new MP4BytesProperty("languageCode", 3));
	AddProperty( /* 1 */
		new MP4BitfieldProperty("isUTF8String", 1));
	AddProperty( /* 2 */
		new MP4BitfieldProperty("reserved", 7));
	MP4Integer8Property* pCount = 
		new MP4Integer8Property("keywordCount");
	AddProperty(pCount); /* 3 */

	MP4TableProperty* pTable = new MP4TableProperty("keywords", pCount);
	AddProperty(pTable); /* 4 */

	pTable->AddProperty( /* 4, 0 */
		new MP4StringProperty("string", Counted));

	SetReadMutate(2);
}

void MP4KeywordDescriptor::Mutate()
{
	bool utf8Flag = ((MP4BitfieldProperty*)m_pProperties[1])->GetValue();
	MP4Property* pProperty =
		((MP4TableProperty*)m_pProperties[4])->GetProperty(0);
	ASSERT(pProperty);
	((MP4StringProperty*)pProperty)->SetUnicode(!utf8Flag);
}

MP4RatingDescriptor::MP4RatingDescriptor()
	: MP4Descriptor()
{
	AddProperty( /* 0 */
		new MP4Integer32Property("ratingEntity"));
	AddProperty( /* 1 */
		new MP4Integer16Property("ratingCriteria"));
	AddProperty( /* 2 */
		new MP4BytesProperty("ratingInfo"));
}

void MP4RatingDescriptor::Read(MP4File* pFile)
{
	ReadHeader(pFile);

	/* byte properties need to know how long they are before reading */
	((MP4BytesProperty*)m_pProperties[2])->SetValueSize(m_size - 6);

	ReadProperties(pFile);
}

MP4LanguageDescriptor::MP4LanguageDescriptor()
	: MP4Descriptor()
{
	AddProperty( /* 0 */
		new MP4BytesProperty("languageCode", 3));
}

MP4ShortTextDescriptor::MP4ShortTextDescriptor()
	: MP4Descriptor()
{
	AddProperty( /* 0 */
		new MP4BytesProperty("languageCode", 3));
	AddProperty( /* 1 */
		new MP4BitfieldProperty("isUTF8String", 1));
	AddProperty( /* 2 */
		new MP4BitfieldProperty("reserved", 7));
	AddProperty( /* 3 */
		new MP4StringProperty("eventName", Counted));
	AddProperty( /* 4 */
		new MP4StringProperty("eventText", Counted));

	SetReadMutate(2);
}

void MP4ShortTextDescriptor::Mutate()
{
	bool utf8Flag = ((MP4BitfieldProperty*)m_pProperties[1])->GetValue();
	((MP4StringProperty*)m_pProperties[3])->SetUnicode(!utf8Flag);
	((MP4StringProperty*)m_pProperties[4])->SetUnicode(!utf8Flag);
}

MP4ExpandedTextDescriptor::MP4ExpandedTextDescriptor()
	: MP4Descriptor()
{
	AddProperty( /* 0 */
		new MP4BytesProperty("languageCode", 3));
	AddProperty( /* 1 */
		new MP4BitfieldProperty("isUTF8String", 1));
	AddProperty( /* 2 */
		new MP4BitfieldProperty("reserved", 7));
	MP4Integer8Property* pCount = 
		new MP4Integer8Property("itemCount");
	AddProperty(pCount); /* 3 */

	MP4TableProperty* pTable = new MP4TableProperty("items", pCount);
	AddProperty(pTable); /* 4 */

	pTable->AddProperty( /* Table 0 */
		new MP4StringProperty("itemDescription", Counted));
	pTable->AddProperty( /* Table 1 */
		new MP4StringProperty("itemText", Counted));

	AddProperty( /* 5 */
		new MP4StringProperty("nonItemText"));
	((MP4StringProperty*)m_pProperties[5])->SetExpandedCountedFormat(true);

	SetReadMutate(2);
}

void MP4ExpandedTextDescriptor::Mutate()
{
	bool utf8Flag = ((MP4BitfieldProperty*)m_pProperties[1])->GetValue();

	MP4Property* pProperty =
		((MP4TableProperty*)m_pProperties[4])->GetProperty(0);
	ASSERT(pProperty);
	((MP4StringProperty*)pProperty)->SetUnicode(!utf8Flag);

	pProperty = ((MP4TableProperty*)m_pProperties[4])->GetProperty(1);
	ASSERT(pProperty);
	((MP4StringProperty*)pProperty)->SetUnicode(!utf8Flag);

	((MP4StringProperty*)m_pProperties[5])->SetUnicode(!utf8Flag);
}

class MP4CreatorTableProperty : public MP4TableProperty {
public:
	MP4CreatorTableProperty(char* name, MP4Integer8Property* pCountProperty) :
		MP4TableProperty(name, pCountProperty) {
	};
protected:
	void ReadEntry(MP4File* pFile, u_int32_t index);
	void WriteEntry(MP4File* pFile, u_int32_t index);
};

MP4CreatorDescriptor::MP4CreatorDescriptor(u_int8_t tag)
	: MP4Descriptor(tag)
{
	MP4Integer8Property* pCount = 
		new MP4Integer8Property("creatorCount");
	AddProperty(pCount); /* 0 */

	MP4TableProperty* pTable = new MP4CreatorTableProperty("creators", pCount);
	AddProperty(pTable); /* 1 */

	pTable->AddProperty( /* Table 0 */
		new MP4BytesProperty("languageCode", 3));
	pTable->AddProperty( /* Table 1 */
		new MP4BitfieldProperty("isUTF8String", 1));
	pTable->AddProperty( /* Table 2 */
		new MP4BitfieldProperty("reserved", 7));
	pTable->AddProperty( /* Table 3 */
		new MP4StringProperty("name", Counted));
}

void MP4CreatorTableProperty::ReadEntry(MP4File* pFile, u_int32_t index)
{
	m_pProperties[0]->Read(pFile, index);
	m_pProperties[1]->Read(pFile, index);

	bool utf8Flag = ((MP4BitfieldProperty*)m_pProperties[1])->GetValue(index);
	((MP4StringProperty*)m_pProperties[3])->SetUnicode(!utf8Flag);

	m_pProperties[2]->Read(pFile, index);
	m_pProperties[3]->Read(pFile, index);
}

void MP4CreatorTableProperty::WriteEntry(MP4File* pFile, u_int32_t index)
{
	bool utf8Flag = ((MP4BitfieldProperty*)m_pProperties[1])->GetValue(index);
	((MP4StringProperty*)m_pProperties[3])->SetUnicode(!utf8Flag);

	MP4TableProperty::WriteEntry(pFile, index);
}

MP4CreationDescriptor::MP4CreationDescriptor(u_int8_t tag)
	: MP4Descriptor(tag)
{
	AddProperty( /* 0 */
		new MP4BitfieldProperty("contentCreationDate", 40));
}

MP4SmpteCameraDescriptor::MP4SmpteCameraDescriptor()
	: MP4Descriptor()
{
	MP4Integer8Property* pCount = 
		new MP4Integer8Property("parameterCount"); 
	AddProperty(pCount);

	MP4TableProperty* pTable = new MP4TableProperty("parameters", pCount);
	AddProperty(pTable);

	pTable->AddProperty(
		new MP4Integer8Property("id"));
	pTable->AddProperty(
		new MP4Integer32Property("value"));
}

MP4UnknownOCIDescriptor::MP4UnknownOCIDescriptor()
	: MP4Descriptor()
{
	AddProperty( /* 0 */
		new MP4BytesProperty("data"));
}

void MP4UnknownOCIDescriptor::Read(MP4File* pFile)
{
	ReadHeader(pFile);

	/* byte properties need to know how long they are before reading */
	((MP4BytesProperty*)m_pProperties[0])->SetValueSize(m_size);

	ReadProperties(pFile);
}

MP4Descriptor* CreateOCIDescriptor(u_int8_t tag) 
{
	MP4Descriptor* pDescriptor = NULL;

	switch (tag) {
	case MP4ContentClassDescrTag:
		pDescriptor = new MP4ContentClassDescriptor();
		break;
	case MP4KeywordDescrTag:
		pDescriptor = new MP4KeywordDescriptor();
		break;
	case MP4RatingDescrTag:
		pDescriptor = new MP4RatingDescriptor();
		break;
	case MP4LanguageDescrTag:
		pDescriptor = new MP4LanguageDescriptor();
		break;
	case MP4ShortTextDescrTag:
		pDescriptor = new MP4ShortTextDescriptor();
		break;
	case MP4ExpandedTextDescrTag:
		pDescriptor = new MP4ExpandedTextDescriptor();
		break;
	case MP4ContentCreatorDescrTag:
	case MP4OCICreatorDescrTag:
		pDescriptor = new MP4CreatorDescriptor(tag);
		break;
	case MP4ContentCreationDescrTag:
	case MP4OCICreationDescrTag:
		pDescriptor = new MP4CreationDescriptor(tag);
		break;
	case MP4SmpteCameraDescrTag:
		pDescriptor = new MP4SmpteCameraDescriptor();
		break;
	}

	if (pDescriptor == NULL) {
		if (tag >= MP4OCIDescrTagsStart && tag <= MP4OCIDescrTagsEnd) {
			pDescriptor = new MP4UnknownOCIDescriptor();
			pDescriptor->SetTag(tag);
		}
	}

	return pDescriptor;
}

