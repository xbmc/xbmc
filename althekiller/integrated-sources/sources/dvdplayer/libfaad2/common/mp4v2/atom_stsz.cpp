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

MP4StszAtom::MP4StszAtom() 
	: MP4Atom("stsz") 
{
	AddVersionAndFlags(); /* 0, 1 */

	AddProperty( /* 2 */
		new MP4Integer32Property("sampleSize")); 

	MP4Integer32Property* pCount = 
		new MP4Integer32Property("sampleCount"); 
	AddProperty(pCount); /* 3 */

	MP4TableProperty* pTable = new MP4TableProperty("entries", pCount);
	AddProperty(pTable); /* 4 */

	pTable->AddProperty( /* 4/0 */
		new MP4Integer32Property("sampleSize"));
}

void MP4StszAtom::Read() 
{
	ReadProperties(0, 4);

	u_int32_t sampleSize = 
		((MP4Integer32Property*)m_pProperties[2])->GetValue();

	// only attempt to read entries table if sampleSize is zero
	// i.e sample size is not constant
	m_pProperties[4]->SetImplicit(sampleSize != 0);

	ReadProperties(4);

	Skip();	// to end of atom
}

void MP4StszAtom::Write() 
{
	u_int32_t sampleSize = 
		((MP4Integer32Property*)m_pProperties[2])->GetValue();

	// only attempt to write entries table if sampleSize is zero
	// i.e sample size is not constant
	m_pProperties[4]->SetImplicit(sampleSize != 0);

	MP4Atom::Write();
}
