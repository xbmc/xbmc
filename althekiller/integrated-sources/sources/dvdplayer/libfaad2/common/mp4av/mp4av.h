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
 * Copyright (C) Cisco Systems Inc. 2001-2002.  All Rights Reserved.
 * 
 * Contributor(s): 
 *		Dave Mackie		dmackie@cisco.com
 */

#ifndef __MP4AV_INCLUDED__
#define __MP4AV_INCLUDED__ 

#include <mp4.h>

#ifdef __cplusplus
/* exploit C++ ability of default values for function parameters */
#define DEFAULT_PARM(x)	=x
#else
#define DEFAULT_PARM(x)
#endif

/* MP4AV library API */
#include "mp4av_aac.h"
#include "mp4av_adts.h"
#include "mp4av_mp3.h"
#include "mp4av_mpeg4.h"
#include "mp4av_audio.h"
#include "mp4av_hinters.h"
#include "mp4av_mpeg3.h"

#undef DEFAULT_PARM

#endif /* __MP4AV_INCLUDED__ */ 

