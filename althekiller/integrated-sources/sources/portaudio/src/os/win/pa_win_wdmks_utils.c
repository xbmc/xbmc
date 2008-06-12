/*
 * PortAudio Portable Real-Time Audio Library
 * Windows WDM KS utilities
 *
 * Copyright (c) 1999 - 2007 Andrew Baldwin, Ross Bencina
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

#include <windows.h>
#include <mmreg.h>
#include <ks.h>
#include <ksmedia.h>
#include <stdio.h>              // just for some development printfs

#include "portaudio.h"
#include "pa_util.h"
#include "pa_win_wdmks_utils.h"


static PaError WdmGetPinPropertySimple(
    HANDLE  handle,
    unsigned long pinId,
    unsigned long property,
    void* value,
    unsigned long valueSize )
{
    DWORD bytesReturned;
    KSP_PIN ksPProp;
    ksPProp.Property.Set = KSPROPSETID_Pin;
    ksPProp.Property.Id = property;
    ksPProp.Property.Flags = KSPROPERTY_TYPE_GET;
    ksPProp.PinId = pinId;
    ksPProp.Reserved = 0;

    if( DeviceIoControl( handle, IOCTL_KS_PROPERTY, &ksPProp, sizeof(KSP_PIN),
            value, valueSize, &bytesReturned, NULL ) == 0 || bytesReturned != valueSize )
    {
        return paUnanticipatedHostError;
    }
    else
    {
        return paNoError;
    }
}


static PaError WdmGetPinPropertyMulti(
    HANDLE handle,
    unsigned long pinId,
    unsigned long property,
    KSMULTIPLE_ITEM** ksMultipleItem)
{
    unsigned long multipleItemSize = 0;
    KSP_PIN ksPProp;
    DWORD bytesReturned;

    *ksMultipleItem = 0;

    ksPProp.Property.Set = KSPROPSETID_Pin;
    ksPProp.Property.Id = property;
    ksPProp.Property.Flags = KSPROPERTY_TYPE_GET;
    ksPProp.PinId = pinId;
    ksPProp.Reserved = 0;

    if( DeviceIoControl( handle, IOCTL_KS_PROPERTY, &ksPProp.Property,
            sizeof(KSP_PIN), NULL, 0, &multipleItemSize, NULL ) == 0 && GetLastError() != ERROR_MORE_DATA )
    {
        return paUnanticipatedHostError;
    }

    *ksMultipleItem = (KSMULTIPLE_ITEM*)PaUtil_AllocateMemory( multipleItemSize );
    if( !*ksMultipleItem )
    {
        return paInsufficientMemory;
    }

    if( DeviceIoControl( handle, IOCTL_KS_PROPERTY, &ksPProp, sizeof(KSP_PIN),
            (void*)*ksMultipleItem,  multipleItemSize, &bytesReturned, NULL ) == 0 || bytesReturned != multipleItemSize )
    {
        PaUtil_FreeMemory( ksMultipleItem );
        return paUnanticipatedHostError;
    }

    return paNoError;
}


static int GetKSFilterPinCount( HANDLE deviceHandle )
{
    DWORD result;

    if( WdmGetPinPropertySimple( deviceHandle, 0, KSPROPERTY_PIN_CTYPES, &result, sizeof(result) ) == paNoError ){
        return result;
    }else{
        return 0;
    }
}


static KSPIN_COMMUNICATION GetKSFilterPinPropertyCommunication( HANDLE deviceHandle, int pinId )
{
    KSPIN_COMMUNICATION result;

    if( WdmGetPinPropertySimple( deviceHandle, pinId, KSPROPERTY_PIN_COMMUNICATION, &result, sizeof(result) ) == paNoError ){
        return result;
    }else{
        return KSPIN_COMMUNICATION_NONE;
    }
}


static KSPIN_DATAFLOW GetKSFilterPinPropertyDataflow( HANDLE deviceHandle, int pinId )
{
    KSPIN_DATAFLOW result;

    if( WdmGetPinPropertySimple( deviceHandle, pinId, KSPROPERTY_PIN_DATAFLOW, &result, sizeof(result) ) == paNoError ){
        return result;
    }else{
        return (KSPIN_DATAFLOW)0;
    }
}


static int KSFilterPinPropertyIdentifiersInclude( 
        HANDLE deviceHandle, int pinId, unsigned long property, const GUID *identifierSet, unsigned long identifierId  )
{
    KSMULTIPLE_ITEM* item = NULL;
    KSIDENTIFIER* identifier;
    int i;
    int result = 0;

    if( WdmGetPinPropertyMulti( deviceHandle, pinId, property, &item) != paNoError )
        return 0;
    
    identifier = (KSIDENTIFIER*)(item+1);

    for( i = 0; i < (int)item->Count; i++ )
    {
        if( !memcmp( (void*)&identifier[i].Set, (void*)identifierSet, sizeof( GUID ) ) &&
           ( identifier[i].Id == identifierId ) )
        {
            result = 1;
            break;
        }
    }

    PaUtil_FreeMemory( item );

    return result;
}


/* return the maximum channel count supported by any pin on the device. 
   if isInput is non-zero we query input pins, otherwise output pins.
*/
int PaWin_WDMKS_QueryFilterMaximumChannelCount( void *wcharDevicePath, int isInput )
{
    HANDLE deviceHandle;
    int pinCount, pinId, i;
    int result = 0;
    KSPIN_DATAFLOW requiredDataflowDirection = (isInput ? KSPIN_DATAFLOW_OUT : KSPIN_DATAFLOW_IN );
    
    if( !wcharDevicePath )
        return 0;

    deviceHandle = CreateFileW( (LPCWSTR)wcharDevicePath, FILE_SHARE_READ|FILE_SHARE_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL );
    if( deviceHandle == INVALID_HANDLE_VALUE )
        return 0;

    pinCount = GetKSFilterPinCount( deviceHandle );
    for( pinId = 0; pinId < pinCount; ++pinId )
    {
        KSPIN_COMMUNICATION communication = GetKSFilterPinPropertyCommunication( deviceHandle, pinId );
        KSPIN_DATAFLOW dataflow = GetKSFilterPinPropertyDataflow( deviceHandle, pinId );
        if( ( dataflow == requiredDataflowDirection ) &&
                (( communication == KSPIN_COMMUNICATION_SINK) ||
                 ( communication == KSPIN_COMMUNICATION_BOTH)) 
             && ( KSFilterPinPropertyIdentifiersInclude( deviceHandle, pinId, 
                    KSPROPERTY_PIN_INTERFACES, &KSINTERFACESETID_Standard, KSINTERFACE_STANDARD_STREAMING )
                || KSFilterPinPropertyIdentifiersInclude( deviceHandle, pinId, 
                    KSPROPERTY_PIN_INTERFACES, &KSINTERFACESETID_Standard, KSINTERFACE_STANDARD_LOOPED_STREAMING ) )
             && KSFilterPinPropertyIdentifiersInclude( deviceHandle, pinId, 
                    KSPROPERTY_PIN_MEDIUMS, &KSMEDIUMSETID_Standard, KSMEDIUM_STANDARD_DEVIO ) )
         {
            KSMULTIPLE_ITEM* item = NULL;
            if( WdmGetPinPropertyMulti( deviceHandle, pinId, KSPROPERTY_PIN_DATARANGES, &item ) == paNoError )
            {
                KSDATARANGE *dataRange = (KSDATARANGE*)(item+1);

                for( i=0; i < item->Count; ++i ){

                    if( IS_VALID_WAVEFORMATEX_GUID(&dataRange->SubFormat)
                            || memcmp( (void*)&dataRange->SubFormat, (void*)&KSDATAFORMAT_SUBTYPE_PCM, sizeof(GUID) ) == 0
                            || memcmp( (void*)&dataRange->SubFormat, (void*)&KSDATAFORMAT_SUBTYPE_IEEE_FLOAT, sizeof(GUID) ) == 0
                            || ( ( memcmp( (void*)&dataRange->MajorFormat, (void*)&KSDATAFORMAT_TYPE_AUDIO, sizeof(GUID) ) == 0 )
                                && ( memcmp( (void*)&dataRange->SubFormat, (void*)&KSDATAFORMAT_SUBTYPE_WILDCARD, sizeof(GUID) ) == 0 ) ) )
                    {
                        KSDATARANGE_AUDIO *dataRangeAudio = (KSDATARANGE_AUDIO*)dataRange;
                        
                        /*
                        printf( ">>> %d %d %d %d %S\n", isInput, dataflow, communication, dataRangeAudio->MaximumChannels, devicePath );
                       
                        if( memcmp((void*)&dataRange->Specifier, (void*)&KSDATAFORMAT_SPECIFIER_WAVEFORMATEX, sizeof(GUID) ) == 0 )
                            printf( "\tspecifier: KSDATAFORMAT_SPECIFIER_WAVEFORMATEX\n" );
                        else if( memcmp((void*)&dataRange->Specifier, (void*)&KSDATAFORMAT_SPECIFIER_DSOUND, sizeof(GUID) ) == 0 )
                            printf( "\tspecifier: KSDATAFORMAT_SPECIFIER_DSOUND\n" );
                        else if( memcmp((void*)&dataRange->Specifier, (void*)&KSDATAFORMAT_SPECIFIER_WILDCARD, sizeof(GUID) ) == 0 )
                            printf( "\tspecifier: KSDATAFORMAT_SPECIFIER_WILDCARD\n" );
                        else
                            printf( "\tspecifier: ?\n" );
                        */

                        /*
                            We assume that very high values for MaximumChannels are not useful and indicate
                            that the driver isn't prepared to tell us the real number of channels which it supports.
                        */
                        if( dataRangeAudio->MaximumChannels  < 0xFFFFUL && (int)dataRangeAudio->MaximumChannels > result )
                            result = (int)dataRangeAudio->MaximumChannels;
                    }
                    
                    dataRange = (KSDATARANGE*)( ((char*)dataRange) + dataRange->FormatSize);
                }

                PaUtil_FreeMemory( item );
            }
        }
    }
    
    CloseHandle( deviceHandle );
    return result;
}
