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

MP4RtpAtom::MP4RtpAtom() 
	: MP4Atom("rtp ")
{
	// The atom type "rtp " is used in two complete unrelated ways
	// i.e. it's real two atoms with the same name
	// To handle that we need to postpone property creation until
	// we know who our parent atom is (stsd or hnti) which gives us
	// the context info we need to know who we are
}

void MP4RtpAtom::AddPropertiesStsdType()
{
	AddReserved("reserved1", 6); /* 0 */

	AddProperty( /* 1 */
		new MP4Integer16Property("dataReferenceIndex"));

	AddProperty( /* 2 */
		new MP4Integer16Property("hintTrackVersion"));
	AddProperty( /* 3 */
		new MP4Integer16Property("highestCompatibleVersion"));
	AddProperty( /* 4 */
		new MP4Integer32Property("maxPacketSize"));

	ExpectChildAtom("tims", Required, OnlyOne);
	ExpectChildAtom("tsro", Optional, OnlyOne);
	ExpectChildAtom("snro", Optional, OnlyOne);
}

void MP4RtpAtom::AddPropertiesHntiType()
{
	MP4StringProperty* pProp =
		new MP4StringProperty("descriptionFormat");
	pProp->SetFixedLength(4);
	AddProperty(pProp); /* 0 */

	AddProperty( /* 1 */
		new MP4StringProperty("sdpText"));
}

void MP4RtpAtom::Generate() 
{
	if (!strcmp(m_pParentAtom->GetType(), "stsd")) {
		AddPropertiesStsdType();
		GenerateStsdType();
	} else if (!strcmp(m_pParentAtom->GetType(), "hnti")) {
		AddPropertiesHntiType();
		GenerateHntiType();
	} else {
		VERBOSE_WARNING(m_pFile->GetVerbosity(),
			printf("Warning: rtp atom in unexpected context, can not generate"));
	}
}

void MP4RtpAtom::GenerateStsdType() 
{
	// generate children
	MP4Atom::Generate();

	((MP4Integer16Property*)m_pProperties[1])->SetValue(1);
	((MP4Integer16Property*)m_pProperties[2])->SetValue(1);
	((MP4Integer16Property*)m_pProperties[3])->SetValue(1);
}

void MP4RtpAtom::GenerateHntiType() 
{
	MP4Atom::Generate();

	((MP4StringProperty*)m_pProperties[0])->SetValue("sdp ");
}

void MP4RtpAtom::Read()
{
	if (!strcmp(m_pParentAtom->GetType(), "stsd")) {
		AddPropertiesStsdType();
		ReadStsdType();
	} else if (!strcmp(m_pParentAtom->GetType(), "hnti")) {
		AddPropertiesHntiType();
		ReadHntiType();
	} else {
		VERBOSE_READ(m_pFile->GetVerbosity(),
			printf("rtp atom in unexpected context, can not read"));
	}

	Skip(); // to end of atom
}

void MP4RtpAtom::ReadStsdType()
{
	MP4Atom::Read();
}

void MP4RtpAtom::ReadHntiType() 
{
	ReadProperties(0, 1);

	// read sdp string, length is implicit in size of atom
	u_int64_t size = GetEnd() - m_pFile->GetPosition();
	char* data = (char*)MP4Malloc(size + 1);
	m_pFile->ReadBytes((u_int8_t*)data, size);
	data[size] = '\0';
	((MP4StringProperty*)m_pProperties[1])->SetValue(data);
	MP4Free(data);
}

void MP4RtpAtom::Write()
{
	if (!strcmp(m_pParentAtom->GetType(), "hnti")) {
		WriteHntiType();
	} else {
		MP4Atom::Write();
	}
}

void MP4RtpAtom::WriteHntiType()
{
	// since length of string is implicit in size of atom
	// we need to handle this specially, and not write the terminating \0
	MP4StringProperty* pSdp = (MP4StringProperty*)m_pProperties[1];
	pSdp->SetFixedLength(strlen(pSdp->GetValue()));
	MP4Atom::Write();
	pSdp->SetFixedLength(0);
}

