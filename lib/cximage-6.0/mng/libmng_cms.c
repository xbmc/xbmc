/* ************************************************************************** */
/* *             For conditions of distribution and use,                    * */
/* *                see copyright notice in libmng.h                        * */
/* ************************************************************************** */
/* *                                                                        * */
/* * project   : libmng                                                     * */
/* * file      : libmng_cms.c              copyright (c) 2000-2004 G.Juyn   * */
/* * version   : 1.0.9                                                      * */
/* *                                                                        * */
/* * purpose   : color management routines (implementation)                 * */
/* *                                                                        * */
/* * author    : G.Juyn                                                     * */
/* *                                                                        * */
/* * comment   : implementation of the color management routines            * */
/* *                                                                        * */
/* * changes   : 0.5.1 - 05/01/2000 - G.Juyn                                * */
/* *             - B001(105795) - fixed a typo and misconception about      * */
/* *               freeing allocated gamma-table. (reported by Marti Maria) * */
/* *             0.5.1 - 05/08/2000 - G.Juyn                                * */
/* *             - changed strict-ANSI stuff                                * */
/* *             0.5.1 - 05/09/2000 - G.Juyn                                * */
/* *             - filled application-based color-management routines       * */
/* *             0.5.1 - 05/11/2000 - G.Juyn                                * */
/* *             - added creatememprofile                                   * */
/* *             - added callback error-reporting support                   * */
/* *             0.5.1 - 05/12/2000 - G.Juyn                                * */
/* *             - changed trace to macro for callback error-reporting      * */
/* *                                                                        * */
/* *             0.5.2 - 06/10/2000 - G.Juyn                                * */
/* *             - fixed some compilation-warnings (contrib Jason Morris)   * */
/* *                                                                        * */
/* *             0.5.3 - 06/21/2000 - G.Juyn                                * */
/* *             - fixed problem with color-correction for stored images    * */
/* *             0.5.3 - 06/23/2000 - G.Juyn                                * */
/* *             - fixed problem with incorrect gamma-correction            * */
/* *                                                                        * */
/* *             0.9.2 - 08/05/2000 - G.Juyn                                * */
/* *             - changed file-prefixes                                    * */
/* *                                                                        * */
/* *             0.9.3 - 08/31/2000 - G.Juyn                                * */
/* *             - fixed sRGB precedence for gamma_only corection           * */
/* *                                                                        * */
/* *             0.9.4 - 12/16/2000 - G.Juyn                                * */
/* *             - fixed mixup of data- & function-pointers (thanks Dimitri)* */
/* *                                                                        * */
/* *             1.0.1 - 03/31/2001 - G.Juyn                                * */
/* *             - ignore gamma=0 (see png-list for more info)              * */
/* *             1.0.1 - 04/25/2001 - G.Juyn (reported by Gregg Kelly)      * */
/* *             - fixed problem with cms profile being created multiple    * */
/* *               times when both iCCP & cHRM/gAMA are present             * */
/* *             1.0.1 - 04/25/2001 - G.Juyn                                * */
/* *             - moved mng_clear_cms to libmng_cms                        * */
/* *             1.0.1 - 05/02/2001 - G.Juyn                                * */
/* *             - added "default" sRGB generation (Thanks Marti!)          * */
/* *                                                                        * */
/* *             1.0.5 - 08/19/2002 - G.Juyn                                * */
/* *             - B597134 - libmng pollutes the linker namespace           * */
/* *             1.0.5 - 09/19/2002 - G.Juyn                                * */
/* *             - optimized color-correction routines                      * */
/* *             1.0.5 - 09/23/2002 - G.Juyn                                * */
/* *             - added in-memory color-correction of abstract images      * */
/* *             1.0.5 - 11/08/2002 - G.Juyn                                * */
/* *             - fixed issues in init_app_cms()                           * */
/* *                                                                        * */
/* *             1.0.6 - 04/11/2003 - G.Juyn                                * */
/* *             - B719420 - fixed several MNG_APP_CMS problems             * */
/* *             1.0.6 - 07/11/2003 - G. R-P                                * */
/* *             - added conditional MNG_SKIPCHUNK_cHRM/iCCP                * */
/* *                                                                        * */
/* *             1.0.9 - 12/20/2004 - G.Juyn                                * */
/* *             - cleaned up macro-invocations (thanks to D. Airlie)       * */
/* *                                                                        * */
/* ************************************************************************** */

#include "libmng.h"
#include "libmng_data.h"
#include "libmng_error.h"
#include "libmng_trace.h"
#ifdef __BORLANDC__
#pragma hdrstop
#endif
#include "libmng_objects.h"
#include "libmng_cms.h"

#if defined(__BORLANDC__) && defined(MNG_STRICT_ANSI)
#pragma option -A                      /* force ANSI-C */
#endif

/* ************************************************************************** */

#ifdef MNG_INCLUDE_DISPLAY_PROCS

/* ************************************************************************** */
/* *                                                                        * */
/* * Little CMS helper routines                                             * */
/* *                                                                        * */
/* ************************************************************************** */

#ifdef MNG_INCLUDE_LCMS

#define MNG_CMS_FLAGS 0

/* ************************************************************************** */

void mnglcms_initlibrary ()
{
  cmsErrorAction (LCMS_ERROR_IGNORE);  /* LCMS should ignore errors! */
}

/* ************************************************************************** */

mng_cmsprof mnglcms_createfileprofile (mng_pchar zFilename)
{
  return cmsOpenProfileFromFile (zFilename, "r");
}

/* ************************************************************************** */

mng_cmsprof mnglcms_creatememprofile (mng_uint32 iProfilesize,
                                      mng_ptr    pProfile)
{
  return cmsOpenProfileFromMem (pProfile, iProfilesize);
}

/* ************************************************************************** */

mng_cmsprof mnglcms_createsrgbprofile (void)
{
  cmsCIExyY       D65;
  cmsCIExyYTRIPLE Rec709Primaries = {
                                      {0.6400, 0.3300, 1.0},
                                      {0.3000, 0.6000, 1.0},
                                      {0.1500, 0.0600, 1.0}
                                    };
  LPGAMMATABLE    Gamma24[3];
  mng_cmsprof     hsRGB;

  cmsWhitePointFromTemp(6504, &D65);
  Gamma24[0] = Gamma24[1] = Gamma24[2] = cmsBuildGamma(256, 2.4);
  hsRGB = cmsCreateRGBProfile(&D65, &Rec709Primaries, Gamma24);
  cmsFreeGamma(Gamma24[0]);

  return hsRGB;
}

/* ************************************************************************** */

void mnglcms_freeprofile (mng_cmsprof hProf)
{
  cmsCloseProfile (hProf);
  return;
}

/* ************************************************************************** */

void mnglcms_freetransform (mng_cmstrans hTrans)
{
/* B001 start */
  cmsDeleteTransform (hTrans);
/* B001 end */
  return;
}

/* ************************************************************************** */

mng_retcode mng_clear_cms (mng_datap pData)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_CLEAR_CMS, MNG_LC_START);
#endif

  if (pData->hTrans)                   /* transformation still active ? */
    mnglcms_freetransform (pData->hTrans);

  pData->hTrans = 0;

  if (pData->hProf1)                   /* file profile still active ? */
    mnglcms_freeprofile (pData->hProf1);

  pData->hProf1 = 0;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_CLEAR_CMS, MNG_LC_END);
#endif

  return MNG_NOERROR;
}

/* ************************************************************************** */

#endif /* MNG_INCLUDE_LCMS */

/* ************************************************************************** */
/* *                                                                        * */
/* * Color-management initialization & correction routines                  * */
/* *                                                                        * */
/* ************************************************************************** */

#ifdef MNG_INCLUDE_LCMS

mng_retcode mng_init_full_cms (mng_datap pData,
                               mng_bool  bGlobal,
                               mng_bool  bObject,
                               mng_bool  bRetrobj)
{
  mng_cmsprof    hProf;
  mng_cmstrans   hTrans;
  mng_imagep     pImage = MNG_NULL;
  mng_imagedatap pBuf   = MNG_NULL;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_FULL_CMS, MNG_LC_START);
#endif

  if (bObject)                         /* use object if present ? */
  {                                    /* current object ? */
    if ((mng_imagep)pData->pCurrentobj)
      pImage = (mng_imagep)pData->pCurrentobj;
    else                               /* if not; use object 0 */
      pImage = (mng_imagep)pData->pObjzero;
  }

  if (bRetrobj)                        /* retrieving from an object ? */
    pImage = (mng_imagep)pData->pRetrieveobj;

  if (pImage)                          /* are we using an object ? */
    pBuf = pImage->pImgbuf;            /* then address the buffer */

  if ((!pBuf) || (!pBuf->bCorrected))  /* is the buffer already corrected ? */
  {
#ifndef MNG_SKIPCHUNK_iCCP
    if (((pBuf) && (pBuf->bHasICCP)) || ((bGlobal) && (pData->bHasglobalICCP)))
    {
      if (!pData->hProf2)              /* output profile not defined ? */
      {                                /* then assume sRGB !! */
        pData->hProf2 = mnglcms_createsrgbprofile ();

        if (!pData->hProf2)            /* handle error ? */
          MNG_ERRORL (pData, MNG_LCMS_NOHANDLE);
      }

      if ((pBuf) && (pBuf->bHasICCP))  /* generate a profile handle */
        hProf = cmsOpenProfileFromMem (pBuf->pProfile, pBuf->iProfilesize);
      else
        hProf = cmsOpenProfileFromMem (pData->pGlobalProfile, pData->iGlobalProfilesize);

      pData->hProf1 = hProf;           /* save for future use */

      if (!hProf)                      /* handle error ? */
        MNG_ERRORL (pData, MNG_LCMS_NOHANDLE);

#ifndef MNG_NO_16BIT_SUPPORT
      if (pData->bIsRGBA16)            /* 16-bit intermediates ? */
        hTrans = cmsCreateTransform (hProf,         TYPE_RGBA_16_SE,
                                     pData->hProf2, TYPE_RGBA_16_SE,
                                     INTENT_PERCEPTUAL, MNG_CMS_FLAGS);
      else
#endif
        hTrans = cmsCreateTransform (hProf,         TYPE_RGBA_8,
                                     pData->hProf2, TYPE_RGBA_8,
                                     INTENT_PERCEPTUAL, MNG_CMS_FLAGS);

      pData->hTrans = hTrans;          /* save for future use */

      if (!hTrans)                     /* handle error ? */
        MNG_ERRORL (pData, MNG_LCMS_NOTRANS);
                                       /* load color-correction routine */
      pData->fCorrectrow = (mng_fptr)mng_correct_full_cms;

      return MNG_NOERROR;              /* and done */
    }
    else
#endif
    if (((pBuf) && (pBuf->bHasSRGB)) || ((bGlobal) && (pData->bHasglobalSRGB)))
    {
      mng_uint8 iIntent;

      if (pData->bIssRGB)              /* sRGB system ? */
        return MNG_NOERROR;            /* no conversion required */

      if (!pData->hProf3)              /* sRGB profile not defined ? */
      {                                /* then create it implicitly !! */
        pData->hProf3 = mnglcms_createsrgbprofile ();

        if (!pData->hProf3)            /* handle error ? */
          MNG_ERRORL (pData, MNG_LCMS_NOHANDLE);
      }

      hProf = pData->hProf3;           /* convert from sRGB profile */

      if ((pBuf) && (pBuf->bHasSRGB))  /* determine rendering intent */
        iIntent = pBuf->iRenderingintent;
      else
        iIntent = pData->iGlobalRendintent;

      if (pData->bIsRGBA16)            /* 16-bit intermediates ? */
        hTrans = cmsCreateTransform (hProf,         TYPE_RGBA_16_SE,
                                     pData->hProf2, TYPE_RGBA_16_SE,
                                     iIntent, MNG_CMS_FLAGS);
      else
        hTrans = cmsCreateTransform (hProf,         TYPE_RGBA_8,
                                     pData->hProf2, TYPE_RGBA_8,
                                     iIntent, MNG_CMS_FLAGS);

      pData->hTrans = hTrans;          /* save for future use */

      if (!hTrans)                     /* handle error ? */
        MNG_ERRORL (pData, MNG_LCMS_NOTRANS);
                                       /* load color-correction routine */
      pData->fCorrectrow = (mng_fptr)mng_correct_full_cms;

      return MNG_NOERROR;              /* and done */
    }
    else
    if ( (((pBuf) && (pBuf->bHasCHRM)) || ((bGlobal) && (pData->bHasglobalCHRM))) &&
         ( ((pBuf) && (pBuf->bHasGAMA) && (pBuf->iGamma > 0)) ||
           ((bGlobal) && (pData->bHasglobalGAMA) && (pData->iGlobalGamma > 0))  )    )
    {
      mng_CIExyY       sWhitepoint;
      mng_CIExyYTRIPLE sPrimaries;
      mng_gammatabp    pGammatable[3];
      mng_float        dGamma;

      if (!pData->hProf2)              /* output profile not defined ? */
      {                                /* then assume sRGB !! */
        pData->hProf2 = mnglcms_createsrgbprofile ();

        if (!pData->hProf2)            /* handle error ? */
          MNG_ERRORL (pData, MNG_LCMS_NOHANDLE);
      }

#ifndef MNG_SKIPCHUNK_cHRM
      if ((pBuf) && (pBuf->bHasCHRM))  /* local cHRM ? */
      {
        sWhitepoint.x      = (mng_float)pBuf->iWhitepointx   / 100000;
        sWhitepoint.y      = (mng_float)pBuf->iWhitepointy   / 100000;
        sPrimaries.Red.x   = (mng_float)pBuf->iPrimaryredx   / 100000;
        sPrimaries.Red.y   = (mng_float)pBuf->iPrimaryredy   / 100000;
        sPrimaries.Green.x = (mng_float)pBuf->iPrimarygreenx / 100000;
        sPrimaries.Green.y = (mng_float)pBuf->iPrimarygreeny / 100000;
        sPrimaries.Blue.x  = (mng_float)pBuf->iPrimarybluex  / 100000;
        sPrimaries.Blue.y  = (mng_float)pBuf->iPrimarybluey  / 100000;
      }
      else
      {
        sWhitepoint.x      = (mng_float)pData->iGlobalWhitepointx   / 100000;
        sWhitepoint.y      = (mng_float)pData->iGlobalWhitepointy   / 100000;
        sPrimaries.Red.x   = (mng_float)pData->iGlobalPrimaryredx   / 100000;
        sPrimaries.Red.y   = (mng_float)pData->iGlobalPrimaryredy   / 100000;
        sPrimaries.Green.x = (mng_float)pData->iGlobalPrimarygreenx / 100000;
        sPrimaries.Green.y = (mng_float)pData->iGlobalPrimarygreeny / 100000;
        sPrimaries.Blue.x  = (mng_float)pData->iGlobalPrimarybluex  / 100000;
        sPrimaries.Blue.y  = (mng_float)pData->iGlobalPrimarybluey  / 100000;
      }
#endif

      sWhitepoint.Y      =             /* Y component is always 1.0 */
      sPrimaries.Red.Y   =
      sPrimaries.Green.Y =
      sPrimaries.Blue.Y  = 1.0;

      if ((pBuf) && (pBuf->bHasGAMA))  /* get the gamma value */
        dGamma = (mng_float)pBuf->iGamma / 100000;
      else
        dGamma = (mng_float)pData->iGlobalGamma / 100000;

      dGamma = pData->dViewgamma / dGamma;

      pGammatable [0] =                /* and build the lookup tables */
      pGammatable [1] =
      pGammatable [2] = cmsBuildGamma (256, dGamma);

      if (!pGammatable [0])            /* enough memory ? */
        MNG_ERRORL (pData, MNG_LCMS_NOMEM);
                                       /* create the profile */
      hProf = cmsCreateRGBProfile (&sWhitepoint, &sPrimaries, pGammatable);

      cmsFreeGamma (pGammatable [0]);  /* free the temporary gamma tables ? */
                                       /* yes! but just the one! */

      pData->hProf1 = hProf;           /* save for future use */

      if (!hProf)                      /* handle error ? */
        MNG_ERRORL (pData, MNG_LCMS_NOHANDLE);

      if (pData->bIsRGBA16)            /* 16-bit intermediates ? */
        hTrans = cmsCreateTransform (hProf,         TYPE_RGBA_16_SE,
                                     pData->hProf2, TYPE_RGBA_16_SE,
                                     INTENT_PERCEPTUAL, MNG_CMS_FLAGS);
      else
        hTrans = cmsCreateTransform (hProf,         TYPE_RGBA_8,
                                     pData->hProf2, TYPE_RGBA_8,
                                     INTENT_PERCEPTUAL, MNG_CMS_FLAGS);

      pData->hTrans = hTrans;          /* save for future use */

      if (!hTrans)                     /* handle error ? */
        MNG_ERRORL (pData, MNG_LCMS_NOTRANS);
                                       /* load color-correction routine */
      pData->fCorrectrow = (mng_fptr)mng_correct_full_cms;

      return MNG_NOERROR;              /* and done */
    }
  }

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_FULL_CMS, MNG_LC_END);
#endif
                                       /* if we get here, we'll only do gamma */
  return mng_init_gamma_only (pData, bGlobal, bObject, bRetrobj);
}
#endif /* MNG_INCLUDE_LCMS */

/* ************************************************************************** */

#ifdef MNG_INCLUDE_LCMS
mng_retcode mng_correct_full_cms (mng_datap pData)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_CORRECT_FULL_CMS, MNG_LC_START);
#endif

  cmsDoTransform (pData->hTrans, pData->pRGBArow, pData->pRGBArow, pData->iRowsamples);

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_CORRECT_FULL_CMS, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif /* MNG_INCLUDE_LCMS */

/* ************************************************************************** */

#if defined(MNG_GAMMA_ONLY) || defined(MNG_FULL_CMS) || defined(MNG_APP_CMS)
mng_retcode mng_init_gamma_only (mng_datap pData,
                                 mng_bool  bGlobal,
                                 mng_bool  bObject,
                                 mng_bool  bRetrobj)
{
  mng_float      dGamma;
  mng_imagep     pImage = MNG_NULL;
  mng_imagedatap pBuf   = MNG_NULL;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_GAMMA_ONLY, MNG_LC_START);
#endif

  if (bObject)                         /* use object if present ? */
  {                                    /* current object ? */
    if ((mng_imagep)pData->pCurrentobj)
      pImage = (mng_imagep)pData->pCurrentobj;
    else                               /* if not; use object 0 */
      pImage = (mng_imagep)pData->pObjzero;
  }

  if (bRetrobj)                        /* retrieving from an object ? */
    pImage = (mng_imagep)pData->pRetrieveobj;

  if (pImage)                          /* are we using an object ? */
    pBuf = pImage->pImgbuf;            /* then address the buffer */

  if ((!pBuf) || (!pBuf->bCorrected))  /* is the buffer already corrected ? */
  {
    if ((pBuf) && (pBuf->bHasSRGB))    /* get the gamma value */
      dGamma = 0.45455;
    else
    if ((pBuf) && (pBuf->bHasGAMA))
      dGamma = (mng_float)pBuf->iGamma / 100000;
    else
    if ((bGlobal) && (pData->bHasglobalSRGB))
      dGamma = 0.45455;
    else
    if ((bGlobal) && (pData->bHasglobalGAMA))
      dGamma = (mng_float)pData->iGlobalGamma / 100000;
    else
      dGamma = pData->dDfltimggamma;

    if (dGamma > 0)                    /* ignore gamma=0 */
    {
      dGamma = pData->dViewgamma / (dGamma * pData->dDisplaygamma);

      if (dGamma != pData->dLastgamma) /* lookup table needs to be computed ? */
      {
        mng_int32 iX;

        pData->aGammatab [0] = 0;

        for (iX = 1; iX <= 255; iX++)
          pData->aGammatab [iX] = (mng_uint8)(pow (iX / 255.0, dGamma) * 255 + 0.5);

        pData->dLastgamma = dGamma;    /* keep for next time */
      }
                                       /* load color-correction routine */
      pData->fCorrectrow = (mng_fptr)mng_correct_gamma_only;
    }
  }

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_GAMMA_ONLY, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif /* MNG_GAMMA_ONLY || MNG_FULL_CMS || MNG_APP_CMS */

/* ************************************************************************** */

#if defined(MNG_GAMMA_ONLY) || defined(MNG_FULL_CMS) || defined(MNG_APP_CMS)
mng_retcode mng_correct_gamma_only (mng_datap pData)
{
  mng_uint8p pWork;
  mng_int32  iX;

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_CORRECT_GAMMA_ONLY, MNG_LC_START);
#endif

  pWork = pData->pRGBArow;             /* address intermediate row */

  if (pData->bIsRGBA16)                /* 16-bit intermediate row ? */
  {

  
     /* TODO: 16-bit precision gamma processing */
     /* we'll just do the high-order byte for now */

     
                                       /* convert all samples in the row */
     for (iX = 0; iX < pData->iRowsamples; iX++)
     {                                 /* using the precalculated gamma lookup table */
       *pWork     = pData->aGammatab [*pWork];
       *(pWork+2) = pData->aGammatab [*(pWork+2)];
       *(pWork+4) = pData->aGammatab [*(pWork+4)];

       pWork += 8;
     }
  }
  else
  {                                    /* convert all samples in the row */
     for (iX = 0; iX < pData->iRowsamples; iX++)
     {                                 /* using the precalculated gamma lookup table */
       *pWork     = pData->aGammatab [*pWork];
       *(pWork+1) = pData->aGammatab [*(pWork+1)];
       *(pWork+2) = pData->aGammatab [*(pWork+2)];

       pWork += 4;
     }
  }

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_CORRECT_GAMMA_ONLY, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif /* MNG_GAMMA_ONLY || MNG_FULL_CMS || MNG_APP_CMS */

/* ************************************************************************** */

#ifdef MNG_APP_CMS
mng_retcode mng_init_app_cms (mng_datap pData,
                              mng_bool  bGlobal,
                              mng_bool  bObject,
                              mng_bool  bRetrobj)
{
  mng_imagep     pImage = MNG_NULL;
  mng_imagedatap pBuf   = MNG_NULL;
  mng_bool       bDone  = MNG_FALSE;
  mng_retcode    iRetcode;
  
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_APP_CMS, MNG_LC_START);
#endif

  if (bObject)                         /* use object if present ? */
  {                                    /* current object ? */
    if ((mng_imagep)pData->pCurrentobj)
      pImage = (mng_imagep)pData->pCurrentobj;
    else                               /* if not; use object 0 */
      pImage = (mng_imagep)pData->pObjzero;
  }

  if (bRetrobj)                        /* retrieving from an object ? */
    pImage = (mng_imagep)pData->pRetrieveobj;

  if (pImage)                          /* are we using an object ? */
    pBuf = pImage->pImgbuf;            /* then address the buffer */

  if ((!pBuf) || (!pBuf->bCorrected))  /* is the buffer already corrected ? */
  {
#ifndef MNG_SKIPCHUNK_iCCP
    if ( (pData->fProcessiccp) &&
         (((pBuf) && (pBuf->bHasICCP)) || ((bGlobal) && (pData->bHasglobalICCP))) )
    {
      mng_uint32 iProfilesize;
      mng_ptr    pProfile;

      if ((pBuf) && (pBuf->bHasICCP))  /* get the right profile */
      {
        iProfilesize = pBuf->iProfilesize;
        pProfile     = pBuf->pProfile;
      }
      else
      {
        iProfilesize = pData->iGlobalProfilesize;
        pProfile     = pData->pGlobalProfile;
      }
                                       /* inform the app */
      if (!pData->fProcessiccp ((mng_handle)pData, iProfilesize, pProfile))
        MNG_ERROR (pData, MNG_APPCMSERROR);
                                       /* load color-correction routine */
      pData->fCorrectrow = (mng_fptr)mng_correct_app_cms;
      bDone              = MNG_TRUE;
    }
#endif

    if ( (pData->fProcesssrgb) &&
         (((pBuf) && (pBuf->bHasSRGB)) || ((bGlobal) && (pData->bHasglobalSRGB))) )
    {
      mng_uint8 iIntent;

      if ((pBuf) && (pBuf->bHasSRGB))  /* determine rendering intent */
        iIntent = pBuf->iRenderingintent;
      else
        iIntent = pData->iGlobalRendintent;
                                       /* inform the app */
      if (!pData->fProcesssrgb ((mng_handle)pData, iIntent))
        MNG_ERROR (pData, MNG_APPCMSERROR);
                                       /* load color-correction routine */
      pData->fCorrectrow = (mng_fptr)mng_correct_app_cms;
      bDone              = MNG_TRUE;
    }

#ifndef MNG_SKIPCHUNK_cHRM
    if ( (pData->fProcesschroma) &&
         (((pBuf) && (pBuf->bHasCHRM)) || ((bGlobal) && (pData->bHasglobalCHRM))) )
    {
      mng_uint32 iWhitepointx,   iWhitepointy;
      mng_uint32 iPrimaryredx,   iPrimaryredy;
      mng_uint32 iPrimarygreenx, iPrimarygreeny;
      mng_uint32 iPrimarybluex,  iPrimarybluey;

      if ((pBuf) && (pBuf->bHasCHRM))  /* local cHRM ? */
      {
        iWhitepointx   = pBuf->iWhitepointx;
        iWhitepointy   = pBuf->iWhitepointy;
        iPrimaryredx   = pBuf->iPrimaryredx;
        iPrimaryredy   = pBuf->iPrimaryredy;
        iPrimarygreenx = pBuf->iPrimarygreenx;
        iPrimarygreeny = pBuf->iPrimarygreeny;
        iPrimarybluex  = pBuf->iPrimarybluex;
        iPrimarybluey  = pBuf->iPrimarybluey;
      }
      else
      {
        iWhitepointx   = pData->iGlobalWhitepointx;
        iWhitepointy   = pData->iGlobalWhitepointy;
        iPrimaryredx   = pData->iGlobalPrimaryredx;
        iPrimaryredy   = pData->iGlobalPrimaryredy;
        iPrimarygreenx = pData->iGlobalPrimarygreenx;
        iPrimarygreeny = pData->iGlobalPrimarygreeny;
        iPrimarybluex  = pData->iGlobalPrimarybluex;
        iPrimarybluey  = pData->iGlobalPrimarybluey;
      }
                                       /* inform the app */
      if (!pData->fProcesschroma ((mng_handle)pData, iWhitepointx,   iWhitepointy,
                                                     iPrimaryredx,   iPrimaryredy,
                                                     iPrimarygreenx, iPrimarygreeny,
                                                     iPrimarybluex,  iPrimarybluey))
        MNG_ERROR (pData, MNG_APPCMSERROR);
                                       /* load color-correction routine */
      pData->fCorrectrow = (mng_fptr)mng_correct_app_cms;
      bDone              = MNG_TRUE;
    }
#endif

    if ( (pData->fProcessgamma) &&
         (((pBuf) && (pBuf->bHasGAMA)) || ((bGlobal) && (pData->bHasglobalGAMA))) )
    {
      mng_uint32 iGamma;

      if ((pBuf) && (pBuf->bHasGAMA))  /* get the gamma value */
        iGamma = pBuf->iGamma;
      else
        iGamma = pData->iGlobalGamma;
                                       /* inform the app */
      if (!pData->fProcessgamma ((mng_handle)pData, iGamma))
      {                                /* app wants us to use internal routines ! */
        iRetcode = mng_init_gamma_only (pData, bGlobal, bObject, bRetrobj);
        if (iRetcode)                  /* on error bail out */
          return iRetcode;
      }
      else
      {                                /* load color-correction routine */
        pData->fCorrectrow = (mng_fptr)mng_correct_app_cms;
      }

      bDone = MNG_TRUE;
    }

    if (!bDone)                        /* no color-info at all ? */
    {
                                       /* then use default image gamma ! */
      if (!pData->fProcessgamma ((mng_handle)pData,
                                 (mng_uint32)((pData->dDfltimggamma * 100000) + 0.5)))
      {                                /* app wants us to use internal routines ! */
        iRetcode = mng_init_gamma_only (pData, bGlobal, bObject, bRetrobj);
        if (iRetcode)                  /* on error bail out */
          return iRetcode;
      }
      else
      {                                /* load color-correction routine */
        pData->fCorrectrow = (mng_fptr)mng_correct_app_cms;
      }  
    }
  }

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_INIT_APP_CMS, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif /* MNG_APP_CMS */

/* ************************************************************************** */

#ifdef MNG_APP_CMS
mng_retcode mng_correct_app_cms (mng_datap pData)
{
#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_CORRECT_APP_CMS, MNG_LC_START);
#endif

  if (pData->fProcessarow)             /* let the app do something with our row */
    if (!pData->fProcessarow ((mng_handle)pData, pData->iRowsamples,
                              pData->bIsRGBA16, pData->pRGBArow))
      MNG_ERROR (pData, MNG_APPCMSERROR);

#ifdef MNG_SUPPORT_TRACE
  MNG_TRACE (pData, MNG_FN_CORRECT_APP_CMS, MNG_LC_END);
#endif

  return MNG_NOERROR;
}
#endif /* MNG_APP_CMS */

/* ************************************************************************** */

#endif /* MNG_INCLUDE_DISPLAY_PROCS */

/* ************************************************************************** */
/* * end of file                                                            * */
/* ************************************************************************** */



