/**
 *
 * Alternative msacmdrv.h file, for use in the Lame project.
 * File from the FFDshow project, released under LGPL by permission
 * from Milan Cutka.
 * Modified by Gabriel Bouvigne to allow compilation of Lame.
 *
 *  Copyright (c) 2003 Milan Cutka
 *  Copyright (c) 2003 Gabriel Bouvigne
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef _MSACMDRV_H_
#define _MSACMDRV_H_


#include <mmreg.h>
#include <msacm.h>



typedef  unsigned long ULONG_PTR, *PULONG_PTR;



typedef ULONG_PTR DWORD_PTR, *PDWORD_PTR;


#undef ACMDRIVERDETAILS
#undef PACMDRIVERDETAILS
#undef LPACMDRIVERDETAILS

#define ACMDRIVERDETAILS        ACMDRIVERDETAILSW
#define PACMDRIVERDETAILS       PACMDRIVERDETAILSW
#define LPACMDRIVERDETAILS      LPACMDRIVERDETAILSW

#undef ACMFORMATTAGDETAILS
#undef PACMFORMATTAGDETAILS
#undef LPACMFORMATTAGDETAILS

#define ACMFORMATTAGDETAILS     ACMFORMATTAGDETAILSW
#define PACMFORMATTAGDETAILS    PACMFORMATTAGDETAILSW
#define LPACMFORMATTAGDETAILS   LPACMFORMATTAGDETAILSW

#undef ACMFORMATDETAILS
#undef PACMFORMATDETAILS
#undef LPACMFORMATDETAILS

#define ACMFORMATDETAILS	ACMFORMATDETAILSW
#define PACMFORMATDETAILS	PACMFORMATDETAILSW
#define LPACMFORMATDETAILS	LPACMFORMATDETAILSW


#define MAKE_ACM_VERSION(mjr, mnr, bld) (((long)(mjr)<<24)| \
                                         ((long)(mnr)<<16)| \
                                         ((long)bld))

#define VERSION_MSACM       MAKE_ACM_VERSION(3, 50, 0)
#define VERSION_ACM_DRIVER  MAKE_ACM_VERSION(4,  0, 0)


#define ACMDM_DRIVER_NOTIFY             (ACMDM_BASE + 1)
#define ACMDM_DRIVER_DETAILS            (ACMDM_BASE + 10)

#define ACMDM_HARDWARE_WAVE_CAPS_INPUT  (ACMDM_BASE + 20)
#define ACMDM_HARDWARE_WAVE_CAPS_OUTPUT (ACMDM_BASE + 21)

#define ACMDM_FORMATTAG_DETAILS         (ACMDM_BASE + 25)
#define ACMDM_FORMAT_DETAILS            (ACMDM_BASE + 26)
#define ACMDM_FORMAT_SUGGEST            (ACMDM_BASE + 27)

#define ACMDM_FILTERTAG_DETAILS         (ACMDM_BASE + 50)
#define ACMDM_FILTER_DETAILS            (ACMDM_BASE + 51)

#define ACMDM_STREAM_OPEN               (ACMDM_BASE + 76)
#define ACMDM_STREAM_CLOSE              (ACMDM_BASE + 77)
#define ACMDM_STREAM_SIZE               (ACMDM_BASE + 78)
#define ACMDM_STREAM_CONVERT            (ACMDM_BASE + 79)
#define ACMDM_STREAM_RESET              (ACMDM_BASE + 80)
#define ACMDM_STREAM_PREPARE            (ACMDM_BASE + 81)
#define ACMDM_STREAM_UNPREPARE          (ACMDM_BASE + 82)

typedef struct tACMDRVFORMATSUGGEST
{
    DWORD               cbStruct;           // sizeof(ACMDRVFORMATSUGGEST)
    DWORD               fdwSuggest;         // Suggest flags
    LPWAVEFORMATEX      pwfxSrc;            // Source Format
    DWORD               cbwfxSrc;           // Source Size
    LPWAVEFORMATEX      pwfxDst;            // Dest format
    DWORD               cbwfxDst;           // Dest Size

} ACMDRVFORMATSUGGEST, *PACMDRVFORMATSUGGEST, FAR *LPACMDRVFORMATSUGGEST;




typedef struct tACMDRVOPENDESC
{
    DWORD           cbStruct;       // sizeof(ACMDRVOPENDESC)
    FOURCC          fccType;        // 'audc'
    FOURCC          fccComp;        // sub-type (not used--must be 0)
    DWORD           dwVersion;      // current version of ACM opening you
    DWORD           dwFlags;        //
    DWORD           dwError;        // result from DRV_OPEN request
    LPCWSTR         pszSectionName; // see DRVCONFIGINFO.lpszDCISectionName
    LPCWSTR         pszAliasName;   // see DRVCONFIGINFO.lpszDCIAliasName
    DWORD           dnDevNode;	    // devnode id for pnp drivers.

} ACMDRVOPENDESC, *PACMDRVOPENDESC, FAR *LPACMDRVOPENDESC;


typedef struct tACMDRVSTREAMINSTANCE
{
    DWORD               cbStruct;
    LPWAVEFORMATEX      pwfxSrc;
    LPWAVEFORMATEX      pwfxDst;
    LPWAVEFILTER        pwfltr;
    DWORD_PTR           dwCallback;
    DWORD_PTR           dwInstance;
    DWORD               fdwOpen;
    DWORD               fdwDriver;
    DWORD_PTR           dwDriver;
    HACMSTREAM          has;

} ACMDRVSTREAMINSTANCE, *PACMDRVSTREAMINSTANCE, FAR *LPACMDRVSTREAMINSTANCE;

typedef struct tACMDRVSTREAMSIZE
{
    DWORD               cbStruct;
    DWORD               fdwSize;
    DWORD               cbSrcLength;
    DWORD               cbDstLength;

} ACMDRVSTREAMSIZE, *PACMDRVSTREAMSIZE, FAR *LPACMDRVSTREAMSIZE;

typedef struct tACMDRVSTREAMHEADER FAR *LPACMDRVSTREAMHEADER;
typedef struct tACMDRVSTREAMHEADER
{
    DWORD                   cbStruct;
    DWORD                   fdwStatus;
    DWORD_PTR               dwUser;
    LPBYTE                  pbSrc;
    DWORD                   cbSrcLength;
    DWORD                   cbSrcLengthUsed;
    DWORD_PTR               dwSrcUser;
    LPBYTE                  pbDst;
    DWORD                   cbDstLength;
    DWORD                   cbDstLengthUsed;
    DWORD_PTR               dwDstUser;

    DWORD                   fdwConvert;     // flags passed from convert func
    LPACMDRVSTREAMHEADER    padshNext;      // for async driver queueing
    DWORD                   fdwDriver;      // driver instance flags
    DWORD_PTR               dwDriver;       // driver instance data

    //
    //  all remaining fields are used by the ACM for bookkeeping purposes.
    //  an ACM driver should never use these fields (though than can be
    //  helpful for debugging)--note that the meaning of these fields
    //  may change, so do NOT rely on them in shipping code.
    //
    DWORD                   fdwPrepared;
    DWORD_PTR               dwPrepared;
    LPBYTE                  pbPreparedSrc;
    DWORD                   cbPreparedSrcLength;
    LPBYTE                  pbPreparedDst;
    DWORD                   cbPreparedDstLength;

} ACMDRVSTREAMHEADER, *PACMDRVSTREAMHEADER;

#endif
