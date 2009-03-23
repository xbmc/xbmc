/*
 *      Copyright (C) 2005-2009 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#import "ZeroconfOSX.h"

#import <string>

#import <Cocoa/Cocoa.h>
#import <netinet/in.h>
#import <sys/socket.h>


@interface ZeroconfOSXImpl : NSObject
{
  // Keeps track of active services or services about to be published
  // keys in the dictionary are the identifiers given when a service is published
  NSMutableDictionary* services;
}

- (bool) publishService:(NSString*) identifier withType:(NSString*) service_type withName:(NSString*) name withPort:(unsigned int) port;
- (bool) removeService:(NSString*) identifier;
- (bool) hasService:(NSString*) identifier;

//stops all services
- (void) stop;

// NSNetService delegate methods for publication
- (void)netServiceWillPublish:(NSNetService *)netService;
- (void)netService:(NSNetService *)netService
     didNotPublish:(NSDictionary *)errorDict;
- (void)netServiceDidStop:(NSNetService *)netService;

// Other methods
- (void)handleError:(NSNumber *)error withService:(NSNetService *)service;

@end


@implementation ZeroconfOSXImpl

-(id) init
{
  if( (self = [super init]) )
  {
    NSLog(@" ZeroconfOSX created");
  }
  services = [[NSMutableDictionary alloc] init];
  return self;
}

-(void) dealloc
{
  [self stop];
  [services release];
  [super dealloc];
}

- (bool) publishService:(NSString*) identifier withType:(NSString*) service_type withName:(NSString*) name withPort:(unsigned int) port
{
  NSLog(@" ZeroconfOSX::publishService %@ withType %@ withName %@ withPort %i", identifier, service_type, name, port);

  //check if we already have that identifier
  if([services objectForKey:identifier])
  {
    NSLog(@" ZeroconfOSX::publishService: Error, identifier already exists!");
    return false;
  }

  NSString *hostname = [[NSProcessInfo processInfo] hostName];
  NSNetService* service = [[[NSNetService alloc] initWithDomain:@""// 4
                                                           type:service_type
                                                           name:[NSString stringWithFormat:@"%@@%@", name, hostname]
                                                           port:port] autorelease];
  if(service)
  {
    [service setDelegate:self];
    [services setObject:service forKey:identifier];
    [service publish];
    return true;
  }
  else
  {
    NSLog(@"An error occurred initializing the NSNetService object.");
    return false;
  }
}

- (bool) removeService:(NSString*) identifier
{
  NSNetService* service = [services objectForKey:identifier];
  if(!service)
  {
    return false;
  }
  else
  {
    [service stop];
    return service;
  }
}

- (bool) hasService:(NSString*) identifier
{
  return ([services objectForKey:identifier] != nil);
}

-(void) stop
{
  NSLog(@" ZeroconfOSX::stop");
  NSEnumerator* enumerator = [services objectEnumerator];
  NSNetService* service;
  while ((service = [enumerator nextObject]))
  {
    [service stop];
  }
}


// Sent when the service is about to publish
- (void)netServiceWillPublish:(NSNetService *)netService
{
  // You may want to do something here, such as updating a user interface
}

// Sent if publication fails
- (void)netService:(NSNetService *)netService
     didNotPublish:(NSDictionary *)errorDict
{
  [self handleError:[errorDict objectForKey:NSNetServicesErrorCode]
        withService:netService];

  //one-liner below is a bit stupid. first it searches for all keys (in this case exactly one) for that object,
  //then it uses that key to delete it
  [services removeObjectsForKeys:[services allKeysForObject:netService]];
}

// Sent when the service stops
- (void)netServiceDidStop:(NSNetService *)netService
{
  //one-liner below is a bit stupid. first it searches for all keys (in this case exactly one) for that object,
  //then it uses that key to delete it
  [services removeObjectsForKeys:[services allKeysForObject:netService]];
}

// Error handling code
- (void)handleError:(NSNumber *)error withService:(NSNetService *)service
{
  NSLog(@"CZeroconfOSX::handleError An error occurred with service %@.%@.%@, error code = %@",
        [service name], [service type], [service domain], error);
  // Handle error here
}

@end


struct CZeroconfOSX::CZeroconfOSXData
{
  ZeroconfOSXImpl* impl;
};

CZeroconfOSX::CZeroconfOSX():mp_data(new CZeroconfOSXData)
{
  mp_data->impl = [[ZeroconfOSXImpl alloc] init];
}

CZeroconfOSX::~CZeroconfOSX()
{
  [ZeroconfOSXImpl release];
}

//methods to implement for concrete implementations
bool CZeroconfOSX::doPublishService(const std::string& fcr_identifier,
                      const std::string& fcr_type,
                      const std::string& fcr_name,
                      unsigned int f_port)
{
  return [mp_data->impl publishService:[NSString stringWithCString:fcr_identifier.c_str()]
                              withType:[NSString stringWithCString:fcr_type.c_str()]
                              withName:[NSString stringWithCString:fcr_name.c_str()]
                              withPort:f_port
          ];
}

bool CZeroconfOSX::doRemoveService(const std::string& fcr_ident)
{
  return [mp_data->impl removeService:[NSString stringWithCString:fcr_ident.c_str()]];
}

bool CZeroconfOSX::doHasService(const std::string& fcr_ident)
{
  return [mp_data->impl hasService:[NSString stringWithCString:fcr_ident.c_str()]];
}

void CZeroconfOSX::doStop()
{
  [mp_data->impl stop];
}
