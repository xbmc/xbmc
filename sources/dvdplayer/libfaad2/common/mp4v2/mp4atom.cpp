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
 *		Dave Mackie			dmackie@cisco.com
 *		Alix Marchandise-Franquet	alix@cisco.com
 */

#include "mp4common.h"
#include "atoms.h"

MP4AtomInfo::MP4AtomInfo(const char* name, bool mandatory, bool onlyOne) 
{
	m_name = name;
	m_mandatory = mandatory;
	m_onlyOne = onlyOne;
	m_count = 0;
}

MP4Atom::MP4Atom(const char* type) 
{
	SetType(type);
	m_unknownType = FALSE;
	m_pFile = NULL;
	m_start = 0;
	m_end = 0;
	m_size = 0;
	m_pParentAtom = NULL;
	m_depth = 0xFF;
}

MP4Atom::~MP4Atom()
{
	u_int32_t i;

	for (i = 0; i < m_pProperties.Size(); i++) {
		delete m_pProperties[i];
	}
	for (i = 0; i < m_pChildAtomInfos.Size(); i++) {
		delete m_pChildAtomInfos[i];
	}
	for (i = 0; i < m_pChildAtoms.Size(); i++) {
		delete m_pChildAtoms[i];
	}
}

MP4Atom* MP4Atom::CreateAtom(const char* type)
{
  MP4Atom* pAtom = NULL;

  if (type == NULL) {
    pAtom = new MP4RootAtom();
  } else {
    switch(type[0]) {
    case 'c':
      if (ATOMID(type) == ATOMID("ctts")) {
	pAtom = new MP4CttsAtom();
      } else if (ATOMID(type) == ATOMID("co64")) {
	pAtom = new MP4Co64Atom();
      } else if (ATOMID(type) == ATOMID("cprt")) {
	pAtom = new MP4CprtAtom();
      } else if (ATOMID(type) == ATOMID("cpil")) { /* Apple iTunes */
	pAtom = new MP4CpilAtom();
      } else if (ATOMID(type) == ATOMID("covr")) { /* Apple iTunes */
	pAtom = new MP4CovrAtom();
      }
      break;
    case 'd':
      if (ATOMID(type) == ATOMID("dinf")) {
	pAtom = new MP4DinfAtom();
      } else if (ATOMID(type) == ATOMID("dref")) {
	pAtom = new MP4DrefAtom();
      } else if (ATOMID(type) == ATOMID("dpnd")) {
	pAtom = new MP4TrefTypeAtom(type);
      } else if (ATOMID(type) == ATOMID("dmed")) {
	pAtom = new MP4DmedAtom();
      } else if (ATOMID(type) == ATOMID("dimm")) {
	pAtom = new MP4DimmAtom();
      } else if (ATOMID(type) == ATOMID("drep")) {
	pAtom = new MP4DrepAtom();
      } else if (ATOMID(type) == ATOMID("dmax")) {
	pAtom = new MP4DmaxAtom();
      } else if (ATOMID(type) == ATOMID("data")) { /* Apple iTunes */
	pAtom = new MP4DataAtom();
      } else if (ATOMID(type) == ATOMID("disk")) { /* Apple iTunes */
	pAtom = new MP4DiskAtom();
      }
      break;
    case 'e':
      if (ATOMID(type) == ATOMID("esds")) {
	pAtom = new MP4EsdsAtom();
      } else if (ATOMID(type) == ATOMID("edts")) {
	pAtom = new MP4EdtsAtom();
      } else if (ATOMID(type) == ATOMID("elst")) {
	pAtom = new MP4ElstAtom();
      } else if (ATOMID(type) == ATOMID("enca")) {
	pAtom = new MP4EncaAtom();
      } else if (ATOMID(type) == ATOMID("encv")) {
	pAtom = new MP4EncvAtom();
      }
      break;
    case 'f':
      if (ATOMID(type) == ATOMID("free")) {
	pAtom = new MP4FreeAtom();
      } else if (ATOMID(type) == ATOMID("frma")) {
	pAtom = new MP4FrmaAtom();
      } else if (ATOMID(type) == ATOMID("ftyp")) {
	pAtom = new MP4FtypAtom();
      }
      break;
    case 'g':
      if (ATOMID(type) == ATOMID("gnre")) { /* Apple iTunes */
	pAtom = new MP4GnreAtom();
      }
      break;
    case 'h':
      if (ATOMID(type) == ATOMID("hdlr")) {
	pAtom = new MP4HdlrAtom();
      } else if (ATOMID(type) == ATOMID("hmhd")) {
	pAtom = new MP4HmhdAtom();
      } else if (ATOMID(type) == ATOMID("hint")) {
	pAtom = new MP4TrefTypeAtom(type);
      } else if (ATOMID(type) == ATOMID("hnti")) {
	pAtom = new MP4HntiAtom();
      } else if (ATOMID(type) == ATOMID("hinf")) {
	pAtom = new MP4HinfAtom();
      }
      break;
    case 'i':
      if (ATOMID(type) == ATOMID("iKMS")) {
	pAtom = new MP4IKMSAtom();
      } else if (ATOMID(type) == ATOMID("iSFM")) {
	pAtom = new MP4ISFMAtom();
      } else if (ATOMID(type) == ATOMID("iods")) {
	pAtom = new MP4IodsAtom();
      } else if (ATOMID(type) == ATOMID("ipir")) {
	pAtom = new MP4TrefTypeAtom(type);
      } else if (ATOMID(type) == ATOMID("ilst")) { /* Apple iTunes */
	pAtom = new MP4IlstAtom();
      }
      break;
    case 'm':
      if (ATOMID(type) == ATOMID("mdia")) {
	pAtom = new MP4MdiaAtom();
      } else if (ATOMID(type) == ATOMID("minf")) {
	pAtom = new MP4MinfAtom();
      } else if (ATOMID(type) == ATOMID("mdhd")) {
	pAtom = new MP4MdhdAtom();
      } else if (ATOMID(type) == ATOMID("mdat")) {
	pAtom = new MP4MdatAtom();
      } else if (ATOMID(type) == ATOMID("moov")) {
	pAtom = new MP4MoovAtom();
      } else if (ATOMID(type) == ATOMID("mvhd")) {
	pAtom = new MP4MvhdAtom();
      } else if (ATOMID(type) == ATOMID("mpod")) {
	pAtom = new MP4TrefTypeAtom(type);
      } else if (ATOMID(type) == ATOMID("mp4a")) {
	pAtom = new MP4Mp4aAtom();
      } else if (ATOMID(type) == ATOMID("mp4s")) {
	pAtom = new MP4Mp4sAtom();
      } else if (ATOMID(type) == ATOMID("mp4v")) {
	pAtom = new MP4Mp4vAtom();
      } else if (ATOMID(type) == ATOMID("moof")) {
	pAtom = new MP4MoofAtom();
      } else if (ATOMID(type) == ATOMID("mfhd")) {
	pAtom = new MP4MfhdAtom();
      } else if (ATOMID(type) == ATOMID("mvex")) {
	pAtom = new MP4MvexAtom();
      } else if (ATOMID(type) == ATOMID("maxr")) {
	pAtom = new MP4MaxrAtom();
      } else if (ATOMID(type) == ATOMID("meta")) { /* Apple iTunes */
	pAtom = new MP4MetaAtom();
      } else if (ATOMID(type) == ATOMID("mean")) { /* Apple iTunes */
	pAtom = new MP4MeanAtom();
      }
      break;
    case 'n':
      if (ATOMID(type) == ATOMID("nmhd")) {
	pAtom = new MP4NmhdAtom();
      } else if (ATOMID(type) == ATOMID("nump")) {
	pAtom = new MP4NumpAtom();
      } else if (ATOMID(type) == ATOMID("name")) {
	pAtom = new MP4NameAtom();
      }
      break;
    case 'p':
      if (ATOMID(type) == ATOMID("pmax")) {
	pAtom = new MP4PmaxAtom();
      } else if (ATOMID(type) == ATOMID("payt")) {
	pAtom = new MP4PaytAtom();
      }
      break;
    case 'r':
      if (ATOMID(type) == ATOMID("rtp ")) {
	pAtom = new MP4RtpAtom();
      }
      break;
    case 's':
      if (ATOMID(type) == ATOMID("schi")) {
	pAtom = new MP4SchiAtom();
      } else if (ATOMID(type) == ATOMID("schm")) {
	pAtom = new MP4SchmAtom();
      } else if (ATOMID(type) == ATOMID("sinf")) {
	pAtom = new MP4SinfAtom();
      } else if (ATOMID(type) == ATOMID("stbl")) {
	pAtom = new MP4StblAtom();
      } else if (ATOMID(type) == ATOMID("stsd")) {
	pAtom = new MP4StsdAtom();
      } else if (ATOMID(type) == ATOMID("stts")) {
	pAtom = new MP4SttsAtom();
      } else if (ATOMID(type) == ATOMID("stsz")) {
	pAtom = new MP4StszAtom();
      } else if (ATOMID(type) == ATOMID("stsc")) {
	pAtom = new MP4StscAtom();
      } else if (ATOMID(type) == ATOMID("stco")) {
	pAtom = new MP4StcoAtom();
      } else if (ATOMID(type) == ATOMID("stss")) {
	pAtom = new MP4StssAtom();
      } else if (ATOMID(type) == ATOMID("stsh")) {
	pAtom = new MP4StshAtom();
      } else if (ATOMID(type) == ATOMID("stdp")) {
	pAtom = new MP4StdpAtom();
      } else if (ATOMID(type) == ATOMID("smhd")) {
	pAtom = new MP4SmhdAtom();
      } else if (ATOMID(type) == ATOMID("sdp ")) {
	pAtom = new MP4SdpAtom();
      } else if (ATOMID(type) == ATOMID("snro")) {
	pAtom = new MP4SnroAtom();
      } else if (ATOMID(type) == ATOMID("sync")) {
	pAtom = new MP4TrefTypeAtom(type);
      } else if (ATOMID(type) == ATOMID("skip")) {
	pAtom = new MP4FreeAtom();
	pAtom->SetType("skip");
      }
      break;
    case 't':
      if (ATOMID(type) == ATOMID("trak")) {
	pAtom = new MP4TrakAtom();
      } else if (ATOMID(type) == ATOMID("tkhd")) {
	pAtom = new MP4TkhdAtom();
      } else if (ATOMID(type) == ATOMID("tref")) {
	pAtom = new MP4TrefAtom();
      } else if (ATOMID(type) == ATOMID("traf")) {
	pAtom = new MP4TrafAtom();
      } else if (ATOMID(type) == ATOMID("tfhd")) {
	pAtom = new MP4TfhdAtom();
      } else if (ATOMID(type) == ATOMID("trex")) {
	pAtom = new MP4TrexAtom();
      } else if (ATOMID(type) == ATOMID("trun")) {
	pAtom = new MP4TrunAtom();
      } else if (ATOMID(type) == ATOMID("tmin")) {
	pAtom = new MP4TminAtom();
      } else if (ATOMID(type) == ATOMID("tmax")) {
	pAtom = new MP4TmaxAtom();
      } else if (ATOMID(type) == ATOMID("trpy")) {
	pAtom = new MP4TrpyAtom();
      } else if (ATOMID(type) == ATOMID("tpyl")) {
	pAtom = new MP4TpylAtom();
      } else if (ATOMID(type) == ATOMID("tims")) {
	pAtom = new MP4TimsAtom();
      } else if (ATOMID(type) == ATOMID("tsro")) {
	pAtom = new MP4TsroAtom();
      } else if (ATOMID(type) == ATOMID("trkn")) { /* Apple iTunes */
	pAtom = new MP4TrknAtom();
      } else if (ATOMID(type) == ATOMID("tmpo")) { /* Apple iTunes */
	pAtom = new MP4TmpoAtom();
      }
      break;
    case 'u':
      if (ATOMID(type) == ATOMID("udta")) {
	pAtom = new MP4UdtaAtom();
      } else if (ATOMID(type) == ATOMID("url ")) {
	pAtom = new MP4UrlAtom();
      } else if (ATOMID(type) == ATOMID("urn ")) {
	pAtom = new MP4UrnAtom();
      }
      break;
    case 'v':
      if (ATOMID(type) == ATOMID("vmhd")) {
	pAtom = new MP4VmhdAtom();
      }
      break;
    case '©':
      if (ATOMID(type) == ATOMID("©nam")) {
	pAtom = new MP4NamAtom();
      } else if (ATOMID(type) == ATOMID("©ART")) { /* Apple iTunes */
	pAtom = new MP4ArtAtom();
      } else if (ATOMID(type) == ATOMID("©wrt")) { /* Apple iTunes */
	pAtom = new MP4WrtAtom();
      } else if (ATOMID(type) == ATOMID("©alb")) { /* Apple iTunes */
	pAtom = new MP4AlbAtom();
      } else if (ATOMID(type) == ATOMID("©day")) { /* Apple iTunes */
	pAtom = new MP4DayAtom();
      } else if (ATOMID(type) == ATOMID("©too")) { /* Apple iTunes */
	pAtom = new MP4TooAtom();
      } else if (ATOMID(type) == ATOMID("©cmt")) { /* Apple iTunes */
	pAtom = new MP4CmtAtom();
      } else if (ATOMID(type) == ATOMID("©gen")) { /* Apple iTunes */
	pAtom = new MP4GenAtom();
      }
      break;
    case '-':
      if (ATOMID(type) == ATOMID("----")) { /* Apple iTunes */
	pAtom = new MP4DashAtom();
      }
    }
  }

  if (pAtom == NULL) {
    pAtom = new MP4Atom(type);
    pAtom->SetUnknownType(true);
  }

  ASSERT(pAtom);
  return pAtom;
}

// generate a skeletal self

void MP4Atom::Generate()
{
	u_int32_t i;

	// for all properties
	for (i = 0; i < m_pProperties.Size(); i++) {
		// ask it to self generate
		m_pProperties[i]->Generate();
	}

	// for all mandatory, single child atom types
	for (i = 0; i < m_pChildAtomInfos.Size(); i++) {
		if (m_pChildAtomInfos[i]->m_mandatory
		  && m_pChildAtomInfos[i]->m_onlyOne) {

			// create the mandatory, single child atom
			MP4Atom* pChildAtom = 
				CreateAtom(m_pChildAtomInfos[i]->m_name);

			AddChildAtom(pChildAtom);

			// and ask it to self generate
			pChildAtom->Generate();
		}
	}
}

MP4Atom* MP4Atom::ReadAtom(MP4File* pFile, MP4Atom* pParentAtom)
{
	u_int8_t hdrSize = 8;
	u_int8_t extendedType[16];

	u_int64_t pos = pFile->GetPosition();

	VERBOSE_READ(pFile->GetVerbosity(), 
		printf("ReadAtom: pos = 0x"LLX"\n", pos));

	u_int64_t dataSize = pFile->ReadUInt32();

	char type[5];
	pFile->ReadBytes((u_int8_t*)&type[0], 4);
	type[4] = '\0';
	
	// extended size
	if (dataSize == 1) {
		dataSize = pFile->ReadUInt64(); 
		hdrSize += 8;
	}

	// extended type
	if (ATOMID(type) == ATOMID("uuid")) {
		pFile->ReadBytes(extendedType, sizeof(extendedType));
		hdrSize += sizeof(extendedType);
	}

	if (dataSize == 0) {
		// extends to EOF
		dataSize = pFile->GetSize() - pos;
	}

	dataSize -= hdrSize;

	VERBOSE_READ(pFile->GetVerbosity(), 
		printf("ReadAtom: type = %s data-size = "LLU" (0x"LLX")\n", 
			type, dataSize, dataSize));

	if (pos + hdrSize + dataSize > pParentAtom->GetEnd()) {
		VERBOSE_READ(pFile->GetVerbosity(), 
			printf("ReadAtom: invalid atom size, extends outside parent atom\n"));
		throw new MP4Error("invalid atom size", "ReadAtom");
	}


	MP4Atom* pAtom = CreateAtom(type);
	pAtom->SetFile(pFile);
	pAtom->SetStart(pos);
	pAtom->SetEnd(pos + hdrSize + dataSize);
	pAtom->SetSize(dataSize);
	if (ATOMID(type) == ATOMID("uuid")) {
		pAtom->SetExtendedType(extendedType);
	}
	if (pAtom->IsUnknownType()) {
		if (!IsReasonableType(pAtom->GetType())) {
			VERBOSE_READ(pFile->GetVerbosity(),
				printf("Warning: atom type %s is suspect\n", pAtom->GetType()));
		} else {
			VERBOSE_READ(pFile->GetVerbosity(),
				printf("Info: atom type %s is unknown\n", pAtom->GetType()));
		}

		if (dataSize > 0) {
			pAtom->AddProperty(
				new MP4BytesProperty("data", dataSize));
		}
	}

	pAtom->SetParentAtom(pParentAtom);

	pAtom->Read();

	return pAtom;
}

bool MP4Atom::IsReasonableType(const char* type)
{
	for (u_int8_t i = 0; i < 4; i++) {
		if (isalnum(type[i])) {
			continue;
		}
		if (i == 3 && type[i] == ' ') {
			continue;
		}
		return false;
	}
	return true;
}

// generic read
void MP4Atom::Read()
{
	ASSERT(m_pFile);

	if (ATOMID(m_type) != 0 && m_size > 1000000) {
		VERBOSE_READ(GetVerbosity(), 
			printf("Warning: %s atom size "LLU" is suspect\n",
				m_type, m_size));
	}

	ReadProperties();

	// read child atoms, if we expect there to be some
	if (m_pChildAtomInfos.Size() > 0) {
		ReadChildAtoms();
	}

	Skip();	// to end of atom
}

void MP4Atom::Skip()
{
	if (m_pFile->GetPosition() != m_end) {
		VERBOSE_READ(m_pFile->GetVerbosity(),
			printf("Skip: "LLU" bytes\n", m_end - m_pFile->GetPosition()));
	}
	m_pFile->SetPosition(m_end);
}

MP4Atom* MP4Atom::FindAtom(const char* name)
{
	if (!IsMe(name)) {
		return NULL;
	}

	if (!IsRootAtom()) {
		VERBOSE_FIND(m_pFile->GetVerbosity(),
			printf("FindAtom: matched %s\n", name));

		name = MP4NameAfterFirst(name);

		// I'm the sought after atom 
		if (name == NULL) {
			return this;
		}
	}

	// else it's one of my children
	return FindChildAtom(name);
}

bool MP4Atom::FindProperty(const char *name, 
	MP4Property** ppProperty, u_int32_t* pIndex)
{
	if (!IsMe(name)) {
		return false;
	}

	if (!IsRootAtom()) {
		VERBOSE_FIND(m_pFile->GetVerbosity(),
			printf("FindProperty: matched %s\n", name));

		name = MP4NameAfterFirst(name);

		// no property name given
		if (name == NULL) {
			return false;
		}
	}

	return FindContainedProperty(name, ppProperty, pIndex);
}

bool MP4Atom::IsMe(const char* name)
{
	if (name == NULL) {
		return false;
	}

	// root atom always matches
	if (!strcmp(m_type, "")) {
		return true;
	}

	// check if our atom name is specified as the first component
	if (!MP4NameFirstMatches(m_type, name)) {
		return false;
	}

	return true;
}

MP4Atom* MP4Atom::FindChildAtom(const char* name)
{
	u_int32_t atomIndex = 0;

	// get the index if we have one, e.g. moov.trak[2].mdia...
	MP4NameFirstIndex(name, &atomIndex);

	// need to get to the index'th child atom of the right type
	for (u_int32_t i = 0; i < m_pChildAtoms.Size(); i++) {
		if (MP4NameFirstMatches(m_pChildAtoms[i]->GetType(), name)) {
			if (atomIndex == 0) {
				// this is the one, ask it to match
				return m_pChildAtoms[i]->FindAtom(name);
			}
			atomIndex--;
		}
	}

	return NULL;
}

bool MP4Atom::FindContainedProperty(const char *name,
	MP4Property** ppProperty, u_int32_t* pIndex)
{
	u_int32_t numProperties = m_pProperties.Size();
	u_int32_t i;
	// check all of our properties
	for (i = 0; i < numProperties; i++) {
		if (m_pProperties[i]->FindProperty(name, ppProperty, pIndex)) {
			return true;
		}
	}

	// not one of our properties, 
	// presumably one of our children's properties
	// check child atoms...

	// check if we have an index, e.g. trak[2].mdia...
	u_int32_t atomIndex = 0;
	MP4NameFirstIndex(name, &atomIndex);

	// need to get to the index'th child atom of the right type
	for (i = 0; i < m_pChildAtoms.Size(); i++) {
		if (MP4NameFirstMatches(m_pChildAtoms[i]->GetType(), name)) {
			if (atomIndex == 0) {
				// this is the one, ask it to match
				return m_pChildAtoms[i]->FindProperty(name, ppProperty, pIndex);
			}
			atomIndex--;
		}
	}

	VERBOSE_FIND(m_pFile->GetVerbosity(),
		printf("FindProperty: no match for %s\n", name));
	return false;
}

void MP4Atom::ReadProperties(u_int32_t startIndex, u_int32_t count)
{
	u_int32_t numProperties = MIN(count, m_pProperties.Size() - startIndex);

	// read any properties of the atom
	for (u_int32_t i = startIndex; i < startIndex + numProperties; i++) {

		m_pProperties[i]->Read(m_pFile);

		if (m_pFile->GetPosition() > m_end) {
			VERBOSE_READ(GetVerbosity(), 
				printf("ReadProperties: insufficient data for property: %s pos 0x"LLX" atom end 0x"LLX"\n",
					m_pProperties[i]->GetName(), 
					m_pFile->GetPosition(), m_end)); 

			throw new MP4Error("atom is too small", "Atom ReadProperties");
		}

		if (m_pProperties[i]->GetType() == TableProperty) {
			VERBOSE_READ_TABLE(GetVerbosity(), 
				printf("Read: "); m_pProperties[i]->Dump(stdout, 0, true));
		} else if (m_pProperties[i]->GetType() != DescriptorProperty) {
			VERBOSE_READ(GetVerbosity(), 
				printf("Read: "); m_pProperties[i]->Dump(stdout, 0, true));
		}
	}
}

void MP4Atom::ReadChildAtoms()
{
	VERBOSE_READ(GetVerbosity(), 
		printf("ReadChildAtoms: of %s\n", m_type[0] ? m_type : "root"));

	// read any child atoms
	while (m_pFile->GetPosition() < m_end) {
		MP4Atom* pChildAtom = MP4Atom::ReadAtom(m_pFile, this);

		AddChildAtom(pChildAtom);

		MP4AtomInfo* pChildAtomInfo = FindAtomInfo(pChildAtom->GetType());

		// if child atom is of known type
		// but not expected here print warning
		if (pChildAtomInfo == NULL && !pChildAtom->IsUnknownType()) {
			VERBOSE_READ(GetVerbosity(),
				printf("Warning: In atom %s unexpected child atom %s\n",
					GetType(), pChildAtom->GetType()));
		}

		// if child atoms should have just one instance
		// and this is more than one, print warning
		if (pChildAtomInfo) {
			pChildAtomInfo->m_count++;

			if (pChildAtomInfo->m_onlyOne && pChildAtomInfo->m_count > 1) {
				VERBOSE_READ(GetVerbosity(),
					printf("Warning: In atom %s multiple child atoms %s\n",
						GetType(), pChildAtom->GetType()));
			}
		}
	}

	// if mandatory child atom doesn't exist, print warning
	u_int32_t numAtomInfo = m_pChildAtomInfos.Size();
	for (u_int32_t i = 0; i < numAtomInfo; i++) {
		if (m_pChildAtomInfos[i]->m_mandatory
		  && m_pChildAtomInfos[i]->m_count == 0) {
				VERBOSE_READ(GetVerbosity(),
					printf("Warning: In atom %s missing child atom %s\n",
						GetType(), m_pChildAtomInfos[i]->m_name));
		}
	}

	VERBOSE_READ(GetVerbosity(), 
		printf("ReadChildAtoms: finished %s\n", m_type));
}

MP4AtomInfo* MP4Atom::FindAtomInfo(const char* name)
{
	u_int32_t numAtomInfo = m_pChildAtomInfos.Size();
	for (u_int32_t i = 0; i < numAtomInfo; i++) {
		if (ATOMID(m_pChildAtomInfos[i]->m_name) == ATOMID(name)) {
			return m_pChildAtomInfos[i];
		}
	}
	return NULL;
}

// generic write
void MP4Atom::Write()
{
	ASSERT(m_pFile);

	BeginWrite();

	WriteProperties();

	WriteChildAtoms();

	FinishWrite();
}

void MP4Atom::BeginWrite(bool use64)
{
	m_start = m_pFile->GetPosition();
	//use64 = m_pFile->Use64Bits();
	if (use64) {
		m_pFile->WriteUInt32(1);
	} else {
		m_pFile->WriteUInt32(0);
	}
	m_pFile->WriteBytes((u_int8_t*)&m_type[0], 4);
	if (use64) {
		m_pFile->WriteUInt64(0);
	}
	if (ATOMID(m_type) == ATOMID("uuid")) {
		m_pFile->WriteBytes(m_extendedType, sizeof(m_extendedType));
	}
}

void MP4Atom::FinishWrite(bool use64)
{
	m_end = m_pFile->GetPosition();
	m_size = (m_end - m_start);
	//use64 = m_pFile->Use64Bits();
	if (use64) {
		m_pFile->SetPosition(m_start + 8);
		m_pFile->WriteUInt64(m_size);
	} else {
		ASSERT(m_size <= (u_int64_t)0xFFFFFFFF);
		m_pFile->SetPosition(m_start);
		m_pFile->WriteUInt32(m_size);
	}
	m_pFile->SetPosition(m_end);

	// adjust size to just reflect data portion of atom
	m_size -= (use64 ? 16 : 8);
	if (ATOMID(m_type) == ATOMID("uuid")) {
		m_size -= sizeof(m_extendedType);
	}
}

void MP4Atom::WriteProperties(u_int32_t startIndex, u_int32_t count)
{
	u_int32_t numProperties = MIN(count, m_pProperties.Size() - startIndex);

	VERBOSE_WRITE(GetVerbosity(), 
		printf("Write: type %s\n", m_type));

	for (u_int32_t i = startIndex; i < startIndex + numProperties; i++) {
		m_pProperties[i]->Write(m_pFile);

		if (m_pProperties[i]->GetType() == TableProperty) {
			VERBOSE_WRITE_TABLE(GetVerbosity(), 
				printf("Write: "); m_pProperties[i]->Dump(stdout, 0, false));
		} else {
			VERBOSE_WRITE(GetVerbosity(), 
				printf("Write: "); m_pProperties[i]->Dump(stdout, 0, false));
		}
	}
}

void MP4Atom::WriteChildAtoms()
{
	u_int32_t size = m_pChildAtoms.Size();
	for (u_int32_t i = 0; i < size; i++) {
		m_pChildAtoms[i]->Write();
	}

	VERBOSE_WRITE(GetVerbosity(), 
		printf("Write: finished %s\n", m_type));
}

void MP4Atom::AddProperty(MP4Property* pProperty) 
{
	ASSERT(pProperty);
	m_pProperties.Add(pProperty);
	pProperty->SetParentAtom(this);
}

void MP4Atom::AddVersionAndFlags()
{
	AddProperty(new MP4Integer8Property("version"));
	AddProperty(new MP4Integer24Property("flags"));
}

void MP4Atom::AddReserved(char* name, u_int32_t size) 
{
	MP4BytesProperty* pReserved = new MP4BytesProperty(name, size); 
	pReserved->SetReadOnly();
	AddProperty(pReserved);
}

void MP4Atom::ExpectChildAtom(const char* name, bool mandatory, bool onlyOne)
{
	m_pChildAtomInfos.Add(new MP4AtomInfo(name, mandatory, onlyOne));
}

u_int8_t MP4Atom::GetVersion()
{
	if (strcmp("version", m_pProperties[0]->GetName())) {
		return 0;
	}
	return ((MP4Integer8Property*)m_pProperties[0])->GetValue();
}

void MP4Atom::SetVersion(u_int8_t version) 
{
	if (strcmp("version", m_pProperties[0]->GetName())) {
		return;
	}
	((MP4Integer8Property*)m_pProperties[0])->SetValue(version);
}

u_int32_t MP4Atom::GetFlags()
{
	if (strcmp("flags", m_pProperties[1]->GetName())) {
		return 0;
	}
	return ((MP4Integer24Property*)m_pProperties[1])->GetValue();
}

void MP4Atom::SetFlags(u_int32_t flags) 
{
	if (strcmp("flags", m_pProperties[1]->GetName())) {
		return;
	}
	((MP4Integer24Property*)m_pProperties[1])->SetValue(flags);
}

void MP4Atom::Dump(FILE* pFile, u_int8_t indent, bool dumpImplicits)
{
	if (m_type[0] != '\0') {
		Indent(pFile, indent);
		fprintf(pFile, "type %s\n", m_type);
	}

	u_int32_t i;
	u_int32_t size;

	// dump our properties
	size = m_pProperties.Size();
	for (i = 0; i < size; i++) {

		/* skip details of tables unless we're told to be verbose */
		if (m_pProperties[i]->GetType() == TableProperty
		  && !(GetVerbosity() & MP4_DETAILS_TABLE)) {
			Indent(pFile, indent + 1);
			fprintf(pFile, "<table entries suppressed>\n");
			continue;
		}

		m_pProperties[i]->Dump(pFile, indent + 1, dumpImplicits);
	}

	// dump our children
	size = m_pChildAtoms.Size();
	for (i = 0; i < size; i++) {
		m_pChildAtoms[i]->Dump(pFile, indent + 1, dumpImplicits);
	}
}

u_int32_t MP4Atom::GetVerbosity() 
{
	ASSERT(m_pFile);
	return m_pFile->GetVerbosity();
}

u_int8_t MP4Atom::GetDepth()
{
	if (m_depth < 0xFF) {
		return m_depth;
	}

	MP4Atom *pAtom = this;
	m_depth = 0;

	while ((pAtom = pAtom->GetParentAtom()) != NULL) {
		m_depth++;
		ASSERT(m_depth < 255);
	}
	return m_depth;
}

