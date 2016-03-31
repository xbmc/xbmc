/*
    File:       SimplePing.h

    Contains:   Implements ping.

    Written by: DTS

    Copyright:  Copyright (c) 2010-2012 Apple Inc. All Rights Reserved.

    Disclaimer: IMPORTANT: This Apple software is supplied to you by Apple Inc.
                ("Apple") in consideration of your agreement to the following
                terms, and your use, installation, modification or
                redistribution of this Apple software constitutes acceptance of
                these terms.  If you do not agree with these terms, please do
                not use, install, modify or redistribute this Apple software.

                In consideration of your agreement to abide by the following
                terms, and subject to these terms, Apple grants you a personal,
                non-exclusive license, under Apple's copyrights in this
                original Apple software (the "Apple Software"), to use,
                reproduce, modify and redistribute the Apple Software, with or
                without modifications, in source and/or binary forms; provided
                that if you redistribute the Apple Software in its entirety and
                without modifications, you must retain this notice and the
                following text and disclaimers in all such redistributions of
                the Apple Software. Neither the name, trademarks, service marks
                or logos of Apple Inc. may be used to endorse or promote
                products derived from the Apple Software without specific prior
                written permission from Apple.  Except as expressly stated in
                this notice, no other rights or licenses, express or implied,
                are granted by Apple herein, including but not limited to any
                patent rights that may be infringed by your derivative works or
                by other works in which the Apple Software may be incorporated.

                The Apple Software is provided by Apple on an "AS IS" basis. 
                APPLE MAKES NO WARRANTIES, EXPRESS OR IMPLIED, INCLUDING
                WITHOUT LIMITATION THE IMPLIED WARRANTIES OF NON-INFRINGEMENT,
                MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE, REGARDING
                THE APPLE SOFTWARE OR ITS USE AND OPERATION ALONE OR IN
                COMBINATION WITH YOUR PRODUCTS.

                IN NO EVENT SHALL APPLE BE LIABLE FOR ANY SPECIAL, INDIRECT,
                INCIDENTAL OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
                TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
                DATA, OR PROFITS; OR BUSINESS INTERRUPTION) ARISING IN ANY WAY
                OUT OF THE USE, REPRODUCTION, MODIFICATION AND/OR DISTRIBUTION
                OF THE APPLE SOFTWARE, HOWEVER CAUSED AND WHETHER UNDER THEORY
                OF CONTRACT, TORT (INCLUDING NEGLIGENCE), STRICT LIABILITY OR
                OTHERWISE, EVEN IF APPLE HAS BEEN ADVISED OF THE POSSIBILITY OF
                SUCH DAMAGE.

*/

#import <Foundation/Foundation.h>

#if TARGET_OS_EMBEDDED || TARGET_IPHONE_SIMULATOR
    #import <CFNetwork/CFNetwork.h>
#else
    #import <CoreServices/CoreServices.h>
#endif

#include <AssertMacros.h>

#pragma mark * SimplePing

// The SimplePing class is a very simple class that lets you send and receive pings.

@protocol SimplePingDelegate;

@interface SimplePing : NSObject

+ (SimplePing *)simplePingWithHostName:(NSString *)hostName;        // chooses first IPv4 address
+ (SimplePing *)simplePingWithHostAddress:(NSData *)hostAddress;    // contains (struct sockaddr)

@property (nonatomic, assign, readwrite) id<SimplePingDelegate> delegate;

@property (nonatomic, copy,   readonly ) NSString *             hostName;
@property (nonatomic, copy,   readonly ) NSData *               hostAddress;
@property (nonatomic, assign, readonly ) uint16_t               identifier;
@property (nonatomic, assign, readonly ) uint16_t               nextSequenceNumber;

- (void)start;
    // Starts the pinger object pinging.  You should call this after 
    // you've setup the delegate and any ping parameters.

- (void)sendPingWithData:(NSData *)data;
    // Sends an actual ping.  Pass nil for data to use a standard 56 byte payload (resulting in a 
    // standard 64 byte ping).  Otherwise pass a non-nil value and it will be appended to the 
    // ICMP header.
    //
    // Do not try to send a ping before you receive the -simplePing:didStartWithAddress: delegate 
    // callback.

- (void)stop;
    // Stops the pinger object.  You should call this when you're done 
    // pinging.

+ (const struct ICMPHeader *)icmpInPacket:(NSData *)packet;
    // Given a valid IP packet contains an ICMP , returns the address of the ICMP header that 
    // follows the IP header.  This doesn't do any significant validation of the packet.

@end

@protocol SimplePingDelegate <NSObject>

@optional

- (void)simplePing:(SimplePing *)pinger didStartWithAddress:(NSData *)address;
    // Called after the SimplePing has successfully started up.  After this callback, you 
    // can start sending pings via -sendPingWithData:
    
- (void)simplePing:(SimplePing *)pinger didFailWithError:(NSError *)error;
    // If this is called, the SimplePing object has failed.  By the time this callback is 
    // called, the object has stopped (that is, you don't need to call -stop yourself).

// IMPORTANT: On the send side the packet does not include an IP header. 
// On the receive side, it does.  In that case, use +[SimplePing icmpInPacket:] 
// to find the ICMP header within the packet.

- (void)simplePing:(SimplePing *)pinger didSendPacket:(NSData *)packet;
    // Called whenever the SimplePing object has successfully sent a ping packet. 
    
- (void)simplePing:(SimplePing *)pinger didFailToSendPacket:(NSData *)packet error:(NSError *)error;
    // Called whenever the SimplePing object tries and fails to send a ping packet.

- (void)simplePing:(SimplePing *)pinger didReceivePingResponsePacket:(NSData *)packet;
    // Called whenever the SimplePing object receives an ICMP packet that looks like 
    // a response to one of our pings (that is, has a valid ICMP checksum, has 
    // an identifier that matches our identifier, and has a sequence number in 
    // the range of sequence numbers that we've sent out).

- (void)simplePing:(SimplePing *)pinger didReceiveUnexpectedPacket:(NSData *)packet;
    // Called whenever the SimplePing object receives an ICMP packet that does not 
    // look like a response to one of our pings.

@end

#pragma mark * IP and ICMP On-The-Wire Format

// The following declarations specify the structure of ping packets on the wire.

// IP header structure:

struct IPHeader {
    uint8_t     versionAndHeaderLength;
    uint8_t     differentiatedServices;
    uint16_t    totalLength;
    uint16_t    identification;
    uint16_t    flagsAndFragmentOffset;
    uint8_t     timeToLive;
    uint8_t     protocol;
    uint16_t    headerChecksum;
    uint8_t     sourceAddress[4];
    uint8_t     destinationAddress[4];
    // options...
    // data...
};
typedef struct IPHeader IPHeader;

check_compile_time(sizeof(IPHeader) == 20);
check_compile_time(offsetof(IPHeader, versionAndHeaderLength) == 0);
check_compile_time(offsetof(IPHeader, differentiatedServices) == 1);
check_compile_time(offsetof(IPHeader, totalLength) == 2);
check_compile_time(offsetof(IPHeader, identification) == 4);
check_compile_time(offsetof(IPHeader, flagsAndFragmentOffset) == 6);
check_compile_time(offsetof(IPHeader, timeToLive) == 8);
check_compile_time(offsetof(IPHeader, protocol) == 9);
check_compile_time(offsetof(IPHeader, headerChecksum) == 10);
check_compile_time(offsetof(IPHeader, sourceAddress) == 12);
check_compile_time(offsetof(IPHeader, destinationAddress) == 16);

// ICMP type and code combinations:

enum {
    kICMPTypeEchoReply   = 0,           // code is always 0
    kICMPTypeEchoRequest = 8            // code is always 0
};

// ICMP header structure:

struct ICMPHeader {
    uint8_t     type;
    uint8_t     code;
    uint16_t    checksum;
    uint16_t    identifier;
    uint16_t    sequenceNumber;
    // data...
};
typedef struct ICMPHeader ICMPHeader;

check_compile_time(sizeof(ICMPHeader) == 8);
check_compile_time(offsetof(ICMPHeader, type) == 0);
check_compile_time(offsetof(ICMPHeader, code) == 1);
check_compile_time(offsetof(ICMPHeader, checksum) == 2);
check_compile_time(offsetof(ICMPHeader, identifier) == 4);
check_compile_time(offsetof(ICMPHeader, sequenceNumber) == 6);
