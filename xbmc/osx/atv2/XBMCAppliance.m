/*
 *      Copyright (C) 2010-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
 
#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import <BackRow/BackRow.h>
// objc-runtime.h is missing from iPhoneOS4.2SDK but present in iPhoneSimulator4.2.sdk
// pull it from runtime system for now
#import "/usr/include/objc/objc-runtime.h"

#import "XBMCAppliance.h"
#import "XBMCController.h"

#define XBMCAppliance_CAT [BRApplianceCategory categoryWithName:@"XBMC" identifier:@"xbmc" preferredOrder:-5]

// ATVVersionInfo declare to shut up compiler warning
@interface ATVVersionInfo : NSObject
{
}
+ (id)currentOSVersion;
@end
//--------------------------------------------------------------
//--------------------------------------------------------------
@interface BRTopShelfView (specialAdditions)
//
- (BRImageControl *)productImage;

@end

@implementation BRTopShelfView (specialAdditions)
- (BRImageControl *)productImage
{
  Ivar ivar = object_getInstanceVariable(self, "_productImage", NULL);
  id result = object_getIvar(self, ivar);
  return result;
}
@end

//--------------------------------------------------------------
//--------------------------------------------------------------
@interface XBMCTopShelfController : NSObject
{
}
- (void) selectCategoryWithIdentifier:(id)identifier;
- (id) topShelfView;
// added in 4.1+
- (void) refresh;
@end

@implementation XBMCTopShelfController
//
- (void) selectCategoryWithIdentifier:(id)identifier 
{
}

- (BRTopShelfView *)topShelfView {
	BRTopShelfView *topShelf = [[BRTopShelfView alloc] init];
	BRImageControl *imageControl = [topShelf productImage];
	BRImage *gpImage = [BRImage imageWithPath:[[NSBundle bundleForClass:[XBMCAppliance class]] pathForResource:@"XBMC" ofType:@"png"]];
	[imageControl setImage:gpImage];
	
	return topShelf;
}
- (void) refresh
{
}
@end

//--------------------------------------------------------------
//--------------------------------------------------------------
@implementation XBMCAppliance
@synthesize topShelfController=_topShelfController;

-(void)XBMCfixUIDevice
{
  // iOS 5.x has removed the internal load of UIKit in AppleTV app
  // and there is an overlap of some UIKit and AppleTV methods.
  // This voodoo seems to clear up the wonkiness. :)
  Class cls = NSClassFromString(@"ATVVersionInfo");
  if (cls != nil && [[cls currentOSVersion] rangeOfString:@"5."].location != NSNotFound)
  {
    id cd = nil;

    @try
    {
      cd = [UIDevice currentDevice];
    }
    
    @catch (NSException *e)
    {
      NSLog(@"exception: %@", e);
    }
    
    @finally
    {
      //NSLog(@"will it work the second try?");
      cd = [UIDevice currentDevice];
      NSLog(@"current device fixed: %@", cd);
    }
  }
}

-(id) init
{
  //NSLog(@"%s", __PRETTY_FUNCTION__);

  if ((self = [super init]) != nil) 
  {
    _topShelfController = [[XBMCTopShelfController alloc] init];
    _applianceCategories = [[NSArray alloc] initWithObjects:XBMCAppliance_CAT ,nil];
	}

  return self;
}

- (void) dealloc
{
  //NSLog(@"%s", __PRETTY_FUNCTION__);

  [_applianceCategories release];
  [_topShelfController release];

	[super dealloc];
}

- (id) applianceCategories
{
	return _applianceCategories;
}

- (id) identifierForContentAlias:(id)contentAlias
{
	return @"xbmc";
}

- (id) selectCategoryWithIdentifier:(id)ident
{
	//NSLog(@"eglv2:selecteCategoryWithIdentifier: %@", ident);

	return nil;
}
- (BOOL) handleObjectSelection:(id)fp8 userInfo:(id)fp12
{
  //NSLog(@"%s", __PRETTY_FUNCTION__);

	return YES;
}

- (id) applianceSpecificControllerForIdentifier:(id)arg1 args:(id)arg2
{
  return nil;
}
- (BOOL) handlePlay:(id)play userInfo:(id)info
{
  //NSLog(@"%s", __PRETTY_FUNCTION__);

  return YES;
}

- (id) controllerForIdentifier:(id)identifier args:(id)args
{
  //NSLog(@"%s", __PRETTY_FUNCTION__);
  
  [self XBMCfixUIDevice];
  XBMCController *controller = [[[XBMCController alloc] init] autorelease];
  //XBMCController *controller = [XBMCController sharedInstance];
  return controller;
}

- (id) localizedSearchTitle { return @"xbmc"; }
- (id) applianceName { return @"xbmc"; }
- (id) moduleName { return @"xbmc"; }
- (id) applianceKey { return @"xbmc"; }

@end

