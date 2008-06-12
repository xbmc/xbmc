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

MP4FtypAtom::MP4FtypAtom() 
	: MP4Atom("ftyp")
{
	MP4StringProperty* pProp = new MP4StringProperty("majorBrand");
	pProp->SetFixedLength(4);
	AddProperty(pProp); /* 0 */

	AddProperty( /* 1 */
		new MP4Integer32Property("minorVersion"));

	MP4Integer32Property* pCount = 
		new MP4Integer32Property("compatibleBrandsCount"); 
	pCount->SetImplicit();
	AddProperty(pCount); /* 2 */

	MP4TableProperty* pTable = 
		new MP4TableProperty("compatibleBrands", pCount);
	AddProperty(pTable); /* 3 */

	pProp = new MP4StringProperty("brand");
	pProp->SetFixedLength(4);
	pTable->AddProperty(pProp);
}

void MP4FtypAtom::Generate() 
{
	MP4Atom::Generate();

	((MP4StringProperty*)m_pProperties[0])->SetValue("mp42");

	MP4StringProperty* pBrandProperty = (MP4StringProperty*)
		((MP4TableProperty*)m_pProperties[3])->GetProperty(0);
	ASSERT(pBrandProperty);
	pBrandProperty->AddValue("mp42");
	pBrandProperty->AddValue("isom");
	((MP4Integer32Property*)m_pProperties[2])->IncrementValue();
	((MP4Integer32Property*)m_pProperties[2])->IncrementValue();
}

void MP4FtypAtom::Read() 
{
	// table entry count computed from atom size
	((MP4Integer32Property*)m_pProperties[2])->SetReadOnly(false);
	((MP4Integer32Property*)m_pProperties[2])->SetValue((m_size - 8) / 4);
	((MP4Integer32Property*)m_pProperties[2])->SetReadOnly(true);

	MP4Atom::Read();
}
