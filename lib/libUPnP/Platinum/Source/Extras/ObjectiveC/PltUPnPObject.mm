//
//  PltUPnPObject.mm
//  Platinum
//
//  Created by Sylvain on 9/14/10.
//  Copyright 2010 Plutinosoft LLC. All rights reserved.
//

#import "Platinum.h"
#import "PltUPnPObject.h"

/*----------------------------------------------------------------------
|   PLT_ActionObject
+---------------------------------------------------------------------*/
@implementation PLT_ActionObject

- (id)initWithAction:(PLT_Action *)_action
{
    if ((self = [super init])) {
        action = _action;
    }
    return self;
}

- (NPT_Result)setValue:(NSString *)value forArgument:(NSString *)argument
{
    return action->SetArgumentValue([argument UTF8String], [value UTF8String]);
}

- (NPT_Result)setErrorCode:(unsigned int)code withDescription:(NSString*)description
{
    return action->SetError(code, [description UTF8String]);
}

@end

/*----------------------------------------------------------------------
|   PLT_DeviceHostObject
+---------------------------------------------------------------------*/
@implementation PLT_DeviceHostObject

- (id)init
{
	return [super init];
}

- (void)setDevice:(PLT_DeviceHostReference*)_device
{
	delete device;
	device = new PLT_DeviceHostReference(*_device);
}

- (void)dealloc
{
    delete device;
}

- (PLT_DeviceHostReference&)getDevice 
{
    return *device;
}

@end

/*----------------------------------------------------------------------
|   PLT_UPnPObject
+---------------------------------------------------------------------*/

@interface PLT_UPnPObject () {
	PLT_UPnP *_upnp;
	NSMutableArray *_devices;
}
@end

@implementation PLT_UPnPObject

- (id)init
{
    if ((self = [super init])) {
        _upnp = new PLT_UPnP();
		_devices = [NSMutableArray array];
    }
    return self;
}

-(void) dealloc
{
    delete _upnp;
}

- (NPT_Result)start
{
    return _upnp->Start();
}

- (NPT_Result)stop
{
    return _upnp->Stop();
}

- (bool)isRunning
{
    return _upnp->IsRunning();
}

- (NPT_Result)addDevice:(PLT_DeviceHostObject*)device
{
	[_devices addObject:device];
    return _upnp->AddDevice([device getDevice]);
}

- (NPT_Result)removeDevice:(PLT_DeviceHostObject*)device
{
	[_devices removeObject:device];
    return _upnp->RemoveDevice([device getDevice]);
}

@end
