/*
    File:       SimplePing.m

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

#import "SimplePing.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>

#pragma mark * ICMP On-The-Wire Format

static uint16_t in_cksum(const void *buffer, size_t bufferLen)
    // This is the standard BSD checksum code, modified to use modern types.
{
	size_t           bytesLeft;
  int32_t          sum;
	const uint16_t  *cursor;
	union {
		uint16_t        us;
		uint8_t         uc[2];
	} last;
	uint16_t          answer;

	bytesLeft = bufferLen;
	sum = 0;
	cursor = (const uint16_t*)buffer;

	/*
	 * Our algorithm is simple, using a 32 bit accumulator (sum), we add
	 * sequential 16 bit words to it, and at the end, fold back all the
	 * carry bits from the top 16 bits into the lower 16 bits.
	 */
	while (bytesLeft > 1) {
		sum += *cursor;
        cursor += 1;
		bytesLeft -= 2;
	}

	/* mop up an odd byte, if necessary */
	if (bytesLeft == 1) {
		last.uc[0] = * (const uint8_t *) cursor;
		last.uc[1] = 0;
		sum += last.us;
	}

	/* add back carry outs from top 16 bits to low 16 bits */
	sum = (sum >> 16) + (sum & 0xffff);	/* add hi 16 to low 16 */
	sum += (sum >> 16);			/* add carry */
	answer = (uint16_t) ~sum;   /* truncate to 16 bits */

	return answer;
}

#pragma mark * SimplePing

@interface SimplePing ()

@property (nonatomic, copy,   readwrite) NSData *           hostAddress;
@property (nonatomic, assign, readwrite) uint16_t           nextSequenceNumber;

- (void)stopHostResolution;
- (void)stopDataTransfer;

@end

@implementation SimplePing
{
    CFHostRef               _host;
    CFSocketRef             _socket;
}

@synthesize hostName           = _hostName;
@synthesize hostAddress        = _hostAddress;

@synthesize delegate           = _delegate;
@synthesize identifier         = _identifier;
@synthesize nextSequenceNumber = _nextSequenceNumber;

- (id)initWithHostName:(NSString *)hostName address:(NSData *)hostAddress
    // The initialiser common to both of our construction class methods.
{
    assert( (hostName != nil) == (hostAddress == nil) );
    self = [super init];
    if (self != nil) {
        self->_hostName    = [hostName copy];
        self->_hostAddress = [hostAddress copy];
        self->_identifier  = (uint16_t) arc4random();
    }
    return self;
}

- (void)dealloc
{
    // -stop takes care of _host and _socket.
    [self stop];
    assert(self->_host == NULL);
    assert(self->_socket == NULL);
}

+ (SimplePing *)simplePingWithHostName:(NSString *)hostName
    // See comment in header.
{
    return [[SimplePing alloc] initWithHostName:hostName address:nil];
}

+ (SimplePing *)simplePingWithHostAddress:(NSData *)hostAddress
    // See comment in header.
{
    return [[SimplePing alloc] initWithHostName:NULL address:hostAddress];
}

- (void)noop
{
}

- (void)didFailWithError:(NSError *)error
    // Shut down the pinger object and tell the delegate about the error.
{
    assert(error != nil);
    
    // We retain ourselves temporarily because it's common for the delegate method 
    // to release its last reference to use, which causes -dealloc to be called here. 
    // If we then reference self on the return path, things go badly.  I don't think 
    // that happens currently, but I've got into the habit of doing this as a 
    // defensive measure.
    
    [self performSelector:@selector(noop) withObject:nil afterDelay:0.0];
    
    [self stop];
    if ( (self.delegate != nil) && [self.delegate respondsToSelector:@selector(simplePing:didFailWithError:)] ) {
        [self.delegate simplePing:self didFailWithError:error];
    }
}

- (void)didFailWithHostStreamError:(CFStreamError)streamError
    // Convert the CFStreamError to an NSError and then call through to 
    // -didFailWithError: to shut down the pinger object and tell the 
    // delegate about the error.
{
    NSDictionary *  userInfo;
    NSError *       error;

    if (streamError.domain == kCFStreamErrorDomainNetDB) {
        userInfo = [NSDictionary dictionaryWithObjectsAndKeys:
            [NSNumber numberWithInteger:streamError.error], kCFGetAddrInfoFailureKey,
            nil
        ];
    } else {
        userInfo = nil;
    }
    error = [NSError errorWithDomain:(NSString *)kCFErrorDomainCFNetwork code:kCFHostErrorUnknown userInfo:userInfo];
    assert(error != nil);

    [self didFailWithError:error];
}

- (void)sendPingWithData:(NSData *)data
    // See comment in header.
{
    int             err;
    NSData *        payload;
    NSMutableData * packet;
    ICMPHeader *    icmpPtr;
    ssize_t         bytesSent;
    
    // Construct the ping packet.
    payload = data;
    if (payload == nil) {
        payload = [[NSString stringWithFormat:@"%28zd bottles of beer on the wall", (ssize_t) 99 - (size_t) (self.nextSequenceNumber % 100) ] dataUsingEncoding:NSASCIIStringEncoding];
        assert(payload != nil);
        
        assert([payload length] == 56);
    }
    
    packet = [NSMutableData dataWithLength:sizeof(*icmpPtr) + [payload length]];
    assert(packet != nil);

    icmpPtr = (ICMPHeader*)[packet mutableBytes];
    icmpPtr->type = kICMPTypeEchoRequest;
    icmpPtr->code = 0;
    icmpPtr->checksum = 0;
    icmpPtr->identifier     = OSSwapHostToBigInt16(self.identifier);
    icmpPtr->sequenceNumber = OSSwapHostToBigInt16(self.nextSequenceNumber);
    memcpy(&icmpPtr[1], [payload bytes], [payload length]);
    
    // The IP checksum returns a 16-bit number that's already in correct byte order 
    // (due to wacky 1's complement maths), so we just put it into the packet as a 
    // 16-bit unit.
    
    icmpPtr->checksum = in_cksum([packet bytes], [packet length]);
    
    // Send the packet.
    
    if (self->_socket == NULL) {
        bytesSent = -1;
        err = EBADF;
    } else {
        bytesSent = sendto(
            CFSocketGetNative(self->_socket),
            [packet bytes],
            [packet length], 
            0, 
            (struct sockaddr *) [self.hostAddress bytes], 
            (socklen_t) [self.hostAddress length]
        );
        err = 0;
        if (bytesSent < 0) {
            err = errno;
        }
    }

    // Handle the results of the send.
    
    if ( (bytesSent > 0) && (((NSUInteger) bytesSent) == [packet length]) ) {

        // Complete success.  Tell the client.

        if ( (self.delegate != nil) && [self.delegate respondsToSelector:@selector(simplePing:didSendPacket:)] ) {
            [self.delegate simplePing:self didSendPacket:packet];
        }
    } else {
        NSError *   error;
        
        // Some sort of failure.  Tell the client.
        
        if (err == 0) {
            err = ENOBUFS;          // This is not a hugely descriptor error, alas.
        }
        error = [NSError errorWithDomain:NSPOSIXErrorDomain code:err userInfo:nil];
        if ( (self.delegate != nil) && [self.delegate respondsToSelector:@selector(simplePing:didFailToSendPacket:error:)] ) {
            [self.delegate simplePing:self didFailToSendPacket:packet error:error];
        }
    }
    
    self.nextSequenceNumber += 1;
}

+ (NSUInteger)icmpHeaderOffsetInPacket:(NSData *)packet
    // Returns the offset of the ICMPHeader within an IP packet.
{
    NSUInteger              result;
    const struct IPHeader * ipPtr;
    size_t                  ipHeaderLength;
    
    result = NSNotFound;
    if ([packet length] >= (sizeof(IPHeader) + sizeof(ICMPHeader))) {
        ipPtr = (const IPHeader *) [packet bytes];
        assert((ipPtr->versionAndHeaderLength & 0xF0) == 0x40);     // IPv4
        assert(ipPtr->protocol == 1);                               // ICMP
        ipHeaderLength = (ipPtr->versionAndHeaderLength & 0x0F) * sizeof(uint32_t);
        if ([packet length] >= (ipHeaderLength + sizeof(ICMPHeader))) {
            result = ipHeaderLength;
        }
    }
    return result;
}

+ (const struct ICMPHeader *)icmpInPacket:(NSData *)packet
    // See comment in header.
{
    const struct ICMPHeader *   result;
    NSUInteger                  icmpHeaderOffset;
    
    result = nil;
    icmpHeaderOffset = [self icmpHeaderOffsetInPacket:packet];
    if (icmpHeaderOffset != NSNotFound) {
        result = (const struct ICMPHeader *) (((const uint8_t *)[packet bytes]) + icmpHeaderOffset);
    }
    return result;
}

- (BOOL)isValidPingResponsePacket:(NSMutableData *)packet
    // Returns true if the packet looks like a valid ping response packet destined 
    // for us.
{
    BOOL                result;
    NSUInteger          icmpHeaderOffset;
    ICMPHeader *        icmpPtr;
    uint16_t            receivedChecksum;
    uint16_t            calculatedChecksum;
    
    result = NO;
    
    icmpHeaderOffset = [[self class] icmpHeaderOffsetInPacket:packet];
    if (icmpHeaderOffset != NSNotFound) {
        icmpPtr = (struct ICMPHeader *) (((uint8_t *)[packet mutableBytes]) + icmpHeaderOffset);

        receivedChecksum   = icmpPtr->checksum;
        icmpPtr->checksum  = 0;
        calculatedChecksum = in_cksum(icmpPtr, [packet length] - icmpHeaderOffset);
        icmpPtr->checksum  = receivedChecksum;
        
        if (receivedChecksum == calculatedChecksum) {
            if ( (icmpPtr->type == kICMPTypeEchoReply) && (icmpPtr->code == 0) ) {
                if ( OSSwapBigToHostInt16(icmpPtr->identifier) == self.identifier ) {
                    if ( OSSwapBigToHostInt16(icmpPtr->sequenceNumber) < self.nextSequenceNumber ) {
                        result = YES;
                    }
                }
            }
        }
    }

    return result;
}

- (void)readData
    // Called by the socket handling code (SocketReadCallback) to process an ICMP 
    // messages waiting on the socket.
{
    int                     err;
    struct sockaddr_storage addr;
    socklen_t               addrLen;
    ssize_t                 bytesRead;
    void *                  buffer;
    enum { kBufferSize = 65535 };

    // 65535 is the maximum IP packet size, which seems like a reasonable bound 
    // here (plus it's what <x-man-page://8/ping> uses).
    
    buffer = malloc(kBufferSize);
    assert(buffer != NULL);
    
    // Actually read the data.
    
    addrLen = sizeof(addr);
    bytesRead = recvfrom(CFSocketGetNative(self->_socket), buffer, kBufferSize, 0, (struct sockaddr *) &addr, &addrLen);
    err = 0;
    if (bytesRead < 0) {
        err = errno;
    }
    
    // Process the data we read.
    
    if (bytesRead > 0) {
        NSMutableData *     packet;

        packet = [NSMutableData dataWithBytes:buffer length:(NSUInteger) bytesRead];
        assert(packet != nil);

        // We got some data, pass it up to our client.

        if ( [self isValidPingResponsePacket:packet] ) {
            if ( (self.delegate != nil) && [self.delegate respondsToSelector:@selector(simplePing:didReceivePingResponsePacket:)] ) {
                [self.delegate simplePing:self didReceivePingResponsePacket:packet];
            }
        } else {
            if ( (self.delegate != nil) && [self.delegate respondsToSelector:@selector(simplePing:didReceiveUnexpectedPacket:)] ) {
                [self.delegate simplePing:self didReceiveUnexpectedPacket:packet];
            }
        }
    } else {
    
        // We failed to read the data, so shut everything down.
        
        if (err == 0) {
            err = EPIPE;
        }
        [self didFailWithError:[NSError errorWithDomain:NSPOSIXErrorDomain code:err userInfo:nil]];
    }
    
    free(buffer);
    
    // Note that we don't loop back trying to read more data.  Rather, we just 
    // let CFSocket call us again.
}

static void SocketReadCallback(CFSocketRef s, CFSocketCallBackType type, CFDataRef address, const void *data, void *info)
    // This C routine is called by CFSocket when there's data waiting on our 
    // ICMP socket.  It just redirects the call to Objective-C code.
{
    SimplePing *    obj;
    
    obj = (__bridge SimplePing *) info;
    assert([obj isKindOfClass:[SimplePing class]]);
    
    #pragma unused(s)
    assert(s == obj->_socket);
    #pragma unused(type)
    assert(type == kCFSocketReadCallBack);
    #pragma unused(address)
    assert(address == nil);
    #pragma unused(data)
    assert(data == nil);
    
    [obj readData];
}

- (void)startWithHostAddress
    // We have a host address, so let's actually start pinging it.
{
    int                     err;
    int                     fd;
    const struct sockaddr * addrPtr;

    assert(self.hostAddress != nil);

    // Open the socket.
    
    addrPtr = (const struct sockaddr *) [self.hostAddress bytes];

    fd = -1;
    err = 0;
    switch (addrPtr->sa_family) {
        case AF_INET: {
            fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_ICMP);
            if (fd < 0) {
                err = errno;
            }
        } break;
        case AF_INET6:
            assert(NO);
            // fall through
        default: {
            err = EPROTONOSUPPORT;
        } break;
    }
    
    if (err != 0) {
        [self didFailWithError:[NSError errorWithDomain:NSPOSIXErrorDomain code:err userInfo:nil]];
    } else {
        CFSocketContext     context = {0, (__bridge void *)(self), NULL, NULL, NULL};
        CFRunLoopSourceRef  rls;
        
        // Wrap it in a CFSocket and schedule it on the runloop.
        
        self->_socket = CFSocketCreateWithNative(NULL, fd, kCFSocketReadCallBack, SocketReadCallback, &context);
        assert(self->_socket != NULL);
        
        // The socket will now take care of cleaning up our file descriptor.
        
        assert( CFSocketGetSocketFlags(self->_socket) & kCFSocketCloseOnInvalidate );
        fd = -1;
        
        rls = CFSocketCreateRunLoopSource(NULL, self->_socket, 0);
        assert(rls != NULL);
        
        CFRunLoopAddSource(CFRunLoopGetCurrent(), rls, kCFRunLoopDefaultMode);
        
        CFRelease(rls);

        if ( (self.delegate != nil) && [self.delegate respondsToSelector:@selector(simplePing:didStartWithAddress:)] ) {
            [self.delegate simplePing:self didStartWithAddress:self.hostAddress];
        }
    }
    assert(fd == -1);
}

- (void)hostResolutionDone
    // Called by our CFHost resolution callback (HostResolveCallback) when host 
    // resolution is complete.  We just latch the first IPv4 address and kick 
    // off the pinging process.
{
    Boolean     resolved;
    NSArray *   addresses;
    
    // Find the first IPv4 address.
    
    addresses = (__bridge NSArray *) CFHostGetAddressing(self->_host, &resolved);
    if ( resolved && (addresses != nil) ) {
        resolved = false;
        for (NSData * address in addresses) {
            const struct sockaddr * addrPtr;
            
            addrPtr = (const struct sockaddr *) [address bytes];
            if ( [address length] >= sizeof(struct sockaddr) && addrPtr->sa_family == AF_INET) {
                self.hostAddress = address;
                resolved = true;
                break;
            }
        }
    }

    // We're done resolving, so shut that down.
    
    [self stopHostResolution];
    
    // If all is OK, start pinging, otherwise shut down the pinger completely.
    
    if (resolved) {
        [self startWithHostAddress];
    } else {
        [self didFailWithError:[NSError errorWithDomain:(NSString *)kCFErrorDomainCFNetwork code:kCFHostErrorHostNotFound userInfo:nil]];
    }
}

static void HostResolveCallback(CFHostRef theHost, CFHostInfoType typeInfo, const CFStreamError *error, void *info)
    // This C routine is called by CFHost when the host resolution is complete. 
    // It just redirects the call to the appropriate Objective-C method.
{
    SimplePing *    obj;

    //NSLog(@">HostResolveCallback");
    
    obj = (__bridge SimplePing *) info;
    assert([obj isKindOfClass:[SimplePing class]]);
    
    #pragma unused(theHost)
    assert(theHost == obj->_host);
    #pragma unused(typeInfo)
    assert(typeInfo == kCFHostAddresses);
    
    if ( (error != NULL) && (error->domain != 0) ) {
        [obj didFailWithHostStreamError:*error];
    } else {
        [obj hostResolutionDone];
    }
}

- (void)start
    // See comment in header.
{
    // If the user supplied us with an address, just start pinging that.  Otherwise 
    // start a host resolution.
    
    if (self->_hostAddress != nil) {
        [self startWithHostAddress];
    } else {
        Boolean             success;
        CFHostClientContext context = {0, (__bridge void *)(self), NULL, NULL, NULL};
        CFStreamError       streamError;

        assert(self->_host == NULL);

        self->_host = CFHostCreateWithName(NULL, (__bridge CFStringRef) self.hostName);
        assert(self->_host != NULL);
        
        CFHostSetClient(self->_host, HostResolveCallback, &context);
        
        CFHostScheduleWithRunLoop(self->_host, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
        
        //NSLog(@">CFHostStartInfoResolution");
        success = CFHostStartInfoResolution(self->_host, kCFHostAddresses, &streamError);
        //NSLog(@"<CFHostStartInfoResolution");
        if ( ! success ) {
            [self didFailWithHostStreamError:streamError];
        }
    }
}

- (void)stopHostResolution
    // Shut down the CFHost.
{
    if (self->_host != NULL) {
        CFHostSetClient(self->_host, NULL, NULL);
        CFHostUnscheduleFromRunLoop(self->_host, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
        CFRelease(self->_host);
        self->_host = NULL;
    }
}

- (void)stopDataTransfer   
    // Shut down anything to do with sending and receiving pings.
{
    if (self->_socket != NULL) {
        CFSocketInvalidate(self->_socket);
        CFRelease(self->_socket);
        self->_socket = NULL;
    }
}

- (void)stop
    // See comment in header.
{
    [self stopHostResolution];
    [self stopDataTransfer];
    
    // If we were started with a host name, junk the host address on stop.  If the 
    // client calls -start again, we'll re-resolve the host name.
    
    if (self.hostName != nil) {
        self.hostAddress = NULL;
    }
}

@end
