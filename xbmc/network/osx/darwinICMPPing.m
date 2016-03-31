/*
 *      Copyright (C) 2015 Team MrMC
 *      https://github.com/MrMC
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with MrMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <sysexits.h>
#include <arpa/inet.h>

#import "SimplePing.h"
#import "darwinICMPPing.h"

NSString * const kdidStartWithAddress         = @"didStartWithAddress";
NSString * const kdidFailWithError            = @"didFailWithError";
NSString * const kdidFailToSendPacket         = @"didFailToSendPacket";
NSString * const kdidReceiveUnexpectedPacket  = @"didReceiveUnexpectedPacket";

@interface SimplePingClient : NSObject<SimplePingDelegate>
+(void)pingHostname:(NSString*)hostName andResultCallback:(void(^)(NSString* result))result;
+(void)pingHostAddress:(NSData*)hostAddress andResultCallback:(void(^)(NSString* result))result;
@end

int darwinICMPPingHost(const char *host, unsigned int timeout_ms)
{
  // also checks if host is a name or ip address
  struct sockaddr_in callAddress = {0};
  callAddress.sin_len = sizeof(callAddress);
  callAddress.sin_family = AF_INET;
  callAddress.sin_addr.s_addr = inet_addr(host);

  __block NSString *results = kdidStartWithAddress;
  if (callAddress.sin_addr.s_addr != INADDR_NONE)
  {
    [SimplePingClient pingHostAddress:[NSData dataWithBytes:&callAddress length:sizeof(callAddress)]
             andResultCallback:^(NSString *callbackresults) {
      results = callbackresults;
    }];
  }
  else
  {
    [SimplePingClient pingHostname:[NSString stringWithCString:host encoding:NSUTF8StringEncoding]
             andResultCallback:^(NSString *callbackresults) {
      results = callbackresults;
    }];
  }
  
  float timout_secs = (float)timeout_ms / 1000.0;
  [[NSRunLoop currentRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow:timout_secs]];

  if ([results isEqualToString:kdidStartWithAddress])
  {
    // host address did not reply, it is not present
    return -1;
  }
  else if ([results isEqualToString:kdidFailWithError])
  {
    return -1;
  }
  else if ([results isEqualToString:kdidFailToSendPacket])
  {
    return -1;
  }
  else if ([results isEqualToString:kdidReceiveUnexpectedPacket])
  {
    return -1;
  }
/*
  else
  {
    NSLog(@"the latency is: %@", results);
  }
*/
  return 0;
}

int darwinICMPPing(in_addr_t remote_ip, unsigned int timeout_ms)
{
  // also checks if host is a name or ip address
  struct sockaddr_in callAddress = {0};
  callAddress.sin_len = sizeof(callAddress);
  callAddress.sin_family = AF_INET;
  callAddress.sin_addr.s_addr = remote_ip;

  __block NSString *results = kdidStartWithAddress;
  [SimplePingClient pingHostAddress:[NSData dataWithBytes:&callAddress length:sizeof(callAddress)]
           andResultCallback:^(NSString *callbackresults) {
    results = callbackresults;
  }];
  
  float timout_secs = (float)timeout_ms / 1000.0;
  [[NSRunLoop currentRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow:timout_secs]];

  if ([results isEqualToString:kdidStartWithAddress])
  {
    // host address did not reply, it is not present
    return -1;
  }
  else if ([results isEqualToString:kdidFailWithError])
  {
    return -1;
  }
  else if ([results isEqualToString:kdidFailToSendPacket])
  {
    return -1;
  }
  else if ([results isEqualToString:kdidReceiveUnexpectedPacket])
  {
    return -1;
  }
/*
  else
  {
    NSLog(@"the latency is: %@", results);
  }
*/
  return 0;
}


//********************************************************************************************
@interface SimplePingClient()
{
  SimplePing* _pingClient;
  NSDate* _dateReference;
}
@property(nonatomic, strong) void(^resultCallback)(NSString* result);
@end

@implementation SimplePingClient

+(void)pingHostname:(NSString*)hostName andResultCallback:(void(^)(NSString* result))result
{
  static SimplePingClient* singletonPC = nil;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
      singletonPC = [[SimplePingClient alloc] init];
  });

  //ping hostname
  [singletonPC pingHostname:hostName andResultCallBlock:result];
}

+(void)pingHostAddress:(NSData*)hostAddress andResultCallback:(void(^)(NSString* result))result
{
  static SimplePingClient* singletonPC = nil;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
      singletonPC = [[SimplePingClient alloc] init];
  });

  //ping hostaddress
  [singletonPC pingHostAddress:hostAddress andResultCallBlock:result];
}

-(void)pingHostname:(NSString*)hostName andResultCallBlock:(void(^)(NSString* result))result
{
  _resultCallback = result;
  _pingClient = [SimplePing simplePingWithHostName:hostName];
  _pingClient.delegate = self;
  [_pingClient start];
}

-(void)pingHostAddress:(NSData*)hostAddress andResultCallBlock:(void(^)(NSString* result))result
{
  _resultCallback = result;
  _pingClient = [SimplePing simplePingWithHostAddress:hostAddress];
  _pingClient.delegate = self;
  [_pingClient start];
}

#pragma mark - SimplePingDelegate methods
- (void)simplePing:(SimplePing *)pinger didStartWithAddress:(NSData *)address
{
  [pinger sendPingWithData:nil];
}

- (void)simplePing:(SimplePing *)pinger didFailWithError:(NSError *)error
{
  _resultCallback(kdidFailWithError);
}

- (void)simplePing:(SimplePing *)pinger didSendPacket:(NSData *)packet
{
  _dateReference = [NSDate date];
}

- (void)simplePing:(SimplePing *)pinger didFailToSendPacket:(NSData *)packet error:(NSError *)error
{
  [pinger stop];
  _resultCallback(kdidFailToSendPacket);
}

- (void)simplePing:(SimplePing *)pinger didReceivePingResponsePacket:(NSData *)packet
{
  [pinger stop];
  NSDate *end = [NSDate date];
  double result_ms = [end timeIntervalSinceDate:_dateReference] * 1000;
  _resultCallback([NSString stringWithFormat:@"%.f", result_ms]);
}

- (void)simplePing:(SimplePing *)pinger didReceiveUnexpectedPacket:(NSData *)packet
{
  [pinger stop];
  _resultCallback(kdidReceiveUnexpectedPacket);
}

@end
