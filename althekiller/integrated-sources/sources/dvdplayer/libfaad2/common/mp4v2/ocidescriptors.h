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

#ifndef __OCIDESCRIPTORS_INCLUDED__
#define __OCIDESCRIPTORS_INCLUDED__

const u_int8_t MP4OCIDescrTagsStart	 		= 0x40; 
const u_int8_t MP4ContentClassDescrTag 		= 0x40; 
const u_int8_t MP4KeywordDescrTag 			= 0x41; 
const u_int8_t MP4RatingDescrTag 			= 0x42; 
const u_int8_t MP4LanguageDescrTag	 		= 0x43;
const u_int8_t MP4ShortTextDescrTag	 		= 0x44;
const u_int8_t MP4ExpandedTextDescrTag 		= 0x45;
const u_int8_t MP4ContentCreatorDescrTag	= 0x46;
const u_int8_t MP4ContentCreationDescrTag	= 0x47;
const u_int8_t MP4OCICreatorDescrTag		= 0x48;
const u_int8_t MP4OCICreationDescrTag		= 0x49;
const u_int8_t MP4SmpteCameraDescrTag		= 0x4A;
const u_int8_t MP4OCIDescrTagsEnd			= 0x5F; 

class MP4ContentClassDescriptor : public MP4Descriptor {
public:
	MP4ContentClassDescriptor();
	void Read(MP4File* pFile);
};

class MP4KeywordDescriptor : public MP4Descriptor {
public:
	MP4KeywordDescriptor();
protected:
	void Mutate();
};

class MP4RatingDescriptor : public MP4Descriptor {
public:
	MP4RatingDescriptor();
	void Read(MP4File* pFile);
};

class MP4LanguageDescriptor : public MP4Descriptor {
public:
	MP4LanguageDescriptor();
};

class MP4ShortTextDescriptor : public MP4Descriptor {
public:
	MP4ShortTextDescriptor();
protected:
	void Mutate();
};

class MP4ExpandedTextDescriptor : public MP4Descriptor {
public:
	MP4ExpandedTextDescriptor();
protected:
	void Mutate();
};

class MP4CreatorDescriptor : public MP4Descriptor {
public:
	MP4CreatorDescriptor(u_int8_t tag);
};

class MP4CreationDescriptor : public MP4Descriptor {
public:
	MP4CreationDescriptor(u_int8_t tag);
};

class MP4SmpteCameraDescriptor : public MP4Descriptor {
public:
	MP4SmpteCameraDescriptor();
};

class MP4UnknownOCIDescriptor : public MP4Descriptor {
public:
	MP4UnknownOCIDescriptor();
	void Read(MP4File* pFile);
};


extern MP4Descriptor *CreateOCIDescriptor(u_int8_t tag);

#endif /* __OCIDESCRIPTORS_INCLUDED__ */
