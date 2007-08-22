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

MP4RootAtom::MP4RootAtom() 
	: MP4Atom(NULL)
{
	ExpectChildAtom("ftyp", Required, OnlyOne);
	ExpectChildAtom("moov", Required, OnlyOne);
	ExpectChildAtom("mdat", Optional, Many);
	ExpectChildAtom("free", Optional, Many);
	ExpectChildAtom("skip", Optional, Many);
	ExpectChildAtom("udta", Optional, Many);
	ExpectChildAtom("moof", Optional, Many);
}

void MP4RootAtom::BeginWrite(bool use64) 
{
	// only call under MP4Create() control
	WriteAtomType("ftyp", OnlyOne);

	m_pChildAtoms[GetLastMdatIndex()]->BeginWrite(m_pFile->Use64Bits());
}

void MP4RootAtom::Write()
{
	// no-op
}

void MP4RootAtom::FinishWrite(bool use64)
{
	// finish writing last mdat atom
	u_int32_t mdatIndex = GetLastMdatIndex();
	m_pChildAtoms[mdatIndex]->FinishWrite(m_pFile->Use64Bits());

	// write all atoms after last mdat
	u_int32_t size = m_pChildAtoms.Size();
	for (u_int32_t i = mdatIndex + 1; i < size; i++) {
		m_pChildAtoms[i]->Write();
	}
}

void MP4RootAtom::BeginOptimalWrite() 
{
	WriteAtomType("ftyp", OnlyOne);
	WriteAtomType("moov", OnlyOne);
	WriteAtomType("udta", Many);

	m_pChildAtoms[GetLastMdatIndex()]->BeginWrite(m_pFile->Use64Bits());
}

void MP4RootAtom::FinishOptimalWrite() 
{
	// finish writing mdat
	m_pChildAtoms[GetLastMdatIndex()]->FinishWrite(m_pFile->Use64Bits());

	// find moov atom
	u_int32_t size = m_pChildAtoms.Size();
	MP4Atom* pMoovAtom = NULL;

	u_int32_t i;
	for (i = 0; i < size; i++) {
		if (!strcmp("moov", m_pChildAtoms[i]->GetType())) {
			pMoovAtom = m_pChildAtoms[i];
			break;
		}
	}
	ASSERT(i < size);

	// rewrite moov so that updated chunkOffsets are written to disk
	m_pFile->SetPosition(pMoovAtom->GetStart());
	u_int64_t oldSize = pMoovAtom->GetSize();

	pMoovAtom->Write();

	// sanity check
	u_int64_t newSize = pMoovAtom->GetSize();
	ASSERT(oldSize == newSize);
}

u_int32_t MP4RootAtom::GetLastMdatIndex()
{
	for (int32_t i = m_pChildAtoms.Size() - 1; i >= 0; i--) {
		if (!strcmp("mdat", m_pChildAtoms[i]->GetType())) {
			return i;
		}
	}
	ASSERT(false);
	return (u_int32_t)-1;
}

void MP4RootAtom::WriteAtomType(const char* type, bool onlyOne)
{
	u_int32_t size = m_pChildAtoms.Size();

	for (u_int32_t i = 0; i < size; i++) {
		if (!strcmp(type, m_pChildAtoms[i]->GetType())) {
			m_pChildAtoms[i]->Write();
			if (onlyOne) {
				break;
			}
		}
	}
}
