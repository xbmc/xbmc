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

#ifndef __MP4_COMMON_INCLUDED__
#define __MP4_COMMON_INCLUDED__

// common includes for everything 
// with an internal view of the library
// i.e. all the .cpp's just #include "mp4common.h"

#include "mpeg4ip.h"

#include "mp4.h"
#include "mp4util.h"
#include "mp4array.h"
#include "mp4track.h"
#include "mp4file.h"
#include "mp4property.h"
#include "mp4container.h"
#include "mp4descriptor.h"
#include "mp4atom.h"

#include "atoms.h"
#include "descriptors.h"
#include "ocidescriptors.h"
#include "qosqualifiers.h"

#include "odcommands.h"

#include "rtphint.h"

#endif /* __MP4_COMMON_INCLUDED__ */
