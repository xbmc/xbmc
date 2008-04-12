/***************************************************************************/
/*                                                                         */
/*  afcjk.h                                                                */
/*                                                                         */
/*    Auto-fitter hinting routines for CJK script (specification).         */
/*                                                                         */
/*  Copyright 2006 by                                                      */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


#ifndef __AFCJK_H__
#define __AFCJK_H__

#include "afhints.h"


FT_BEGIN_HEADER


  /* the CJK-specific script class */

  FT_CALLBACK_TABLE const AF_ScriptClassRec
  af_cjk_script_class;


/* */

FT_END_HEADER

#endif /* __AFCJK_H__ */


/* END */
