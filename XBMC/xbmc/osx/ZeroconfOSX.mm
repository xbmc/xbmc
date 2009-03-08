#import "ZeroconfOSX.h"

#import <string>

#import <Cocoa/Cocoa.h>
#import <netinet/in.h>
#import <sys/socket.h>


@interface ZeroconfOSXImpl : NSObject {
    // Keeps track of active services or services about to be published
    NSMutableArray* services;
}

- (void) publishWebserverWithPort:(int) port withPrefix:(NSString*) prefix;
- (void) removeWebserverWithPrefix:(NSString*) prefix;

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

-(id) init{
    if(self = [super init]){
        NSLog(@" ZeroconfOSX created");
    }
    services = [[NSMutableArray alloc] init];
    return self;
}

-(void) dealloc{
    [self stop];
    [services release];
    [super dealloc];
}

- (void) publishWebserverWithPort:(int) port withPrefix:(NSString*) prefix{
    NSLog(@" ZeroconfOSX::publishWebserverWithPort %i withPrefix %@", port, prefix);
    //we're publishing the webserver as XBMC@$(HOSTNAME)
    NSString *hostname = [[NSProcessInfo processInfo] hostName];
    NSNetService* webserver = [[NSNetService alloc] initWithDomain:@""// 4
                                                            type:@"_http._tcp"
                                                            name:[NSString stringWithFormat:@"%@@%@", prefix, hostname] port:port];
    if(webserver)
    {
        [webserver setDelegate:self];
        [services addObject:webserver];
        [webserver publish];
    }
    else
    {
        NSLog(@"An error occurred initializing the NSNetService object.");
        [webserver release];
        webserver = nil;
    }
}

- (void) removeWebserverWithPrefix:(NSString*) prefix{
    //go through our services array and search for the webserver
    unsigned int i;
    for(i=0; i < [services count]; ++i){
        NSNetService* service = [services objectAtIndex:i];
        if([[service type] isEqualTo:@"_http._tcp"] && [[service name] hasPrefix:prefix]){
            [service stop];
            break;
        }
    }
}

-(void) stop{
    NSLog(@" ZeroconfOSX::stop");
    unsigned int i;
    for(i=0; i < [services count]; ++i){
        NSNetService* service = [services objectAtIndex:i];
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
    [self handleError:[errorDict objectForKey:NSNetServicesErrorCode] withService:netService];
    [services removeObject:netService];
}

// Sent when the service stops
- (void)netServiceDidStop:(NSNetService *)netService
{
    [services removeObject:netService];
    // You may want to do something here, such as updating a user interface
}

// Error handling code
- (void)handleError:(NSNumber *)error withService:(NSNetService *)service
{
    NSLog(@"An error occurred with service %@.%@.%@, error code = %@",
          [service name], [service type], [service domain], error);
    // Handle error here
}

@end


struct CZeroconfOSX::CZeroconfOSXData{
    ZeroconfOSXImpl* impl;  
};

CZeroconfOSX::CZeroconfOSX():mp_data(new CZeroconfOSXData)
{
    mp_data->impl = [[ZeroconfOSXImpl alloc] init];
}

CZeroconfOSX::~CZeroconfOSX(){
    [ZeroconfOSXImpl release];
}

void CZeroconfOSX::doPublishWebserver(int f_port){
    [mp_data->impl publishWebserverWithPort:f_port withPrefix: [NSString stringWithCString:GetWebserverPublishPrefix().c_str()]];
}

void  CZeroconfOSX::doRemoveWebserver(){
    [mp_data->impl removeWebserverWithPrefix:[NSString stringWithCString:GetWebserverPublishPrefix().c_str()]];
}

void CZeroconfOSX::doStop(){
    [mp_data->impl stop];
}
