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

MP4IODescriptor::MP4IODescriptor()
	: MP4Descriptor(MP4FileIODescrTag)
{
	/* N.B. other member functions depend on the property indicies */
	AddProperty( /* 0 */
		new MP4BitfieldProperty("objectDescriptorId", 10));
	AddProperty( /* 1 */
		new MP4BitfieldProperty("URLFlag", 1));
	AddProperty( /* 2 */
		new MP4BitfieldProperty("includeInlineProfileLevelFlag", 1));
	AddProperty( /* 3 */
		new MP4BitfieldProperty("reserved", 4));
	AddProperty( /* 4 */
		new MP4StringProperty("URL", Counted));
	AddProperty( /* 5 */
		new MP4Integer8Property("ODProfileLevelId"));
	AddProperty( /* 6 */
		new MP4Integer8Property("sceneProfileLevelId"));
	AddProperty( /* 7 */
		new MP4Integer8Property("audioProfileLevelId"));
	AddProperty( /* 8 */
		new MP4Integer8Property("visualProfileLevelId"));
	AddProperty( /* 9 */
		new MP4Integer8Property("graphicsProfileLevelId"));
	AddProperty( /* 10 */ 
		new MP4DescriptorProperty("esIds", 
			MP4ESIDIncDescrTag, 0, Required, Many));
	AddProperty( /* 11 */ 
		new MP4DescriptorProperty("ociDescr", 
			MP4OCIDescrTagsStart, MP4OCIDescrTagsEnd, Optional, Many));
	AddProperty( /* 12 */
		new MP4DescriptorProperty("ipmpDescrPtr",
			MP4IPMPPtrDescrTag, 0, Optional, Many));
	AddProperty( /* 13 */
		new MP4DescriptorProperty("extDescr",
			MP4ExtDescrTagsStart, MP4ExtDescrTagsEnd, Optional, Many));

	SetReadMutate(2);
}

void MP4IODescriptor::Generate()
{
	((MP4BitfieldProperty*)m_pProperties[0])->SetValue(1);
	((MP4BitfieldProperty*)m_pProperties[3])->SetValue(0xF);
	for (u_int32_t i = 5; i <= 9; i++) {
		((MP4Integer8Property*)m_pProperties[i])->SetValue(0xFF);
	}
}

void MP4IODescriptor::Mutate()
{
	bool urlFlag = ((MP4BitfieldProperty*)m_pProperties[1])->GetValue();

	m_pProperties[4]->SetImplicit(!urlFlag);
	for (u_int32_t i = 5; i <= 12; i++) {
		m_pProperties[i]->SetImplicit(urlFlag);
	}
}

MP4ODescriptor::MP4ODescriptor()
	: MP4Descriptor(MP4FileODescrTag)
{
	/* N.B. other member functions depend on the property indicies */
	AddProperty( /* 0 */
		new MP4BitfieldProperty("objectDescriptorId", 10));
	AddProperty( /* 1 */
		new MP4BitfieldProperty("URLFlag", 1));
	AddProperty( /* 2 */
		new MP4BitfieldProperty("reserved", 5));
	AddProperty( /* 3 */
		new MP4StringProperty("URL", Counted));
	AddProperty( /* 4 */ 
		new MP4DescriptorProperty("esIds", 
			MP4ESIDRefDescrTag, 0, Required, Many));
	AddProperty( /* 5 */ 
		new MP4DescriptorProperty("ociDescr", 
			MP4OCIDescrTagsStart, MP4OCIDescrTagsEnd, Optional, Many));
	AddProperty( /* 6 */
		new MP4DescriptorProperty("ipmpDescrPtr",
			MP4IPMPPtrDescrTag, 0, Optional, Many));
	AddProperty( /* 7 */
		new MP4DescriptorProperty("extDescr",
			MP4ExtDescrTagsStart, MP4ExtDescrTagsEnd, Optional, Many));

	SetReadMutate(2);
}

void MP4ODescriptor::Generate()
{
	((MP4BitfieldProperty*)m_pProperties[2])->SetValue(0x1F);
}

void MP4ODescriptor::Mutate()
{
	bool urlFlag = ((MP4BitfieldProperty*)m_pProperties[1])->GetValue();

	m_pProperties[3]->SetImplicit(!urlFlag);
	for (u_int32_t i = 4; i <= 6; i++) {
		m_pProperties[i]->SetImplicit(urlFlag);
	}
}

MP4ESIDIncDescriptor::MP4ESIDIncDescriptor()
	: MP4Descriptor(MP4ESIDIncDescrTag)
{
	AddProperty( /* 0 */
		new MP4Integer32Property("id"));
}

MP4ESIDRefDescriptor::MP4ESIDRefDescriptor()
	: MP4Descriptor(MP4ESIDRefDescrTag)
{
	AddProperty( /* 0 */
		new MP4Integer16Property("refIndex"));
}

MP4ESDescriptor::MP4ESDescriptor()
	: MP4Descriptor(MP4ESDescrTag)
{
	/* N.B. other class functions depend on the property indicies */
	AddProperty( /* 0 */
		new MP4Integer16Property("ESID"));
	AddProperty( /* 1 */
		new MP4BitfieldProperty("streamDependenceFlag", 1));
	AddProperty( /* 2 */
		new MP4BitfieldProperty("URLFlag", 1));
	AddProperty( /* 3 */
		new MP4BitfieldProperty("OCRstreamFlag", 1));
	AddProperty( /* 4 */
		new MP4BitfieldProperty("streamPriority", 5));
	AddProperty( /* 5 */
		new MP4Integer16Property("dependsOnESID"));
	AddProperty( /* 6 */
		new MP4StringProperty("URL", Counted));
	AddProperty( /* 7 */
		new MP4Integer16Property("OCRESID"));
	AddProperty( /* 8 */
		new MP4DescriptorProperty("decConfigDescr",
			MP4DecConfigDescrTag, 0, Required, OnlyOne));
	AddProperty( /* 9 */
		new MP4DescriptorProperty("slConfigDescr",
			MP4SLConfigDescrTag, 0, Required, OnlyOne));
	AddProperty( /* 10 */
		new MP4DescriptorProperty("ipiPtr",
			MP4IPIPtrDescrTag, 0, Optional, OnlyOne));
	AddProperty( /* 11 */
		new MP4DescriptorProperty("ipIds",
			MP4ContentIdDescrTag, MP4SupplContentIdDescrTag, Optional, Many));
	AddProperty( /* 12 */
		new MP4DescriptorProperty("ipmpDescrPtr",
			MP4IPMPPtrDescrTag, 0, Optional, Many));
	AddProperty( /* 13 */
		new MP4DescriptorProperty("langDescr",
			MP4LanguageDescrTag, 0, Optional, Many));
	AddProperty( /* 14 */
		new MP4DescriptorProperty("qosDescr",
			MP4QosDescrTag, 0, Optional, OnlyOne));
	AddProperty( /* 15 */
		new MP4DescriptorProperty("regDescr",
			MP4RegistrationDescrTag, 0, Optional, OnlyOne));
	AddProperty( /* 16 */
		new MP4DescriptorProperty("extDescr",
			MP4ExtDescrTagsStart, MP4ExtDescrTagsEnd, Optional, Many));

	SetReadMutate(5);
}

void MP4ESDescriptor::Mutate()
{
	bool streamDependFlag = 
		((MP4BitfieldProperty*)m_pProperties[1])->GetValue();
	m_pProperties[5]->SetImplicit(!streamDependFlag);

	bool urlFlag = 
		((MP4BitfieldProperty*)m_pProperties[2])->GetValue();
	m_pProperties[6]->SetImplicit(!urlFlag);

	bool ocrFlag = 
		((MP4BitfieldProperty*)m_pProperties[3])->GetValue();
	m_pProperties[7]->SetImplicit(!ocrFlag);
}

MP4DecConfigDescriptor::MP4DecConfigDescriptor()
	: MP4Descriptor(MP4DecConfigDescrTag)
{
	AddProperty( /* 0 */
		new MP4Integer8Property("objectTypeId"));
	AddProperty( /* 1 */
		new MP4BitfieldProperty("streamType", 6));
	AddProperty( /* 2 */
		new MP4BitfieldProperty("upStream", 1));
	AddProperty( /* 3 */
		new MP4BitfieldProperty("reserved", 1));
	AddProperty( /* 4 */
		new MP4BitfieldProperty("bufferSizeDB", 24));
	AddProperty( /* 5 */
		new MP4Integer32Property("maxBitrate"));
	AddProperty( /* 6 */
		new MP4Integer32Property("avgBitrate"));
	AddProperty( /* 7 */
		new MP4DescriptorProperty("decSpecificInfo",
			MP4DecSpecificDescrTag, 0, Optional, OnlyOne));
	AddProperty( /* 8 */
		new MP4DescriptorProperty("profileLevelIndicationIndexDescr",
			MP4ExtProfileLevelDescrTag, 0, Optional, Many));
}

void MP4DecConfigDescriptor::Generate()
{
	((MP4BitfieldProperty*)m_pProperties[3])->SetValue(1);
}

MP4DecSpecificDescriptor::MP4DecSpecificDescriptor()
	: MP4Descriptor(MP4DecSpecificDescrTag)
{
	AddProperty( /* 0 */
		new MP4BytesProperty("info"));
}

void MP4DecSpecificDescriptor::Read(MP4File* pFile)
{
	ReadHeader(pFile);

	/* byte properties need to know how long they are before reading */
	((MP4BytesProperty*)m_pProperties[0])->SetValueSize(m_size);

	ReadProperties(pFile);
}

MP4SLConfigDescriptor::MP4SLConfigDescriptor()
	: MP4Descriptor(MP4SLConfigDescrTag)
{
	AddProperty( /* 0 */
		new MP4Integer8Property("predefined"));
	AddProperty( /* 1 */
		new MP4BitfieldProperty("useAccessUnitStartFlag", 1));
	AddProperty( /* 2 */
		new MP4BitfieldProperty("useAccessUnitEndFlag", 1));
	AddProperty( /* 3 */
		new MP4BitfieldProperty("useRandomAccessPointFlag", 1));
	AddProperty( /* 4 */
		new MP4BitfieldProperty("hasRandomAccessUnitsOnlyFlag", 1));
	AddProperty( /* 5 */
		new MP4BitfieldProperty("usePaddingFlag", 1));
	AddProperty( /* 6 */
		new MP4BitfieldProperty("useTimeStampsFlag", 1));
	AddProperty( /* 7 */
		new MP4BitfieldProperty("useIdleFlag", 1));
	AddProperty( /* 8 */
		new MP4BitfieldProperty("durationFlag", 1));
	AddProperty( /* 9 */
		new MP4Integer32Property("timeStampResolution"));
	AddProperty( /* 10 */
		new MP4Integer32Property("OCRResolution"));
	AddProperty( /* 11 */
		new MP4Integer8Property("timeStampLength"));
	AddProperty( /* 12 */
		new MP4Integer8Property("OCRLength"));
	AddProperty( /* 13 */
		new MP4Integer8Property("AULength"));
	AddProperty( /* 14 */
		new MP4Integer8Property("instantBitrateLength"));
	AddProperty( /* 15 */
		new MP4BitfieldProperty("degradationPriortyLength", 4));
	AddProperty( /* 16 */
		new MP4BitfieldProperty("AUSeqNumLength", 5));
	AddProperty( /* 17 */
		new MP4BitfieldProperty("packetSeqNumLength", 5));
	AddProperty( /* 18 */
		new MP4BitfieldProperty("reserved", 2));

	// if durationFlag 
	AddProperty( /* 19 */
		new MP4Integer32Property("timeScale"));
	AddProperty( /* 20 */
		new MP4Integer16Property("accessUnitDuration"));
	AddProperty( /* 21 */
		new MP4Integer16Property("compositionUnitDuration"));
	
	// if !useTimeStampsFlag
	AddProperty( /* 22 */
		new MP4BitfieldProperty("startDecodingTimeStamp", 64));
	AddProperty( /* 23 */
		new MP4BitfieldProperty("startCompositionTimeStamp", 64));
}

void MP4SLConfigDescriptor::Generate()
{
	// by default all tracks in an mp4 file 
	// use predefined SLConfig descriptor == 2
	((MP4Integer8Property*)m_pProperties[0])->SetValue(2);

	// which implies UseTimestampsFlag = 1
	((MP4BitfieldProperty*)m_pProperties[6])->SetValue(1);

	// reserved = 3
	((MP4BitfieldProperty*)m_pProperties[18])->SetValue(3);
}

void MP4SLConfigDescriptor::Read(MP4File* pFile)
{
	ReadHeader(pFile);

	// read the first property, 'predefined'
	ReadProperties(pFile, 0, 1);

	// if predefined == 0
	if (((MP4Integer8Property*)m_pProperties[0])->GetValue() == 0) {

		/* read the next 18 properties */
		ReadProperties(pFile, 1, 18);
	}

	// now mutate 
	Mutate();

	// and read the remaining properties
	ReadProperties(pFile, 19);
}

void MP4SLConfigDescriptor::Mutate()
{
	u_int32_t i;
	u_int8_t predefined = 
		((MP4Integer8Property*)m_pProperties[0])->GetValue();

	if (predefined) {
		// properties 1-18 are implicit
		for (i = 1; i < m_pProperties.Size(); i++) {
			m_pProperties[i]->SetImplicit(true);
		}

		if (predefined == 1) {
			// UseTimestampsFlag = 0
			((MP4BitfieldProperty*)m_pProperties[6])->SetValue(0);

			// TimestampResolution = 1000
			((MP4Integer32Property*)m_pProperties[9])->SetValue(1000);

			// TimeStampLength = 32
			((MP4Integer8Property*)m_pProperties[11])->SetValue(32);

		} else if (predefined == 2) {
			// UseTimestampsFlag = 1
			((MP4BitfieldProperty*)m_pProperties[6])->SetValue(1);
		}
	} else {
#if 1
	  for (i = 1; i <= 18; i++) {
	    m_pProperties[i]->SetImplicit(false);
	  }
	((MP4BitfieldProperty*)m_pProperties[18])->SetValue(3);
#endif
	}

	bool durationFlag = 
		((MP4BitfieldProperty*)m_pProperties[8])->GetValue();

	for (i = 19; i <= 21; i++) {
		m_pProperties[i]->SetImplicit(!durationFlag);
	}

	bool useTimeStampsFlag = 
		((MP4BitfieldProperty*)m_pProperties[6])->GetValue();

	for (i = 22; i <= 23; i++) {
		m_pProperties[i]->SetImplicit(useTimeStampsFlag);

		u_int8_t timeStampLength = MIN(64,
			((MP4Integer8Property*)m_pProperties[11])->GetValue());

		((MP4BitfieldProperty*)m_pProperties[i])->SetNumBits(timeStampLength);
		
		// handle a nonsensical situation gracefully
		if (timeStampLength == 0) {
			m_pProperties[i]->SetImplicit(true);
		}
	}
}

MP4IPIPtrDescriptor::MP4IPIPtrDescriptor()
	: MP4Descriptor(MP4IPIPtrDescrTag)
{
	AddProperty( /* 0 */
		new MP4Integer16Property("IPIESId"));
}

MP4ContentIdDescriptor::MP4ContentIdDescriptor()
	: MP4Descriptor(MP4ContentIdDescrTag)
{
	AddProperty( /* 0 */
		new MP4BitfieldProperty("compatibility", 2));
	AddProperty( /* 1 */
		new MP4BitfieldProperty("contentTypeFlag", 1));
	AddProperty( /* 2 */
		new MP4BitfieldProperty("contentIdFlag", 1));
	AddProperty( /* 3 */
		new MP4BitfieldProperty("protectedContent", 1));
	AddProperty( /* 4 */
		new MP4BitfieldProperty("reserved", 3));
	AddProperty( /* 5 */
		new MP4Integer8Property("contentType"));
	AddProperty( /* 6 */
		new MP4Integer8Property("contentIdType"));
	AddProperty( /* 7 */
		new MP4BytesProperty("contentId"));
}

void MP4ContentIdDescriptor::Read(MP4File* pFile)
{
	ReadHeader(pFile);

	/* read the first property, 'compatiblity' */
	ReadProperties(pFile, 0, 1);

	/* if compatiblity != 0 */
	if (((MP4Integer8Property*)m_pProperties[0])->GetValue() != 0) {
		/* we don't understand it */
		VERBOSE_READ(pFile->GetVerbosity(),
			printf("incompatible content id descriptor\n"));
		return;
	}

	/* read the next four properties */
	ReadProperties(pFile, 1, 4);

	/* which allows us to reconfigure ourselves */
	Mutate();

	/* read the remaining properties */
	ReadProperties(pFile, 5);
}

void MP4ContentIdDescriptor::Mutate()
{
	bool contentTypeFlag = ((MP4BitfieldProperty*)m_pProperties[1])->GetValue();
	m_pProperties[5]->SetImplicit(!contentTypeFlag);

	bool contentIdFlag = ((MP4BitfieldProperty*)m_pProperties[2])->GetValue();
	m_pProperties[6]->SetImplicit(!contentIdFlag);
	m_pProperties[7]->SetImplicit(!contentIdFlag);
}

MP4SupplContentIdDescriptor::MP4SupplContentIdDescriptor()
	: MP4Descriptor(MP4SupplContentIdDescrTag)
{
	AddProperty( /* 0 */
		new MP4BytesProperty("languageCode", 3));
	AddProperty( /* 1 */
		new MP4StringProperty("title", Counted));
	AddProperty( /* 2 */
		new MP4StringProperty("value", Counted));
}

MP4IPMPPtrDescriptor::MP4IPMPPtrDescriptor()
	: MP4Descriptor(MP4IPMPPtrDescrTag)
{
	AddProperty( /* 0 */
		new MP4Integer8Property("IPMPDescriptorId"));
}

MP4IPMPDescriptor::MP4IPMPDescriptor()
	: MP4Descriptor(MP4IPMPDescrTag)
{
	AddProperty( /* 0 */
		new MP4Integer8Property("IPMPDescriptorId"));
	AddProperty( /* 1 */
		new MP4Integer16Property("IPMPSType"));
	AddProperty( /* 2 */
		new MP4BytesProperty("IPMPData"));
	/* note: if IPMPSType == 0, IPMPData is an URL */
}

void MP4IPMPDescriptor::Read(MP4File* pFile)
{
	ReadHeader(pFile);

	/* byte properties need to know how long they are before reading */
	((MP4BytesProperty*)m_pProperties[2])->SetValueSize(m_size - 3);

	ReadProperties(pFile);
}

MP4RegistrationDescriptor::MP4RegistrationDescriptor()
	: MP4Descriptor(MP4RegistrationDescrTag)
{
	AddProperty( /* 0 */
		new MP4Integer32Property("formatIdentifier"));
	AddProperty( /* 1 */
		new MP4BytesProperty("additionalIdentificationInfo"));
}

void MP4RegistrationDescriptor::Read(MP4File* pFile)
{
	ReadHeader(pFile);

	/* byte properties need to know how long they are before reading */
	((MP4BytesProperty*)m_pProperties[1])->SetValueSize(m_size - 4);

	ReadProperties(pFile);
}

MP4ExtProfileLevelDescriptor::MP4ExtProfileLevelDescriptor()
	: MP4Descriptor(MP4ExtProfileLevelDescrTag)
{
	AddProperty( /* 0 */
		new MP4Integer8Property("profileLevelIndicationIndex"));
	AddProperty( /* 1 */
		new MP4Integer8Property("ODProfileLevelIndication"));
	AddProperty( /* 2 */
		new MP4Integer8Property("sceneProfileLevelIndication"));
	AddProperty( /* 3 */
		new MP4Integer8Property("audioProfileLevelIndication"));
	AddProperty( /* 4 */
		new MP4Integer8Property("visualProfileLevelIndication"));
	AddProperty( /* 5 */
		new MP4Integer8Property("graphicsProfileLevelIndication"));
	AddProperty( /* 6 */
		new MP4Integer8Property("MPEGJProfileLevelIndication"));
}

MP4ExtensionDescriptor::MP4ExtensionDescriptor()
	: MP4Descriptor()
{
	AddProperty( /* 0 */
		new MP4BytesProperty("data"));
}

void MP4ExtensionDescriptor::Read(MP4File* pFile)
{
	ReadHeader(pFile);

	/* byte properties need to know how long they are before reading */
	((MP4BytesProperty*)m_pProperties[0])->SetValueSize(m_size);

	ReadProperties(pFile);
}

MP4Descriptor* MP4DescriptorProperty::CreateDescriptor(u_int8_t tag) 
{
	MP4Descriptor* pDescriptor = NULL;

	switch (tag) {
	case MP4ESDescrTag:
		pDescriptor = new MP4ESDescriptor();
		break;
	case MP4DecConfigDescrTag:
		pDescriptor = new MP4DecConfigDescriptor();
		break;
	case MP4DecSpecificDescrTag:
		pDescriptor = new MP4DecSpecificDescriptor();
		break;
	case MP4SLConfigDescrTag:
		pDescriptor = new MP4SLConfigDescriptor();
		break;
	case MP4ContentIdDescrTag:
		pDescriptor = new MP4ContentIdDescriptor();
		break;
	case MP4SupplContentIdDescrTag:
		pDescriptor = new MP4SupplContentIdDescriptor();
		break;
	case MP4IPIPtrDescrTag:
		pDescriptor = new MP4IPIPtrDescriptor();
		break;
	case MP4IPMPPtrDescrTag:
		pDescriptor = new MP4IPMPPtrDescriptor();
		break;
	case MP4IPMPDescrTag:
		pDescriptor = new MP4IPMPDescriptor();
		break;
	case MP4QosDescrTag:
		pDescriptor = new MP4QosDescriptor();
		break;
	case MP4RegistrationDescrTag:
		pDescriptor = new MP4RegistrationDescriptor();
		break;
	case MP4ESIDIncDescrTag:
		pDescriptor = new MP4ESIDIncDescriptor();
		break;
	case MP4ESIDRefDescrTag:
		pDescriptor = new MP4ESIDRefDescriptor();
		break;
	case MP4IODescrTag:
	case MP4FileIODescrTag:
		pDescriptor = new MP4IODescriptor();
		pDescriptor->SetTag(tag);
		break;
	case MP4ODescrTag:
	case MP4FileODescrTag:
		pDescriptor = new MP4ODescriptor();
		pDescriptor->SetTag(tag);
		break;
	case MP4ExtProfileLevelDescrTag:
		pDescriptor = new MP4ExtProfileLevelDescriptor();
		break;
	}

	if (pDescriptor == NULL) {
		if (tag >= MP4OCIDescrTagsStart && tag <= MP4OCIDescrTagsEnd) {
			pDescriptor = CreateOCIDescriptor(tag);
		}

		if (tag >= MP4ExtDescrTagsStart && tag <= MP4ExtDescrTagsEnd) {
			pDescriptor = new MP4ExtensionDescriptor();
			pDescriptor->SetTag(tag);
		}
	}

	return pDescriptor;
}

