/*
*	Blade DLL Interface for LAME.
*
*	Copyright (c) 1999 - 2002 A.L. Faber
*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation; either
* version 2 of the License, or (at your option) any later version.
* 
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Lesser General Public License for more details.
* 
* You should have received a copy of the GNU Lesser General Public
* License along with this library; if not, write to the
* Free Software Foundation, Inc., 59 Temple Place - Suite 330,
* Boston, MA  02111-1307, USA.
*/

#include <windows.h>
#include <Windef.h>
#include "BladeMP3EncDLL.h"
#include <assert.h>
#include <stdio.h>

#include <lame.h>


#define         Min(A, B)       ((A) < (B) ? (A) : (B))
#define         Max(A, B)       ((A) > (B) ? (A) : (B))

#define _RELEASEDEBUG 0

// lame_enc DLL version number
const int MAJORVERSION = 1;
const int MINORVERSION = 32;


// Local variables
static DWORD				dwSampleBufferSize=0;
static HANDLE				gs_hModule=NULL;
static BOOL					gs_bLogFile=FALSE;
static lame_global_flags*	gfp_save = NULL;

// Local function prototypes
static void dump_config( 	lame_global_flags*	gfp );
static void DebugPrintf( const char* pzFormat, ... );
static void DispErr( char const* strErr );
static void PresetOptions( lame_global_flags *gfp, LONG myPreset );


static void DebugPrintf(const char* pzFormat, ...)
{
    char	szBuffer[1024]={'\0',};
    char	szFileName[MAX_PATH+1]={'\0',};
    va_list ap;

    // Get the full module (DLL) file name
    GetModuleFileNameA(	gs_hModule, 
        szFileName,
        sizeof( szFileName ) );

    // change file name extention
    szFileName[ strlen(szFileName) - 3 ] = 't';
    szFileName[ strlen(szFileName) - 2 ] = 'x';
    szFileName[ strlen(szFileName) - 1 ] = 't';

    // start at beginning of the list
    va_start(ap, pzFormat);

    // copy it to the string buffer
    _vsnprintf(szBuffer, sizeof(szBuffer), pzFormat, ap);

    // log it to the file?
    if ( gs_bLogFile ) 
    {	
        FILE* fp = NULL;

        // try to open the log file
        fp=fopen( szFileName, "a+" );

        // check file open result
        if (fp)
        {
            // write string to the file
            fputs(szBuffer,fp);

            // close the file
            fclose(fp);
        }
    }

#if defined _DEBUG || defined _RELEASEDEBUG
    OutputDebugStringA( szBuffer );
#endif

    va_end(ap);
}


static void PresetOptions( lame_global_flags *gfp, LONG myPreset )
{
    switch (myPreset)
    {
        /*-1*/case LQP_NOPRESET:
            break;

        /*0*/case LQP_NORMAL_QUALITY:
            /*	lame_set_quality( gfp, 5 );*/
            break;

        /*1*/case LQP_LOW_QUALITY:
             lame_set_quality( gfp, 9 );
             break;

        /*2*/case LQP_HIGH_QUALITY:
             lame_set_quality( gfp, 2 );
             break;

        /*3*/case LQP_VOICE_QUALITY:				// --voice flag for experimental voice mode
             lame_set_mode( gfp, MONO );
             lame_set_preset( gfp, 56);
             break;

        /*4*/case LQP_R3MIX:					// --R3MIX
             lame_set_preset( gfp, R3MIX);
             break;

        /*5*/case LQP_VERYHIGH_QUALITY:
             lame_set_quality( gfp, 0 );
             break;

        /*6*/case LQP_STANDARD:				// --PRESET STANDARD
            lame_set_preset( gfp, STANDARD);
            break;

        /*7*/case LQP_FAST_STANDARD:				// --PRESET FAST STANDARD
            lame_set_preset( gfp, STANDARD_FAST);
            break;

        /*8*/case LQP_EXTREME:				// --PRESET EXTREME
            lame_set_preset( gfp, EXTREME);
            break;

        /*9*/case LQP_FAST_EXTREME:				// --PRESET FAST EXTREME:
            lame_set_preset( gfp, EXTREME_FAST);
            break;

        /*10*/case LQP_INSANE:				// --PRESET INSANE
            lame_set_preset( gfp, INSANE);
            break;

        /*11*/case LQP_ABR:					// --PRESET ABR
            // handled in beInitStream
            break;

        /*12*/case LQP_CBR:					// --PRESET CBR
            // handled in beInitStream
            break;

        /*13*/case LQP_MEDIUM:					// --PRESET MEDIUM
            lame_set_preset( gfp, MEDIUM);
            break;

        /*14*/case LQP_FAST_MEDIUM:					// --PRESET FAST MEDIUM
            lame_set_preset( gfp, MEDIUM_FAST);
            break;

        /*1000*/case LQP_PHONE:
            lame_set_mode( gfp, MONO );
            lame_set_preset( gfp, 16);
            break;

        /*2000*/case LQP_SW:
            lame_set_mode( gfp, MONO );
            lame_set_preset( gfp, 24);
            break;

        /*3000*/case LQP_AM:
            lame_set_mode( gfp, MONO );
            lame_set_preset( gfp, 40);
            break;

        /*4000*/case LQP_FM:
            lame_set_preset( gfp, 112);
            break;

        /*5000*/case LQP_VOICE:
            lame_set_mode( gfp, MONO );
            lame_set_preset( gfp, 56);
            break;

        /*6000*/case LQP_RADIO:
            lame_set_preset( gfp, 112);
            break;

        /*7000*/case LQP_TAPE:
            lame_set_preset( gfp, 112);
            break;

        /*8000*/case LQP_HIFI:
            lame_set_preset( gfp, 160);
            break;

        /*9000*/case LQP_CD:
            lame_set_preset( gfp, 192);
            break;

        /*10000*/case LQP_STUDIO:
            lame_set_preset( gfp, 256);
            break;

    }
}


__declspec(dllexport) BE_ERR	beInitStream(PBE_CONFIG pbeConfig, PDWORD dwSamples, PDWORD dwBufferSize, PHBE_STREAM phbeStream)
{
    int actual_bitrate;
    //2001-12-18
    int					nDllArgC = 0;
    BE_CONFIG			lameConfig = { 0, };
    int					nInitReturn = 0;
    lame_global_flags*	gfp = NULL;

    // Init the global flags structure
    gfp = lame_init();
    *phbeStream = (HBE_STREAM)gfp;

    // clear out structure
    memset(&lameConfig,0x00,CURRENT_STRUCT_SIZE);

    // Check if this is a regular BLADE_ENCODER header
    if (pbeConfig->dwConfig!=BE_CONFIG_LAME)
    {
        int nCRC=pbeConfig->format.mp3.bCRC;
        int nVBR=(nCRC>>12)&0x0F;

        // Copy parameter from old Blade structure
        lameConfig.format.LHV1.dwSampleRate	=pbeConfig->format.mp3.dwSampleRate;
        //for low bitrates, LAME will automatically downsample for better
        //sound quality.  Forcing output samplerate = input samplerate is not a good idea 
        //unless the user specifically requests it:
        //lameConfig.format.LHV1.dwReSampleRate=pbeConfig->format.mp3.dwSampleRate;
        lameConfig.format.LHV1.nMode		=(pbeConfig->format.mp3.byMode&0x0F);
        lameConfig.format.LHV1.dwBitrate	=pbeConfig->format.mp3.wBitrate;
        lameConfig.format.LHV1.bPrivate		=pbeConfig->format.mp3.bPrivate;
        lameConfig.format.LHV1.bOriginal	=pbeConfig->format.mp3.bOriginal;
        lameConfig.format.LHV1.bCRC		=nCRC&0x01;
        lameConfig.format.LHV1.bCopyright	=pbeConfig->format.mp3.bCopyright;

        // Fill out the unknowns
        lameConfig.format.LHV1.dwStructSize=CURRENT_STRUCT_SIZE;
        lameConfig.format.LHV1.dwStructVersion=CURRENT_STRUCT_VERSION;

        // Get VBR setting from fourth nibble
        if ( nVBR>0 )
        {
            lameConfig.format.LHV1.bWriteVBRHeader = TRUE;
            lameConfig.format.LHV1.bEnableVBR = TRUE;
            lameConfig.format.LHV1.nVBRQuality = nVBR-1;
        }

        // Get Quality from third nibble
        lameConfig.format.LHV1.nPreset=((nCRC>>8)&0x0F);

    }
    else
    {
        // Copy the parameters
        memcpy(&lameConfig,pbeConfig,pbeConfig->format.LHV1.dwStructSize);
    }

    // --------------- Set arguments to LAME encoder -------------------------

    // Set input sample frequency
    lame_set_in_samplerate( gfp, lameConfig.format.LHV1.dwSampleRate );

    // disable INFO/VBR tag by default.  
    // if this tag is used, the calling program must call beWriteVBRTag()
    // after encoding.  But the original DLL documentation does not 
    // require the 
    // app to call beWriteVBRTag() unless they have specifically
    // set LHV1.bWriteVBRHeader=TRUE.  Thus the default setting should
    // be disabled.  
    lame_set_bWriteVbrTag( gfp, 0 );

    //2001-12-18 Dibrom's ABR preset stuff

    if(lameConfig.format.LHV1.nPreset == LQP_ABR)		// --ALT-PRESET ABR
    {
        actual_bitrate = lameConfig.format.LHV1.dwVbrAbr_bps / 1000;

        // limit range
        if( actual_bitrate > 320)
        {
            actual_bitrate = 320;
        }

        if( actual_bitrate < 8 )
        {
            actual_bitrate = 8;
        }

        lame_set_preset( gfp, actual_bitrate );
    }    

    // end Dibrom's ABR preset 2001-12-18 ****** START OF CBR

    if(lameConfig.format.LHV1.nPreset == LQP_CBR)		// --ALT-PRESET CBR
    {
        actual_bitrate = lameConfig.format.LHV1.dwBitrate;
        lame_set_preset(gfp, actual_bitrate);
        lame_set_VBR(gfp, vbr_off);
    }

    // end Dibrom's CBR preset 2001-12-18

    // The following settings only used when preset is not one of the LAME QUALITY Presets
    if ( (int)lameConfig.format.LHV1.nPreset < (int) LQP_STANDARD )
    {
        switch ( lameConfig.format.LHV1.nMode )
        {
        case BE_MP3_MODE_STEREO:
            lame_set_mode( gfp, STEREO );
            lame_set_num_channels( gfp, 2 );
            break;
        case BE_MP3_MODE_JSTEREO:
            lame_set_mode( gfp, JOINT_STEREO );
            lame_set_num_channels( gfp, 2 );
            break;
        case BE_MP3_MODE_MONO:
            lame_set_mode( gfp, MONO );
            lame_set_num_channels( gfp, 1 );
            break;
        case BE_MP3_MODE_DUALCHANNEL: //warning: there is NO dual channel option working in Lame
            lame_set_force_ms( gfp, 1 );
            lame_set_mode( gfp, STEREO );
            lame_set_num_channels( gfp, 2 );
            break;
        default:
            {
                DebugPrintf("Invalid lameConfig.format.LHV1.nMode, value is %d\n",lameConfig.format.LHV1.nMode);
                return BE_ERR_INVALID_FORMAT_PARAMETERS;
            }
        }

        if ( lameConfig.format.LHV1.bEnableVBR )
        {
            /* set VBR quality */
            lame_set_VBR_q( gfp, lameConfig.format.LHV1.nVBRQuality );

            /* select proper VBR method */
            switch ( lameConfig.format.LHV1.nVbrMethod)
            {
            case VBR_METHOD_NONE:
                lame_set_VBR( gfp, vbr_off );
                break;

            case VBR_METHOD_DEFAULT:
                lame_set_VBR( gfp, vbr_default ); 
                break;

            case VBR_METHOD_OLD:
                lame_set_VBR( gfp, vbr_rh ); 
                break;

            case VBR_METHOD_MTRH:
            case VBR_METHOD_NEW:
                /*                                
                * the --vbr-mtrh commandline switch is obsolete. 
                * now --vbr-mtrh is known as --vbr-new
                */
                lame_set_VBR( gfp, vbr_mtrh ); 
                break;

            case VBR_METHOD_ABR:
                lame_set_VBR( gfp, vbr_abr ); 
                break;

            default:
                /* unsupported VBR method */
                assert( FALSE );
            }
        }
        else
        {
            /* use CBR encoding method, so turn off VBR */
            lame_set_VBR( gfp, vbr_off );
        }

        /* Set bitrate.  (CDex users always specify bitrate=Min bitrate when using VBR) */
        lame_set_brate( gfp, lameConfig.format.LHV1.dwBitrate );

        /* check if we have to use ABR, in order to backwards compatible, this
        * condition should still be checked indepedent of the nVbrMethod method
        */
        if (lameConfig.format.LHV1.dwVbrAbr_bps > 0 )
        {
            /* set VBR method to ABR */
            lame_set_VBR( gfp, vbr_abr );

            /* calculate to kbps, round to nearest kbps */
            lame_set_VBR_mean_bitrate_kbps( gfp, ( lameConfig.format.LHV1.dwVbrAbr_bps + 500 ) / 1000 );

            /* limit range */
            if( lame_get_VBR_mean_bitrate_kbps( gfp ) > 320)
            {
                lame_set_VBR_mean_bitrate_kbps( gfp, 320 );
            }

            if( lame_get_VBR_mean_bitrate_kbps( gfp ) < 8 )
            {
                lame_set_VBR_mean_bitrate_kbps( gfp, 8 );
            }
        }

    }

    // First set all the preset options
    if ( LQP_NOPRESET !=  lameConfig.format.LHV1.nPreset )
    {
        PresetOptions( gfp, lameConfig.format.LHV1.nPreset );
    }


    // Set frequency resampling rate, if specified
    if ( lameConfig.format.LHV1.dwReSampleRate > 0 )
    {
        lame_set_out_samplerate( gfp, lameConfig.format.LHV1.dwReSampleRate );
    }


    switch ( lameConfig.format.LHV1.nMode )
    {
    case BE_MP3_MODE_MONO:
        lame_set_mode( gfp, MONO );
        lame_set_num_channels( gfp, 1 );
        break;

    default:
        break;
    }


    // Use strict ISO encoding?
    lame_set_strict_ISO( gfp, ( lameConfig.format.LHV1.bStrictIso ) ? 1 : 0 );

    // Set copyright flag?
    if ( lameConfig.format.LHV1.bCopyright )
    {
        lame_set_copyright( gfp, 1 );
    }

    // Do we have to tag  it as non original 
    if ( !lameConfig.format.LHV1.bOriginal )
    {
        lame_set_original( gfp, 0 );
    }
    else
    {
        lame_set_original( gfp, 1 );
    }

    // Add CRC?
    if ( lameConfig.format.LHV1.bCRC )
    {
        lame_set_error_protection( gfp, 1 );
    }
    else
    {
        lame_set_error_protection( gfp, 0 );
    }

    // Set private bit?
    if ( lameConfig.format.LHV1.bPrivate )
    {
        lame_set_extension( gfp, 1 );
    }
    else
    {
        lame_set_extension( gfp, 0 );
    }


    // Set VBR min bitrate, if specified
    if ( lameConfig.format.LHV1.dwBitrate > 0 )
    {
        lame_set_VBR_min_bitrate_kbps( gfp, lameConfig.format.LHV1.dwBitrate );
    }

    // Set Maxbitrate, if specified
    if ( lameConfig.format.LHV1.dwMaxBitrate > 0 )
    {
        lame_set_VBR_max_bitrate_kbps( gfp, lameConfig.format.LHV1.dwMaxBitrate );
    }
    // Set bit resovoir option
    if ( lameConfig.format.LHV1.bNoRes )
    {
        lame_set_disable_reservoir( gfp,1 );
    }

    // check if the VBR tag is required
    if ( lameConfig.format.LHV1.bWriteVBRHeader ) 
    {
        lame_set_bWriteVbrTag( gfp, 1 );
    }
    else
    {
        lame_set_bWriteVbrTag( gfp, 0 );
    }

    // Override Quality setting, use HIGHBYTE = NOT LOWBYTE to be backwards compatible
    if (	( lameConfig.format.LHV1.nQuality & 0xFF ) ==
        ((~( lameConfig.format.LHV1.nQuality >> 8 )) & 0xFF) )
    {
        lame_set_quality( gfp, lameConfig.format.LHV1.nQuality & 0xFF );
    }

    if ( 0 != ( nInitReturn = lame_init_params( gfp ) ) )
    {
        return nInitReturn;
    }

    //LAME encoding call will accept any number of samples.  
    if ( 0 == lame_get_version( gfp ) )
    {
        // For MPEG-II, only 576 samples per frame per channel
        *dwSamples= 576 * lame_get_num_channels( gfp );
    }
    else
    {
        // For MPEG-I, 1152 samples per frame per channel
        *dwSamples= 1152 * lame_get_num_channels( gfp );
    }

    // Set the input sample buffer size, so we know what we can expect
    dwSampleBufferSize = *dwSamples;

    // Set MP3 buffer size, conservative estimate
    *dwBufferSize=(DWORD)( 1.25 * ( *dwSamples / lame_get_num_channels( gfp ) ) + 7200 );

    // For debugging purposes
    dump_config( gfp );

    // Everything went OK, thus return SUCCESSFUL
    return BE_ERR_SUCCESSFUL;
}



__declspec(dllexport) BE_ERR	beFlushNoGap(HBE_STREAM hbeStream, PBYTE pOutput, PDWORD pdwOutput)
{
    int nOutputSamples = 0;

    lame_global_flags*	gfp = (lame_global_flags*)hbeStream;

    // Init the global flags structure
    nOutputSamples = lame_encode_flush_nogap( gfp, pOutput, LAME_MAXMP3BUFFER );

    if ( nOutputSamples < 0 )
    {
        *pdwOutput = 0;
        return BE_ERR_BUFFER_TOO_SMALL;
    }
    else
    {
        *pdwOutput = nOutputSamples;
    }

    return BE_ERR_SUCCESSFUL;
}

__declspec(dllexport) BE_ERR	beDeinitStream(HBE_STREAM hbeStream, PBYTE pOutput, PDWORD pdwOutput)
{
    int nOutputSamples = 0;

    lame_global_flags*	gfp = (lame_global_flags*)hbeStream;

    nOutputSamples = lame_encode_flush( gfp, pOutput, 0 );

    if ( nOutputSamples < 0 )
    {
        *pdwOutput = 0;
        return BE_ERR_BUFFER_TOO_SMALL;
    }
    else
    {
        *pdwOutput = nOutputSamples;
    }

    return BE_ERR_SUCCESSFUL;
}


__declspec(dllexport) BE_ERR	beCloseStream(HBE_STREAM hbeStream)
{
    lame_global_flags*	gfp = (lame_global_flags*)hbeStream;

    // lame will be close in VbrWriteTag function
    if ( !lame_get_bWriteVbrTag( gfp ) )
    {
        // clean up of allocated memory
        lame_close( gfp );

        gfp_save = NULL;
    }
    else
    {
        gfp_save = (lame_global_flags*)hbeStream;
    }

    // DeInit encoder
    return BE_ERR_SUCCESSFUL;
}



__declspec(dllexport) VOID		beVersion(PBE_VERSION pbeVersion)
{
    // DLL Release date
    char lpszDate[20]	= { '\0', };
    char lpszTemp[5]	= { '\0', };
    lame_version_t lv   = { 0, };


    // Set DLL interface version
    pbeVersion->byDLLMajorVersion=MAJORVERSION;
    pbeVersion->byDLLMinorVersion=MINORVERSION;

    get_lame_version_numerical ( &lv );

    // Set Engine version number (Same as Lame version)
    pbeVersion->byMajorVersion = lv.major;
    pbeVersion->byMinorVersion = lv.minor;
    pbeVersion->byAlphaLevel   = lv.alpha;
    pbeVersion->byBetaLevel	   = lv.beta;

#ifdef MMX_choose_table
    pbeVersion->byMMXEnabled=1;
#else
    pbeVersion->byMMXEnabled=0;
#endif

    memset( pbeVersion->btReserved, 0, sizeof( pbeVersion->btReserved ) );

    // Get compilation date
    strcpy(lpszDate,__DATE__);

    // Get the first three character, which is the month
    strncpy(lpszTemp,lpszDate,3);
    lpszTemp[3] = '\0';
    pbeVersion->byMonth=1;

    // Set month
    if (strcmp(lpszTemp,"Jan")==0)	pbeVersion->byMonth = 1;
    if (strcmp(lpszTemp,"Feb")==0)	pbeVersion->byMonth = 2;
    if (strcmp(lpszTemp,"Mar")==0)	pbeVersion->byMonth = 3;
    if (strcmp(lpszTemp,"Apr")==0)	pbeVersion->byMonth = 4;
    if (strcmp(lpszTemp,"May")==0)	pbeVersion->byMonth = 5;
    if (strcmp(lpszTemp,"Jun")==0)	pbeVersion->byMonth = 6;
    if (strcmp(lpszTemp,"Jul")==0)	pbeVersion->byMonth = 7;
    if (strcmp(lpszTemp,"Aug")==0)	pbeVersion->byMonth = 8;
    if (strcmp(lpszTemp,"Sep")==0)	pbeVersion->byMonth = 9;
    if (strcmp(lpszTemp,"Oct")==0)	pbeVersion->byMonth = 10;
    if (strcmp(lpszTemp,"Nov")==0)	pbeVersion->byMonth = 11;
    if (strcmp(lpszTemp,"Dec")==0)	pbeVersion->byMonth = 12;

    // Get day of month string (char [4..5])
    pbeVersion->byDay=atoi( lpszDate + 4 );

    // Get year of compilation date (char [7..10])
    pbeVersion->wYear = atoi( lpszDate + 7 );

    memset( pbeVersion->zHomepage, 0x00, BE_MAX_HOMEPAGE );

    strcpy( pbeVersion->zHomepage, "http://www.mp3dev.org/" );
}

__declspec(dllexport) BE_ERR	beEncodeChunk(HBE_STREAM hbeStream, DWORD nSamples, 
                                              PSHORT pSamples, PBYTE pOutput, PDWORD pdwOutput)
{
    // Encode it
    int dwSamples;
    int	nOutputSamples = 0;
    lame_global_flags*	gfp = (lame_global_flags*)hbeStream;

    dwSamples = nSamples / lame_get_num_channels( gfp );

    // old versions of lame_enc.dll required exactly 1152 samples
    // and worked even if nSamples accidently set to 2304 
    // simulate this behavoir:
    if ( 1 == lame_get_num_channels( gfp ) && nSamples == 2304)
    {
        dwSamples/= 2;
    }


    if ( 1 == lame_get_num_channels( gfp ) )
    {
        nOutputSamples = lame_encode_buffer(gfp,pSamples,pSamples,dwSamples,pOutput,0);
    }
    else
    {
        nOutputSamples = lame_encode_buffer_interleaved(gfp,pSamples,dwSamples,pOutput,0);
    }


    if ( nOutputSamples < 0 )
    {
        *pdwOutput=0;
        return BE_ERR_BUFFER_TOO_SMALL;
    }
    else
    {
        *pdwOutput = (DWORD)nOutputSamples;
    }

    return BE_ERR_SUCCESSFUL;
}


// accept floating point audio samples, scaled to the range of a signed 16-bit
//  integer (within +/- 32768), in non-interleaved channels  -- DSPguru, jd
__declspec(dllexport) BE_ERR	beEncodeChunkFloatS16NI(HBE_STREAM hbeStream, DWORD nSamples, 
                                                        PFLOAT buffer_l, PFLOAT buffer_r, PBYTE pOutput, PDWORD pdwOutput)
{
    int nOutputSamples;
    lame_global_flags*	gfp = (lame_global_flags*)hbeStream;

    nOutputSamples = lame_encode_buffer_float(gfp,buffer_l,buffer_r,nSamples,pOutput,0);

    if ( nOutputSamples >= 0 )
    {
        *pdwOutput = (DWORD) nOutputSamples;
    }
    else
    {
        *pdwOutput=0;
        return BE_ERR_BUFFER_TOO_SMALL;
    }

    return BE_ERR_SUCCESSFUL;
}

static int
maybeSyncWord(FILE* fpStream)
{
    unsigned char mp3_frame_header[4];
    size_t nbytes = fread(mp3_frame_header, 1, sizeof(mp3_frame_header), fpStream);
    if ( nbytes != sizeof(mp3_frame_header) ) {
        return -1;
    }
    if ( mp3_frame_header[0] != 0xffu ) {
        return -1; /* doesn't look like a sync word */
    }
    if ( (mp3_frame_header[1] & 0xE0u) != 0xE0u ) {
        return -1; /* doesn't look like a sync word */
    }
    return 0;
}

static int
skipId3v2(FILE * fpStream, size_t lametag_frame_size)
{
    size_t  nbytes;
    size_t  id3v2TagSize = 0;
    unsigned char id3v2Header[10];

    /* seek to the beginning of the stream */
    if (fseek(fpStream, 0, SEEK_SET) != 0) {
        return -2;  /* not seekable, abort */
    }
    /* read 10 bytes in case there's an ID3 version 2 header here */
    nbytes = fread(id3v2Header, 1, sizeof(id3v2Header), fpStream);
    if (nbytes != sizeof(id3v2Header)) {
        return -3;  /* not readable, maybe opened Write-Only */
    }
    /* does the stream begin with the ID3 version 2 file identifier? */
    if (!strncmp((char *) id3v2Header, "ID3", 3)) {
        /* the tag size (minus the 10-byte header) is encoded into four
        * bytes where the most significant bit is clear in each byte
        */
        id3v2TagSize = (((id3v2Header[6] & 0x7f) << 21)
            | ((id3v2Header[7] & 0x7f) << 14)
            | ((id3v2Header[8] & 0x7f) << 7)
            | (id3v2Header[9] & 0x7f))
            + sizeof id3v2Header;
    }
    /* Seek to the beginning of the audio stream */
    if ( fseek(fpStream, id3v2TagSize, SEEK_SET) != 0 ) {
        return -2;
    }
    if ( maybeSyncWord(fpStream) != 0) {
        return -1;
    }
    if ( fseek(fpStream, id3v2TagSize+lametag_frame_size, SEEK_SET) != 0 ) {
        return -2;
    }
    if ( maybeSyncWord(fpStream) != 0) {
        return -1;
    }
    /* OK, it seems we found our LAME-Tag/Xing frame again */
    /* Seek to the beginning of the audio stream */
    if ( fseek(fpStream, id3v2TagSize, SEEK_SET) != 0 ) {
        return -2;
    }
    return 0;
}

static BE_ERR
updateLameTagFrame(lame_global_flags* gfp, FILE* fpStream)
{
    size_t n = lame_get_lametag_frame( gfp, 0, 0 ); /* ask for bufer size */

    if ( n > 0 )
    {
        unsigned char* buffer = 0;
        size_t m = 1;

        if ( 0 != skipId3v2(fpStream, n) ) 
        {
            DispErr( "Error updating LAME-tag frame:\n\n"
                     "can't locate old frame\n" );
            return BE_ERR_INVALID_FORMAT_PARAMETERS;
        }

        buffer = malloc( n );

        if ( buffer == 0 ) 
        {
            DispErr( "Error updating LAME-tag frame:\n\n"
                     "can't allocate frame buffer\n" );
            return BE_ERR_INVALID_FORMAT_PARAMETERS;
        }

        /* Put it all to disk again */
        n = lame_get_lametag_frame( gfp, buffer, n );
        if ( n > 0 ) 
        {
            m = fwrite( buffer, n, 1, fpStream );        
        }
        free( buffer );

        if ( m != 1 ) 
        {
            DispErr( "Error updating LAME-tag frame:\n\n"
                     "couldn't write frame into file\n" );
            return BE_ERR_INVALID_FORMAT_PARAMETERS;
        }
    }
    return BE_ERR_SUCCESSFUL;
}

__declspec(dllexport) BE_ERR beWriteInfoTag( HBE_STREAM hbeStream,
                                            LPCSTR lpszFileName )
{
    FILE* fpStream	= NULL;
    BE_ERR beResult	= BE_ERR_SUCCESSFUL;

    lame_global_flags*	gfp = (lame_global_flags*)hbeStream;

    if ( NULL != gfp )
    {
        // Do we have to write the VBR tag?
        if ( lame_get_bWriteVbrTag( gfp ) )
        {
            // Try to open the file
            fpStream=fopen( lpszFileName, "rb+" );

            // Check file open result
            if ( NULL == fpStream )
            {
                beResult = BE_ERR_INVALID_FORMAT_PARAMETERS;
                DispErr( "Error updating LAME-tag frame:\n\n"
                         "can't open file for reading and writing\n" );
            }
            else
            {
                beResult = updateLameTagFrame( gfp, fpStream );

                // Close the file stream
                fclose( fpStream );
            }
        }

        // clean up of allocated memory
        lame_close( gfp );
    }
    else
    {
        beResult = BE_ERR_INVALID_FORMAT_PARAMETERS;
    }

    // return result
    return beResult;
}

// for backwards compatiblity
__declspec(dllexport) BE_ERR beWriteVBRHeader(LPCSTR lpszFileName)
{
    return beWriteInfoTag( (HBE_STREAM)gfp_save, lpszFileName );
}


BOOL APIENTRY DllMain(HANDLE hModule, 
                      DWORD  ul_reason_for_call, 
                      LPVOID lpReserved)
{
    gs_hModule=hModule;

    switch( ul_reason_for_call )
    {
    case DLL_PROCESS_ATTACH:
        // Enable debug/logging?
        gs_bLogFile = GetPrivateProfileIntA("Debug","WriteLogFile",gs_bLogFile,"lame_enc.ini");
        break;
    case DLL_THREAD_ATTACH:
        break;
    case DLL_THREAD_DETACH:
        break;
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}


static void dump_config( 	lame_global_flags*	gfp )
{
    DebugPrintf("\n\nLame_enc configuration options:\n");
    DebugPrintf("==========================================================\n");

    DebugPrintf("version                =%d\n",lame_get_version( gfp ) );
    DebugPrintf("Layer                  =3\n");
    DebugPrintf("mode                   =");
    switch ( lame_get_mode( gfp ) )
    {
    case STEREO:       DebugPrintf( "Stereo\n" ); break;
    case JOINT_STEREO: DebugPrintf( "Joint-Stereo\n" ); break;
    case DUAL_CHANNEL: DebugPrintf( "Forced Stereo\n" ); break;
    case MONO:         DebugPrintf( "Mono\n" ); break;
    case NOT_SET:      /* FALLTROUGH */
    default:           DebugPrintf( "Error (unknown)\n" ); break;
    }

    DebugPrintf("Input sample rate      =%.1f kHz\n", lame_get_in_samplerate( gfp ) /1000.0 );
    DebugPrintf("Output sample rate     =%.1f kHz\n", lame_get_out_samplerate( gfp ) /1000.0 );

    DebugPrintf("bitrate                =%d kbps\n", lame_get_brate( gfp ) );
    DebugPrintf("Quality Setting        =%d\n", lame_get_quality( gfp ) );

    DebugPrintf("Low pass frequency     =%d\n", lame_get_lowpassfreq( gfp ) );
    DebugPrintf("Low pass width         =%d\n", lame_get_lowpasswidth( gfp ) );

    DebugPrintf("High pass frequency    =%d\n", lame_get_highpassfreq( gfp ) );
    DebugPrintf("High pass width        =%d\n", lame_get_highpasswidth( gfp ) );

    DebugPrintf("No short blocks        =%d\n", lame_get_no_short_blocks( gfp ) );
    DebugPrintf("Force short blocks     =%d\n", lame_get_force_short_blocks( gfp ) );

    DebugPrintf("de-emphasis            =%d\n", lame_get_emphasis( gfp ) );
    DebugPrintf("private flag           =%d\n", lame_get_extension( gfp ) );

    DebugPrintf("copyright flag         =%d\n", lame_get_copyright( gfp ) );
    DebugPrintf("original flag          =%d\n",	lame_get_original( gfp ) );
    DebugPrintf("CRC                    =%s\n", lame_get_error_protection( gfp ) ? "on" : "off" );
    DebugPrintf("Fast mode              =%s\n", ( lame_get_quality( gfp ) )? "enabled" : "disabled" );
    DebugPrintf("Force mid/side stereo  =%s\n", ( lame_get_force_ms( gfp ) )?"enabled":"disabled" );
    DebugPrintf("Disable Reservoir      =%d\n", lame_get_disable_reservoir( gfp ) );
    DebugPrintf("Allow diff-short       =%d\n", lame_get_allow_diff_short( gfp ) );
    DebugPrintf("Interchannel masking   =%f\n", lame_get_interChRatio( gfp ) );
    DebugPrintf("Strict ISO Encoding    =%s\n", ( lame_get_strict_ISO( gfp ) ) ?"Yes":"No");
    DebugPrintf("Scale                  =%5.2f\n", lame_get_scale( gfp ) );

    DebugPrintf("VBR                    =%s, VBR_q =%d, VBR method =",
        ( lame_get_VBR( gfp ) !=vbr_off ) ? "enabled": "disabled",
        lame_get_VBR_q( gfp ) );

    switch ( lame_get_VBR( gfp ) )
    {
    case vbr_off:	DebugPrintf( "vbr_off\n" );	break;
    case vbr_mt :	DebugPrintf( "vbr_mt \n" );	break;
    case vbr_rh :	DebugPrintf( "vbr_rh \n" );	break;
    case vbr_mtrh:	DebugPrintf( "vbr_mtrh \n" );	break;
    case vbr_abr: 
        DebugPrintf( "vbr_abr (average bitrate %d kbps)\n", lame_get_VBR_mean_bitrate_kbps( gfp ) );
        break;
    default:
        DebugPrintf("error, unknown VBR setting\n");
        break;
    }

    DebugPrintf("Vbr Min bitrate        =%d kbps\n", lame_get_VBR_min_bitrate_kbps( gfp ) );
    DebugPrintf("Vbr Max bitrate        =%d kbps\n", lame_get_VBR_max_bitrate_kbps( gfp ) );

    DebugPrintf("Write VBR Header       =%s\n", ( lame_get_bWriteVbrTag( gfp ) ) ?"Yes":"No");
    DebugPrintf("VBR Hard min           =%d\n", lame_get_VBR_hard_min( gfp ) );

    DebugPrintf("ATH Only               =%d\n", lame_get_ATHonly( gfp ) );
    DebugPrintf("ATH short              =%d\n", lame_get_ATHshort( gfp ) );
    DebugPrintf("ATH no                 =%d\n", lame_get_noATH( gfp ) );
    DebugPrintf("ATH type               =%d\n", lame_get_ATHtype( gfp ) );
    DebugPrintf("ATH lower              =%f\n", lame_get_ATHlower( gfp ) );
    DebugPrintf("ATH aa                 =%d\n", lame_get_athaa_type( gfp ) );
    DebugPrintf("ATH aa  loudapprox     =%d\n", lame_get_athaa_loudapprox( gfp ) );
    DebugPrintf("ATH aa  sensitivity    =%f\n", lame_get_athaa_sensitivity( gfp ) );

    DebugPrintf("Experimental nspsytune =%d\n", lame_get_exp_nspsytune( gfp ) );
    DebugPrintf("Experimental X         =%d\n", lame_get_experimentalX( gfp ) );
    DebugPrintf("Experimental Y         =%d\n", lame_get_experimentalY( gfp ) );
    DebugPrintf("Experimental Z         =%d\n", lame_get_experimentalZ( gfp ) );
}


static void DispErr(char const* strErr)
{
    MessageBoxA(NULL,strErr,"LAME_ENC.DLL",MB_OK|MB_ICONHAND);
}
