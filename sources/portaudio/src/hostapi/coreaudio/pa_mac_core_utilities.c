/*
 * Helper and utility functions for pa_mac_core.c (Apple AUHAL implementation)
 *
 * PortAudio Portable Real-Time Audio Library
 * Latest Version at: http://www.portaudio.com
 *
 * Written by Bjorn Roche of XO Audio LLC, from PA skeleton code.
 * Portions copied from code by Dominic Mazzoni (who wrote a HAL implementation)
 *
 * Dominic's code was based on code by Phil Burk, Darren Gibbs,
 * Gord Peters, Stephane Letz, and Greg Pfiel.
 *
 * The following people also deserve acknowledgements:
 *
 * Olivier Tristan for feedback and testing
 * Glenn Zelniker and Z-Systems engineering for sponsoring the Blocking I/O
 * interface.
 * 
 *
 * Based on the Open Source API proposed by Ross Bencina
 * Copyright (c) 1999-2002 Ross Bencina, Phil Burk
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * The text above constitutes the entire PortAudio license; however, 
 * the PortAudio community also makes the following non-binding requests:
 *
 * Any person wishing to distribute modifications to the Software is
 * requested to send the modifications to the original developer so that
 * they can be incorporated into the canonical version. It is also 
 * requested that these non-binding requests be included along with the 
 * license above.
 */

/**
 @file
 @ingroup hostapi_src
*/

#include "pa_mac_core_utilities.h"

PaError PaMacCore_SetUnixError( int err, int line )
{
   PaError ret;
   const char *errorText;

   if( err == 0 )
   {
      return paNoError;
   }

   ret = paNoError;
   errorText = strerror( err );

   /** Map Unix error to PaError. Pretty much the only one that maps
       is ENOMEM. */
   if( err == ENOMEM )
      ret = paInsufficientMemory;
   else
      ret = paInternalError;

   DBUG(("%d on line %d: msg='%s'\n", err, line, errorText));
   PaUtil_SetLastHostErrorInfo( paCoreAudio, err, errorText );

   return ret;
}

/*
 * Translates MacOS generated errors into PaErrors
 */
PaError PaMacCore_SetError(OSStatus error, int line, int isError)
{
    /*FIXME: still need to handle possible ComponentResult values.*/
    /*       unfortunately, they don't seem to be documented anywhere.*/
    PaError result;
    const char *errorType; 
    const char *errorText;
    
    switch (error) {
    case kAudioHardwareNoError:
        return paNoError;
    case kAudioHardwareNotRunningError:
        errorText = "Audio Hardware Not Running";
        result = paInternalError; break;
    case kAudioHardwareUnspecifiedError: 
        errorText = "Unspecified Audio Hardware Error";
        result = paInternalError; break;
    case kAudioHardwareUnknownPropertyError:
        errorText = "Audio Hardware: Unknown Property";
        result = paInternalError; break;
    case kAudioHardwareBadPropertySizeError:
        errorText = "Audio Hardware: Bad Property Size";
        result = paInternalError; break;
    case kAudioHardwareIllegalOperationError: 
        errorText = "Audio Hardware: Illegal Operation";
        result = paInternalError; break;
    case kAudioHardwareBadDeviceError:
        errorText = "Audio Hardware: Bad Device";
        result = paInvalidDevice; break;
    case kAudioHardwareBadStreamError:
        errorText = "Audio Hardware: BadStream";
        result = paBadStreamPtr; break;
    case kAudioHardwareUnsupportedOperationError:
        errorText = "Audio Hardware: Unsupported Operation";
        result = paInternalError; break;
    case kAudioDeviceUnsupportedFormatError:
        errorText = "Audio Device: Unsupported Format";
        result = paSampleFormatNotSupported; break;
    case kAudioDevicePermissionsError:
        errorText = "Audio Device: Permissions Error";
        result = paDeviceUnavailable; break;
    /* Audio Unit Errors: http://developer.apple.com/documentation/MusicAudio/Reference/CoreAudio/audio_units/chapter_5_section_3.html */
    case kAudioUnitErr_InvalidProperty:
        errorText = "Audio Unit: Invalid Property";
        result = paInternalError; break;
    case kAudioUnitErr_InvalidParameter:
        errorText = "Audio Unit: Invalid Parameter";
        result = paInternalError; break;
    case kAudioUnitErr_NoConnection:
        errorText = "Audio Unit: No Connection";
        result = paInternalError; break;
    case kAudioUnitErr_FailedInitialization:
        errorText = "Audio Unit: Initialization Failed";
        result = paInternalError; break;
    case kAudioUnitErr_TooManyFramesToProcess:
        errorText = "Audio Unit: Too Many Frames";
        result = paInternalError; break;
    case kAudioUnitErr_IllegalInstrument:
        errorText = "Audio Unit: Illegal Instrument";
        result = paInternalError; break;
    case kAudioUnitErr_InstrumentTypeNotFound:
        errorText = "Audio Unit: Instrument Type Not Found";
        result = paInternalError; break;
    case kAudioUnitErr_InvalidFile:
        errorText = "Audio Unit: Invalid File";
        result = paInternalError; break;
    case kAudioUnitErr_UnknownFileType:
        errorText = "Audio Unit: Unknown File Type";
        result = paInternalError; break;
    case kAudioUnitErr_FileNotSpecified:
        errorText = "Audio Unit: File Not Specified";
        result = paInternalError; break;
    case kAudioUnitErr_FormatNotSupported:
        errorText = "Audio Unit: Format Not Supported";
        result = paInternalError; break;
    case kAudioUnitErr_Uninitialized:
        errorText = "Audio Unit: Unitialized";
        result = paInternalError; break;
    case kAudioUnitErr_InvalidScope:
        errorText = "Audio Unit: Invalid Scope";
        result = paInternalError; break;
    case kAudioUnitErr_PropertyNotWritable:
        errorText = "Audio Unit: PropertyNotWritable";
        result = paInternalError; break;
    case kAudioUnitErr_InvalidPropertyValue:
        errorText = "Audio Unit: Invalid Property Value";
        result = paInternalError; break;
    case kAudioUnitErr_PropertyNotInUse:
        errorText = "Audio Unit: Property Not In Use";
        result = paInternalError; break;
    case kAudioUnitErr_Initialized:
        errorText = "Audio Unit: Initialized";
        result = paInternalError; break;
    case kAudioUnitErr_InvalidOfflineRender:
        errorText = "Audio Unit: Invalid Offline Render";
        result = paInternalError; break;
    case kAudioUnitErr_Unauthorized:
        errorText = "Audio Unit: Unauthorized";
        result = paInternalError; break;
    case kAudioUnitErr_CannotDoInCurrentContext:
        errorText = "Audio Unit: cannot do in current context";
        result = paInternalError; break;
    default:
        errorText = "Unknown Error";
        result = paInternalError;
    }

    if (isError)
        errorType = "Error";
    else
        errorType = "Warning";

    char str[20];
    // see if it appears to be a 4-char-code
    *(UInt32 *)(str + 1) = CFSwapInt32HostToBig(error);
    if (isprint(str[1]) && isprint(str[2]) && isprint(str[3]) && isprint(str[4]))
    {
        str[0] = str[5] = '\'';
        str[6] = '\0';
    } else {
        // no, format it as an integer
        sprintf(str, "%d", (int)error);
    }

    DBUG(("%s on line %d: err='%s', msg=%s\n", errorType, line, str, errorText));

    PaUtil_SetLastHostErrorInfo( paCoreAudio, error, errorText );

    return result;
}

/*
 * This function computes an appropriate ring buffer size given
 * a requested latency (in seconds), sample rate and framesPerBuffer.
 *
 * The returned ringBufferSize is computed using the following
 * constraints:
 *   - it must be at least 4.
 *   - it must be at least 3x framesPerBuffer.
 *   - it must be at least 2x the suggestedLatency.
 *   - it must be a power of 2.
 * This function attempts to compute the minimum such size.
 *
 * FEEDBACK: too liberal/conservative/another way?
 */
long computeRingBufferSize( const PaStreamParameters *inputParameters,
                                   const PaStreamParameters *outputParameters,
                                   long inputFramesPerBuffer,
                                   long outputFramesPerBuffer,
                                   double sampleRate )
{
   long ringSize;
   int index;
   int i;
   double latencyTimesChannelCount ;
   long framesPerBufferTimesChannelCount ;

   VVDBUG(( "computeRingBufferSize()\n" ));

   assert( inputParameters || outputParameters );

   if( outputParameters && inputParameters )
   {
      latencyTimesChannelCount = MAX(
           inputParameters->suggestedLatency * inputParameters->channelCount,
           outputParameters->suggestedLatency * outputParameters->channelCount );
      framesPerBufferTimesChannelCount = MAX(
           inputFramesPerBuffer * inputParameters->channelCount,
           outputFramesPerBuffer * outputParameters->channelCount );
   } 
   else if( outputParameters )
   {
      latencyTimesChannelCount
                = outputParameters->suggestedLatency * outputParameters->channelCount;
      framesPerBufferTimesChannelCount
                = outputFramesPerBuffer * outputParameters->channelCount;
   }
   else /* we have inputParameters  */
   {
      latencyTimesChannelCount
                = inputParameters->suggestedLatency * inputParameters->channelCount;
      framesPerBufferTimesChannelCount
                = inputFramesPerBuffer * inputParameters->channelCount;
   }

   ringSize = (long) ( latencyTimesChannelCount * sampleRate * 2 + .5);
   VDBUG( ( "suggested latency * channelCount: %d\n", (int) (latencyTimesChannelCount*sampleRate) ) );
   if( ringSize < framesPerBufferTimesChannelCount * 3 )
      ringSize = framesPerBufferTimesChannelCount * 3 ;
   VDBUG(("framesPerBuffer*channelCount:%d\n",(int)framesPerBufferTimesChannelCount));
   VDBUG(("Ringbuffer size (1): %d\n", (int)ringSize ));

   /* make sure it's at least 4 */
   ringSize = MAX( ringSize, 4 );

   /* round up to the next power of 2 */
   index = -1;
   for( i=0; i<sizeof(long)*8; ++i )
      if( ringSize >> i & 0x01 )
         index = i;
   assert( index > 0 );
   if( ringSize <= ( 0x01 << index ) )
      ringSize = 0x01 << index ;
   else
      ringSize = 0x01 << ( index + 1 );

   VDBUG(( "Final Ringbuffer size (2): %d\n", (int)ringSize ));
   return ringSize;
}

/*
 * Durring testing of core audio, I found that serious crashes could occur
 * if properties such as sample rate were changed multiple times in rapid
 * succession. The function below has some fancy logic to make sure that changes
 * are acknowledged before another is requested. That seems to help a lot.
 */

OSStatus propertyProc(
    AudioDeviceID inDevice, 
    UInt32 inChannel, 
    Boolean isInput, 
    AudioDevicePropertyID inPropertyID, 
    void* inClientData )
{
   MutexAndBool *mab = (MutexAndBool *) inClientData;
   mab->once = TRUE;
   pthread_mutex_unlock( &(mab->mutex) );
   return noErr;
}

/* sets the value of the given property and waits for the change to 
   be acknowledged, and returns the final value, which is not guaranteed
   by this function to be the same as the desired value. Obviously, this
   function can only be used for data whose input and output are the
   same size and format, and their size and format are known in advance.*/
PaError AudioDeviceSetPropertyNowAndWaitForChange(
    AudioDeviceID inDevice,
    UInt32 inChannel, 
    Boolean isInput, 
    AudioDevicePropertyID inPropertyID,
    UInt32 inPropertyDataSize, 
    const void *inPropertyData,
    void *outPropertyData )
{
   OSStatus macErr;
   int unixErr;
   MutexAndBool mab;
   UInt32 outPropertyDataSize = inPropertyDataSize;

   /* First, see if it already has that value. If so, return. */
   macErr = AudioDeviceGetProperty( inDevice, inChannel,
                                 isInput, inPropertyID, 
                                 &outPropertyDataSize, outPropertyData );
   if( macErr )
      goto failMac2;
   if( inPropertyDataSize!=outPropertyDataSize )
      return paInternalError;
   if( 0==memcmp( outPropertyData, inPropertyData, outPropertyDataSize ) )
      return paNoError;

   /* setup and lock mutex */
   mab.once = FALSE;
   unixErr = pthread_mutex_init( &mab.mutex, NULL );
   if( unixErr )
      goto failUnix2;
   unixErr = pthread_mutex_lock( &mab.mutex );
   if( unixErr )
      goto failUnix;

   /* add property listener */
   macErr = AudioDeviceAddPropertyListener( inDevice, inChannel, isInput,
                                   inPropertyID, propertyProc,
                                   &mab ); 
   if( macErr )
      goto failMac;
   /* set property */
   macErr  = AudioDeviceSetProperty( inDevice, NULL, inChannel,
                                 isInput, inPropertyID,
                                 inPropertyDataSize, inPropertyData );
   if( macErr ) {
      /* we couldn't set the property, so we'll just unlock the mutex
         and move on. */
      pthread_mutex_unlock( &mab.mutex );
   }

   /* wait for property to change */                      
   unixErr = pthread_mutex_lock( &mab.mutex );
   if( unixErr )
      goto failUnix;

   /* now read the property back out */
   macErr = AudioDeviceGetProperty( inDevice, inChannel,
                                 isInput, inPropertyID, 
                                 &outPropertyDataSize, outPropertyData );
   if( macErr )
      goto failMac;
   /* cleanup */
   AudioDeviceRemovePropertyListener( inDevice, inChannel, isInput,
                                      inPropertyID, propertyProc );
   unixErr = pthread_mutex_unlock( &mab.mutex );
   if( unixErr )
      goto failUnix2;
   unixErr = pthread_mutex_destroy( &mab.mutex );
   if( unixErr )
      goto failUnix2;

   return paNoError;

 failUnix:
   pthread_mutex_destroy( &mab.mutex );
   AudioDeviceRemovePropertyListener( inDevice, inChannel, isInput,
                                      inPropertyID, propertyProc );

 failUnix2:
   DBUG( ("Error #%d while setting a device property: %s\n", unixErr, strerror( unixErr ) ) );
   return paUnanticipatedHostError;

 failMac:
   pthread_mutex_destroy( &mab.mutex );
   AudioDeviceRemovePropertyListener( inDevice, inChannel, isInput,
                                      inPropertyID, propertyProc );
 failMac2:
   return ERR( macErr );
}

/*
 * Sets the sample rate the HAL device.
 * if requireExact: set the sample rate or fail.
 *
 * otherwise      : set the exact sample rate.
 *             If that fails, check for available sample rates, and choose one
 *             higher than the requested rate. If there isn't a higher one,
 *             just use the highest available.
 */
PaError setBestSampleRateForDevice( const AudioDeviceID device,
                                    const bool isOutput,
                                    const bool requireExact,
                                    const Float64 desiredSrate )
{
   /*FIXME: changing the sample rate is disruptive to other programs using the
            device, so it might be good to offer a custom flag to not change the
            sample rate and just do conversion. (in my casual tests, there is
            no disruption unless the sample rate really does need to change) */
   const bool isInput = isOutput ? 0 : 1;
   Float64 srate;
   UInt32 propsize = sizeof( Float64 );
   OSErr err;
   AudioValueRange *ranges;
   int i=0;
   Float64 max  = -1; /*the maximum rate available*/
   Float64 best = -1; /*the lowest sample rate still greater than desired rate*/
   VDBUG(("Setting sample rate for device %ld to %g.\n",device,(float)desiredSrate));

   /* -- try setting the sample rate -- */
   err = AudioDeviceSetPropertyNowAndWaitForChange(
                                 device, 0, isInput,
                                 kAudioDevicePropertyNominalSampleRate,
                                 propsize, &desiredSrate, &srate );
   if( err )
      return err;

   /* -- if the rate agrees, and we got no errors, we are done -- */
   if( !err && srate == desiredSrate )
      return paNoError;
   /* -- we've failed if the rates disagree and we are setting input -- */
   if( requireExact )
      return paInvalidSampleRate;

   /* -- generate a list of available sample rates -- */
   err = AudioDeviceGetPropertyInfo( device, 0, isInput,
                                kAudioDevicePropertyAvailableNominalSampleRates,
                                &propsize, NULL );
   if( err )
      return ERR( err );
   ranges = (AudioValueRange *)calloc( 1, propsize );
   if( !ranges )
      return paInsufficientMemory;
   err = AudioDeviceGetProperty( device, 0, isInput,
                                kAudioDevicePropertyAvailableNominalSampleRates,
                                &propsize, ranges );
   if( err )
   {
      free( ranges );
      return ERR( err );
   }
   VDBUG(("Requested sample rate of %g was not available.\n", (float)desiredSrate));
   VDBUG(("%lu Available Sample Rates are:\n",propsize/sizeof(AudioValueRange)));
#ifdef MAC_CORE_VERBOSE_DEBUG
   for( i=0; i<propsize/sizeof(AudioValueRange); ++i )
      VDBUG( ("\t%g-%g\n",
              (float) ranges[i].mMinimum,
              (float) ranges[i].mMaximum ) );
#endif
   VDBUG(("-----\n"));
   
   /* -- now pick the best available sample rate -- */
   for( i=0; i<propsize/sizeof(AudioValueRange); ++i )
   {
      if( ranges[i].mMaximum > max ) max = ranges[i].mMaximum;
      if( ranges[i].mMinimum > desiredSrate ) {
         if( best < 0 )
            best = ranges[i].mMinimum;
         else if( ranges[i].mMinimum < best )
            best = ranges[i].mMinimum;
      }
   }
   if( best < 0 )
      best = max;
   VDBUG( ("Maximum Rate %g. best is %g.\n", max, best ) );
   free( ranges );

   /* -- set the sample rate -- */
   propsize = sizeof( best );
   err = AudioDeviceSetPropertyNowAndWaitForChange(
                                 device, 0, isInput,
                                 kAudioDevicePropertyNominalSampleRate,
                                 propsize, &best, &srate );
   if( err )
      return err;

   if( err )
      return ERR( err );
   /* -- if the set rate matches, we are done -- */
   if( srate == best )
      return paNoError;

   /* -- otherwise, something wierd happened: we didn't set the rate, and we got no errors. Just bail. */
   return paInternalError;
}


/*
   Attempts to set the requestedFramesPerBuffer. If it can't set the exact
   value, it settles for something smaller if available. If nothing smaller
   is available, it uses the smallest available size.
   actualFramesPerBuffer will be set to the actual value on successful return.
   OK to pass NULL to actualFramesPerBuffer.
   The logic is very simmilar too setBestSampleRate only failure here is
   not usually catastrophic.
*/
PaError setBestFramesPerBuffer( const AudioDeviceID device,
                                const bool isOutput,
                                UInt32 requestedFramesPerBuffer, 
                                UInt32 *actualFramesPerBuffer )
{
   UInt32 afpb;
   const bool isInput = !isOutput;
   UInt32 propsize = sizeof(UInt32);
   OSErr err;
   Float64 min  = -1; /*the min blocksize*/
   Float64 best = -1; /*the best blocksize*/
   int i=0;
   AudioValueRange *ranges;

   if( actualFramesPerBuffer == NULL )
      actualFramesPerBuffer = &afpb;


   /* -- try and set exact FPB -- */
   err = AudioDeviceSetProperty( device, NULL, 0, isInput,
                                 kAudioDevicePropertyBufferFrameSize,
                                 propsize, &requestedFramesPerBuffer);
   err = AudioDeviceGetProperty( device, 0, isInput,
                           kAudioDevicePropertyBufferFrameSize,
                           &propsize, actualFramesPerBuffer);
   if( err )
      return ERR( err );
   if( *actualFramesPerBuffer == requestedFramesPerBuffer )
      return paNoError; /* we are done */

   /* -- fetch available block sizes -- */
   err = AudioDeviceGetPropertyInfo( device, 0, isInput,
                           kAudioDevicePropertyBufferSizeRange,
                           &propsize, NULL );
   if( err )
      return ERR( err );
   ranges = (AudioValueRange *)calloc( 1, propsize );
   if( !ranges )
      return paInsufficientMemory;
   err = AudioDeviceGetProperty( device, 0, isInput,
                                kAudioDevicePropertyBufferSizeRange,
                                &propsize, ranges );
   if( err )
   {
      free( ranges );
      return ERR( err );
   }
   VDBUG(("Requested block size of %lu was not available.\n",
          requestedFramesPerBuffer ));
   VDBUG(("%lu Available block sizes are:\n",propsize/sizeof(AudioValueRange)));
#ifdef MAC_CORE_VERBOSE_DEBUG
   for( i=0; i<propsize/sizeof(AudioValueRange); ++i )
      VDBUG( ("\t%g-%g\n",
              (float) ranges[i].mMinimum,
              (float) ranges[i].mMaximum ) );
#endif
   VDBUG(("-----\n"));
   
   /* --- now pick the best available framesPerBuffer -- */
   for( i=0; i<propsize/sizeof(AudioValueRange); ++i )
   {
      if( min == -1 || ranges[i].mMinimum < min ) min = ranges[i].mMinimum;
      if( ranges[i].mMaximum < requestedFramesPerBuffer ) {
         if( best < 0 )
            best = ranges[i].mMaximum;
         else if( ranges[i].mMaximum > best )
            best = ranges[i].mMaximum;
      }
   }
   if( best == -1 )
      best = min;
   VDBUG( ("Minimum FPB  %g. best is %g.\n", min, best ) );
   free( ranges );

   /* --- set the buffer size (ignore errors) -- */
   requestedFramesPerBuffer = (UInt32) best ;
   propsize = sizeof( UInt32 );
   err = AudioDeviceSetProperty( device, NULL, 0, isInput,
                                 kAudioDevicePropertyBufferSize,
                                 propsize, &requestedFramesPerBuffer );
   /* --- read the property to check that it was set -- */
   err = AudioDeviceGetProperty( device, 0, isInput,
                                 kAudioDevicePropertyBufferSize,
                                 &propsize, actualFramesPerBuffer );

   if( err )
      return ERR( err );

   return paNoError;
}
