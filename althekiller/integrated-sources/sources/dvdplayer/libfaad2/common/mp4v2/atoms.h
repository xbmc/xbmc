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

#ifndef __MP4_ATOMS_INCLUDED__
#define __MP4_ATOMS_INCLUDED__

// declare all the atom subclasses
// i.e. spare us atom_xxxx.h for all the atoms
//
// The majority of atoms just need their own constructor declared
// Some atoms have a few special needs
// A small minority of atoms need lots of special handling

class MP4RootAtom : public MP4Atom {
public:
	MP4RootAtom();
	void BeginWrite(bool use64 = false);
	void Write();
	void FinishWrite(bool use64 = false);

	void BeginOptimalWrite();
	void FinishOptimalWrite();

protected:
	u_int32_t GetLastMdatIndex();
	void WriteAtomType(const char* type, bool onlyOne);
};

class MP4FtypAtom : public MP4Atom {
public:
	MP4FtypAtom();
	void Generate();
	void Read();
};

class MP4MdatAtom : public MP4Atom {
public:
	MP4MdatAtom();
	void Read();
	void Write();
};

class MP4MoovAtom : public MP4Atom {
public:
	MP4MoovAtom();
};

class MP4MvhdAtom : public MP4Atom {
public:
	MP4MvhdAtom();
	void Generate();
	void Read();
protected:
	void AddProperties(u_int8_t version);
};

class MP4IodsAtom : public MP4Atom {
public:
	MP4IodsAtom();
};

class MP4TrakAtom : public MP4Atom {
public:
	MP4TrakAtom();
};

class MP4TkhdAtom : public MP4Atom {
public:
	MP4TkhdAtom();
	void Generate();
	void Read();
protected:
	void AddProperties(u_int8_t version);
};

class MP4TrefAtom : public MP4Atom {
public:
	MP4TrefAtom();
};

class MP4TrefTypeAtom : public MP4Atom {
public:
	MP4TrefTypeAtom(const char* type);
	void Read();
};

class MP4MdiaAtom : public MP4Atom {
public:
	MP4MdiaAtom();
};

class MP4MdhdAtom : public MP4Atom {
public:
	MP4MdhdAtom();
	void Generate();
	void Read();
protected:
	void AddProperties(u_int8_t version);
};

class MP4HdlrAtom : public MP4Atom {
public:
	MP4HdlrAtom();
	void Read();
};

class MP4MinfAtom : public MP4Atom {
public:
	MP4MinfAtom();
};

class MP4VmhdAtom : public MP4Atom {
public:
	MP4VmhdAtom();
	void Generate();
};

class MP4SmhdAtom : public MP4Atom {
public:
	MP4SmhdAtom();
};

class MP4HmhdAtom : public MP4Atom {
public:
	MP4HmhdAtom();
};

class MP4NmhdAtom : public MP4Atom {
public:
	MP4NmhdAtom();
};

class MP4DinfAtom : public MP4Atom {
public:
	MP4DinfAtom();
};

class MP4DrefAtom : public MP4Atom {
public:
	MP4DrefAtom();
	void Read();
};

class MP4UrlAtom : public MP4Atom {
public:
	MP4UrlAtom();
	void Read();
	void Write();
};

class MP4UrnAtom : public MP4Atom {
public:
	MP4UrnAtom();
	void Read();
};

class MP4StblAtom : public MP4Atom {
public:
	MP4StblAtom();
	void Generate();
};

class MP4StsdAtom : public MP4Atom {
public:
	MP4StsdAtom();
	void Read();
};

class MP4Mp4aAtom : public MP4Atom {
public:
	MP4Mp4aAtom();
	void Generate();
};

class MP4Mp4sAtom : public MP4Atom {
public:
	MP4Mp4sAtom();
	void Generate();
};

class MP4Mp4vAtom : public MP4Atom {
public:
	MP4Mp4vAtom();
	void Generate();
};

class MP4EsdsAtom : public MP4Atom {
public:
	MP4EsdsAtom();
};

class MP4SttsAtom : public MP4Atom {
public:
	MP4SttsAtom();
};

class MP4CttsAtom : public MP4Atom {
public:
	MP4CttsAtom();
};

class MP4StszAtom : public MP4Atom {
public:
	MP4StszAtom();
	void Read();
	void Write();
};

class MP4StscAtom : public MP4Atom {
public:
	MP4StscAtom();
	void Read();
};

class MP4StcoAtom : public MP4Atom {
public:
	MP4StcoAtom();
};

class MP4Co64Atom : public MP4Atom {
public:
	MP4Co64Atom();
};

class MP4StssAtom : public MP4Atom {
public:
	MP4StssAtom();
};

class MP4StshAtom : public MP4Atom {
public:
	MP4StshAtom();
};

class MP4StdpAtom : public MP4Atom {
public:
	MP4StdpAtom();
	void Read();
};

class MP4EdtsAtom : public MP4Atom {
public:
	MP4EdtsAtom();
};

class MP4ElstAtom : public MP4Atom {
public:
	MP4ElstAtom();
	void Generate();
	void Read();
protected:
	void AddProperties(u_int8_t version);
};

class MP4UdtaAtom : public MP4Atom {
public:
	MP4UdtaAtom();
	void Read();
};

class MP4CprtAtom : public MP4Atom {
public:
	MP4CprtAtom();
};

class MP4HntiAtom : public MP4Atom {
public:
	MP4HntiAtom();
	void Read();
};

class MP4RtpAtom : public MP4Atom {
public:
	MP4RtpAtom();
	void Generate();
	void Read();
	void Write();

protected:
	void AddPropertiesStsdType();
	void AddPropertiesHntiType();

	void GenerateStsdType();
	void GenerateHntiType();

	void ReadStsdType();
	void ReadHntiType();

	void WriteHntiType();
};

class MP4TimsAtom : public MP4Atom {
public:
	MP4TimsAtom();
};

class MP4TsroAtom : public MP4Atom {
public:
	MP4TsroAtom();
};

class MP4SnroAtom : public MP4Atom {
public:
	MP4SnroAtom();
};

class MP4SdpAtom : public MP4Atom {
public:
	MP4SdpAtom();
	void Read();
	void Write();
};

class MP4HinfAtom : public MP4Atom {
public:
	MP4HinfAtom();
	void Generate();
};

class MP4TrpyAtom : public MP4Atom {
public:
	MP4TrpyAtom();
};

class MP4NumpAtom : public MP4Atom {
public:
	MP4NumpAtom();
};

class MP4TpylAtom : public MP4Atom {
public:
	MP4TpylAtom();
};

class MP4MaxrAtom : public MP4Atom {
public:
	MP4MaxrAtom();
};

class MP4DmedAtom : public MP4Atom {
public:
	MP4DmedAtom();
};

class MP4DimmAtom : public MP4Atom {
public:
	MP4DimmAtom();
};

class MP4DrepAtom : public MP4Atom {
public:
	MP4DrepAtom();
};

class MP4TminAtom : public MP4Atom {
public:
	MP4TminAtom();
};

class MP4TmaxAtom : public MP4Atom {
public:
	MP4TmaxAtom();
};

class MP4PmaxAtom : public MP4Atom {
public:
	MP4PmaxAtom();
};

class MP4DmaxAtom : public MP4Atom {
public:
	MP4DmaxAtom();
};

class MP4PaytAtom : public MP4Atom {
public:
	MP4PaytAtom();
};

class MP4MvexAtom : public MP4Atom {
public:
	MP4MvexAtom();
};

class MP4TrexAtom : public MP4Atom {
public:
	MP4TrexAtom();
};

class MP4MoofAtom : public MP4Atom {
public:
	MP4MoofAtom();
};

class MP4MfhdAtom : public MP4Atom {
public:
	MP4MfhdAtom();
};

class MP4TrafAtom : public MP4Atom {
public:
	MP4TrafAtom();
};

class MP4TfhdAtom : public MP4Atom {
public:
	MP4TfhdAtom();
	void Read();
protected:
	void AddProperties(u_int32_t flags);
};

class MP4TrunAtom : public MP4Atom {
public:
	MP4TrunAtom();
	void Read();
protected:
	void AddProperties(u_int32_t flags);
};

class MP4FreeAtom : public MP4Atom {
public:
	MP4FreeAtom();
	void Read();
	void Write();
};

// ismacrypt
class MP4EncaAtom : public MP4Atom {
public:
        MP4EncaAtom();
        void Generate();
};

class MP4EncvAtom : public MP4Atom {
public:
        MP4EncvAtom();
        void Generate();
};

class MP4FrmaAtom : public MP4Atom {
public:
        MP4FrmaAtom();
};

class MP4IKMSAtom : public MP4Atom {
public:
        MP4IKMSAtom();
};

class MP4ISFMAtom : public MP4Atom {
public:
        MP4ISFMAtom();
};

class MP4SchiAtom : public MP4Atom {
public:
        MP4SchiAtom();
};

class MP4SchmAtom : public MP4Atom {
public:
        MP4SchmAtom();
};

class MP4SinfAtom : public MP4Atom {
public:
        MP4SinfAtom();
};

/* iTunes META data atoms */
class MP4MetaAtom : public MP4Atom {
public:
	MP4MetaAtom();
};

class MP4NameAtom : public MP4Atom {
public:
    MP4NameAtom();
    void Read();
};

class MP4MeanAtom : public MP4Atom {
public:
    MP4MeanAtom();
    void Read();
};

class MP4DataAtom : public MP4Atom {
public:
    MP4DataAtom();
    void Read();
};

class MP4IlstAtom : public MP4Atom {
public:
	MP4IlstAtom();
};

class MP4DashAtom : public MP4Atom {
public:
    MP4DashAtom();
};

class MP4NamAtom : public MP4Atom {
public:
	MP4NamAtom();
};

class MP4ArtAtom : public MP4Atom {
public:
	MP4ArtAtom();
};

class MP4WrtAtom : public MP4Atom {
public:
	MP4WrtAtom();
};

class MP4AlbAtom : public MP4Atom {
public:
	MP4AlbAtom();
};

class MP4GenAtom : public MP4Atom {
public:
	MP4GenAtom();
};

class MP4TrknAtom : public MP4Atom {
public:
	MP4TrknAtom();
};

class MP4DayAtom : public MP4Atom {
public:
	MP4DayAtom();
};

class MP4TooAtom : public MP4Atom {
public:
	MP4TooAtom();
};

class MP4GnreAtom : public MP4Atom {
public:
	MP4GnreAtom();
};

class MP4CpilAtom : public MP4Atom {
public:
	MP4CpilAtom();
};

class MP4TmpoAtom : public MP4Atom {
public:
	MP4TmpoAtom();
};

class MP4CovrAtom : public MP4Atom {
public:
	MP4CovrAtom();
};

class MP4CmtAtom : public MP4Atom {
public:
	MP4CmtAtom();
};

class MP4DiskAtom : public MP4Atom {
public:
	MP4DiskAtom();
};

#endif /* __MP4_ATOMS_INCLUDED__ */
