/*
 *  AppleRemoteKeys.h
 *  XBMC
 *
 *  Created by Elan Feingold on 2/27/2008.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */
#ifndef _APPLE_REMOTE_KEYS_H_
#define _APPLE_REMOTE_KEYS_H_

#ifdef __cplusplus
extern "C"
{
#endif

enum AppleRemoteEventIdentifier
{
    kRemoteButtonVolume_Plus        =1<<1,
    kRemoteButtonVolume_Minus       =1<<2,
    kRemoteButtonMenu               =1<<3,
    kRemoteButtonPlay               =1<<4,
    kRemoteButtonRight              =1<<5,
    kRemoteButtonLeft               =1<<6,
    kRemoteButtonRight_Hold         =1<<7,
    kRemoteButtonLeft_Hold          =1<<8,
    kRemoteButtonMenu_Hold          =1<<9,
    kRemoteButtonPlay_Sleep         =1<<10,
    kRemoteControl_Switched         =1<<11,
    kRemoteButtonVolume_Plus_Hold   =1<<12,
    kRemoteButtonVolume_Minus_Hold  =1<<13
};

typedef enum AppleRemoteEventIdentifier AppleRemoteEventIdentifier;
typedef void (*AppleRemoteCallback)(AppleRemoteEventIdentifier event, bool pressedDown, unsigned int count);

#ifdef __cplusplus
}
#endif

#endif