/*
 *      Copyright (C) 2010-2013 Team XBMC
 *      http://xbmc.org
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


/* HowTo code in this file:
 * Since AppleTV/iOS6.x (atv2 version 5.2) Apple removed the AppleTV.framework and put all those classes into the
 * AppleTV.app. So we can't use standard obj-c coding here anymore. Instead we need to use the obj-c runtime
 * functions for subclassing and adding methods to our instances during runtime (hooking).
 * 
 * 1. For implementing a method of a base class:
 *  a) declare it in the form <KodiAppliance$nameOfMethod> like the others
 *  b) these methods need to be static and have KodiAppliance* self, SEL _cmd (replace KodiAppliance with the class the method gets implemented for) as minimum params.
 *  c) add the method to the KodiAppliance.h for getting rid of the compiler warnings of unresponsive selectors (declare the method like done in the baseclass).
 *  d) in initApplianceRuntimeClasses exchange the base class implementation with ours by calling MSHookMessageEx
 *  e) if we need to call the base class implementation as well we have to save the original implementation (see initWithApplianceInfo$Orig for reference)
 *
 * 2. For implementing a new method which is not part of the base class:
 *  a) same as 1.a
 *  b) same as 1.b
 *  c) same as 1.c
 *  d) in initApplianceRuntimeClasses add the method to our class via class_addMethod
 *
 * 3. Never access any BackRow classes directly - but always get the class via objc_getClass - if the class is used in multiple places 
 *    save it as static (see BRApplianceCategoryCls)
 * 
 * 4. Keep the structure of this file based on the section comments (marked with // SECTIONCOMMENT).
 * 5. really - obey 4.!
 *
 * 6. for adding class members use associated objects - see topShelfControllerKey
 *
 * For further reference see https://developer.apple.com/library/mac/#documentation/Cocoa/Reference/ObjCRuntimeRef/Reference/reference.html
 */

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

#import "KodiAppliance.h"
#import "KodiController.h"
#include "substrate.h"

// SECTIONCOMMENT
// classes we need multiple times
static Class BRApplianceCategoryCls;

// category for ios5.x and higher is just a short text before xbmc auto starts
#define KodiAppliance_CAT_5andhigher [BRApplianceCategoryCls categoryWithName:@"Kodi is starting..." identifier:@"kodi" preferredOrder:0]
// category for ios4.x is the menu entry
#define KodiAppliance_CAT_4 [BRApplianceCategoryCls categoryWithName:@"Kodi" identifier:@"kodi" preferredOrder:0]

// SECTIONCOMMENT
// forward declaration all referenced classes
@class KodiAppliance;
@class BRTopShelfView;
@class KodiApplianceInfo;
@class BRMainMenuImageControl;

// SECTIONCOMMENT
// orig method handlers we wanna call in hooked methods
static id (*KodiAppliance$initWithApplianceInfo$Orig)(KodiAppliance*, SEL, id);
static id (*KodiAppliance$init$Orig)(KodiAppliance*, SEL);
static id (*KodiAppliance$applianceInfo$Orig)(KodiAppliance*, SEL);

//--------------------------------------------------------------
//--------------------------------------------------------------
// ATVVersionInfo declare to shut up compiler warning
@interface ATVVersionInfo : NSObject
{
}
+ (id)currentOSVersion;

@end

@interface AppATV2Detector : NSObject{}
+ (BOOL) hasOldGui;
+ (BOOL) isIos5;
+ (BOOL) needsApplianceInfoHack;
@end

//--------------------------------------------------------------
//--------------------------------------------------------------
// We need a real implementation (not a runtime generated one)
// for getting our NSBundle instance by calling
// [[NSBundle bundleForClass:objc_getClass("AppATV2Detector")]
// so we just implement some usefull helpers here
// and use those
@implementation AppATV2Detector : NSObject{}
+ (BOOL) hasOldGui
{
  Class cls = NSClassFromString(@"ATVVersionInfo");  
  if (cls != nil && [[cls currentOSVersion] rangeOfString:@"4."].location != NSNotFound)
    return TRUE;
  if (cls != nil && [[cls currentOSVersion] rangeOfString:@"5.0"].location != NSNotFound)
    return TRUE;
  return FALSE;
}

+ (BOOL) isIos5
{
  Class cls = NSClassFromString(@"ATVVersionInfo");  
  if (cls != nil && [[cls currentOSVersion] rangeOfString:@"5."].location != NSNotFound)
    return TRUE;
  return FALSE;
}

+ (BOOL) needsApplianceInfoHack
{
  // if the runtime base class (BRBaseAppliance) doesn't have the initWithApplianceInfo selector
  // we need to hack the appliance info in (id) applianceInfo (KodiAppliance$applianceInfo)
  if (class_respondsToSelector(objc_getClass("BRBaseAppliance"),@selector(initWithApplianceInfo:)))
    return FALSE;
  return TRUE;
}
@end

//--------------------------------------------------------------
//--------------------------------------------------------------
// XBMCApplication declare to shut up compiler warning of BRApplication
@interface XBMCApplication : NSObject
{
}
- (void)setFirstResponder:(id)responder;
@end

//--------------------------------------------------------------
//--------------------------------------------------------------
@interface KodiTopShelfController : NSObject
{
}
- (void) selectCategoryWithIdentifier:(id)identifier;
- (id) topShelfView;
- (id) mainMenuShelfView;
// added in 4.1+
- (void) refresh;
@end

//--------------------------------------------------------------
//--------------------------------------------------------------
@implementation KodiTopShelfController

- (void) selectCategoryWithIdentifier:(id)identifier 
{
}

- (BRTopShelfView *)topShelfView 
{
  Class cls = objc_getClass("BRTopShelfView");
  id topShelf = [[cls alloc] init];
  
  // diddle the topshelf logo on old gui
  if ([AppATV2Detector hasOldGui])
  {
    Class cls = objc_getClass("BRImage");
    BRImageControl *imageControl = (BRImageControl *)MSHookIvar<id>(topShelf, "_productImage");// hook the productImage so we can diddle with it
    BRImage *gpImage = [cls imageWithPath:[[NSBundle bundleForClass:[AppATV2Detector class]] pathForResource:@"TopShelf" ofType:@"png"]];
    [imageControl setImage:gpImage];
  }

  return topShelf;
}

// this method is called with the new ios ui (ios 5.1 and higher)
// its similar to the topshelf view on the opd ios gui
// but its more mighty (thats we we need to dig one level deeper here)
// to get our loogo visible
- (id) mainMenuShelfView;
{
  Class BRTopShelfViewCls = objc_getClass("BRTopShelfView");
  Class BRImageCls = objc_getClass("BRImage");

  id topShelf = [[BRTopShelfViewCls alloc] init];
  
  // first hook into the mainMenuImageControl
  // which is a wrapper for an image control
  BRMainMenuImageControl *mainMenuImageControl = (BRMainMenuImageControl *)MSHookIvar<id>(topShelf, "_productImage");
  // now get the image instance
  BRImageControl *imageControl = (BRImageControl *)MSHookIvar<id>(mainMenuImageControl, "_content");// hook the image so we can diddle with it
  
  // load our logo into it
  BRImage *gpImage = [BRImageCls imageWithPath:[[NSBundle bundleForClass:[AppATV2Detector class]] pathForResource:@"Kodi" ofType:@"png"]];
  [imageControl setImage:gpImage];
  return topShelf;
}

- (void) refresh
{
}
@end

//--------------------------------------------------------------
//--------------------------------------------------------------
// SECTIONCOMMENT
// since we can't inject ivars we need to use associated objects
// these are the keys for KodiAppliance
//implementation KodiAppliance
static char topShelfControllerKey;
static char applianceCategoriesKey;

static NSString* KodiApplianceInfo$key(KodiApplianceInfo* self, SEL _cmd)
{
  return [[[NSBundle bundleForClass:objc_getClass("AppATV2Detector")] infoDictionary] objectForKey:(NSString*)kCFBundleIdentifierKey];
}

static NSString* KodiApplianceInfo$name(KodiApplianceInfo* self, SEL _cmd)
{
  return [[[NSBundle bundleForClass:objc_getClass("AppATV2Detector")] infoDictionary] objectForKey:(NSString*)kCFBundleNameKey];
}

static id KodiApplianceInfo$localizedStringsFileName(KodiApplianceInfo* self, SEL _cmd)
{
  return @"kodi";
}

static void KodiAppliance$XBMCfixUIDevice(KodiAppliance* self, SEL _cmd)
{
  // iOS 5.x has removed the internal load of UIKit in AppleTV app
  // and there is an overlap of some UIKit and AppleTV methods.
  // This voodoo seems to clear up the wonkiness. :)
  if ([AppATV2Detector isIos5])
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


static id KodiAppliance$init(KodiAppliance* self, SEL _cmd)
{
   //NSLog(@"%s", __PRETTY_FUNCTION__);
  if ([AppATV2Detector needsApplianceInfoHack])
  {
    NSLog(@"%s for ios 4", __PRETTY_FUNCTION__);
    if ((self = KodiAppliance$init$Orig(self, _cmd))!= nil)
    {
      id topShelfControl = [[KodiTopShelfController alloc] init];
      [self setTopShelfController:topShelfControl];
      
      NSArray *catArray = [[NSArray alloc] initWithObjects:KodiAppliance_CAT_4,nil];
      [self setApplianceCategories:catArray];
      return self;
    }    
  }
  else// ios >= 5
  {
    NSLog(@"%s for ios 5 and newer", __PRETTY_FUNCTION__);
    return [self initWithApplianceInfo:nil]; // legacy for ios < 6
  }
  return self;
}

static id KodiAppliance$identifierForContentAlias(KodiAppliance* self, SEL _cmd, id contentAlias)
{
  return@"kodi";
}

static BOOL KodiAppliance$handleObjectSelection(KodiAppliance* self, SEL _cmd, id fp8, id fp12)
{
  //NSLog(@"%s", __PRETTY_FUNCTION__);
  return YES;
}              

static id KodiAppliance$applianceInfo(KodiAppliance* self, SEL _cmd)
{
  //NSLog(@"%s", __PRETTY_FUNCTION)
  
  // load our plist into memory and merge it with
  // the dict from the baseclass if needed
  // cause ios seems to fail on that somehow (at least on 4.x)
  if ([AppATV2Detector needsApplianceInfoHack] && self != nil)
  {
    id original = KodiAppliance$applianceInfo$Orig(self, _cmd);
    id info =  MSHookIvar<id>(original, "_info");// hook the infoDictionary so we can diddle with it
    
    NSString *plistPath = [[NSBundle bundleForClass:objc_getClass("AppATV2Detector")] pathForResource:@"Info" ofType:@"plist"];
    NSString *bundlePath = [[NSBundle bundleForClass:objc_getClass("AppATV2Detector")] bundlePath];
    NSMutableDictionary *ourInfoDict = [[NSMutableDictionary alloc] initWithContentsOfFile:plistPath];

    if (ourInfoDict != nil && bundlePath != nil)
    {
      // inject this or we won't get shown up properly on ios4
      [ourInfoDict setObject:bundlePath forKey:@"NSBundleInitialPath"];
      
      // add our plist info to the baseclass info and return it
      [(NSMutableDictionary *)info addEntriesFromDictionary:ourInfoDict];
      [ourInfoDict release];
    }
    return original;
  }
  else
  {
    Class cls = objc_getClass("KodiApplianceInfo");
    return [[[cls alloc] init] autorelease];
  }
  return nil;
}


static id KodiAppliance$topShelfController(KodiAppliance* self, SEL _cmd)
{ 
  return objc_getAssociatedObject(self, &topShelfControllerKey);
}


static void KodiAppliance$setTopShelfController(KodiAppliance* self, SEL _cmd, id topShelfControl)
{ 
  objc_setAssociatedObject(self, &topShelfControllerKey, topShelfControl, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}

static id KodiAppliance$applianceCategories(KodiAppliance* self, SEL _cmd)
{
  return objc_getAssociatedObject(self, &applianceCategoriesKey);
}

static void KodiAppliance$setApplianceCategories(KodiAppliance* self, SEL _cmd, id applianceCategories)
{ 
  objc_setAssociatedObject(self, &applianceCategoriesKey, applianceCategories, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}

static id KodiAppliance$initWithApplianceInfo(KodiAppliance* self, SEL _cmd, id applianceInfo)
{ 
  //NSLog(@"%s", __PRETTY_FUNCTION__);
  if((self = KodiAppliance$initWithApplianceInfo$Orig(self, _cmd, applianceInfo)) != nil)
  {
    id topShelfControl = [[KodiTopShelfController alloc] init];
    [self setTopShelfController:topShelfControl];

    NSArray *catArray = [[NSArray alloc] initWithObjects:KodiAppliance_CAT_5andhigher,nil];
    [self setApplianceCategories:catArray];
  } 
  return self;
}

static id KodiAppliance$controllerForIdentifier(KodiAppliance* self, SEL _cmd, id identifier, id args)
{
  //NSLog(@"%s", __PRETTY_FUNCTION__);
  id menuController = nil;
  Class cls = objc_getClass("BRApplication");
  if ([identifier isEqualToString:@"kodi"])
  {
    [self XBMCfixUIDevice];
    menuController = [[objc_getClass("KodiController") alloc] init];
    if (menuController == nil)
      NSLog(@"initialise controller - fail");
  }
  XBMCApplication *brapp = (XBMCApplication *)[cls sharedApplication];
  [brapp setFirstResponder:menuController];
  return menuController;
}

static void XBMCPopUpManager$_displayPopUp(BRPopUpManager *self, SEL _cmd, id up)
{
  // suppress all popups
  NSLog(@"%s suppressing popup.", __PRETTY_FUNCTION__);
}

// helper function. If the given class responds to the selector
// we hook via MSHookMessageEx
// bCheckSuperClass <- indicates if the hookClass or ist superclass should be checked for hookSelector
// return true if we hooked - else false
static BOOL safeHook(Class hookClass, SEL hookSelector, IMP ourMethod, IMP *theirMethod, BOOL bCheckSuperClass = true)
{
  Class checkClass = class_getSuperclass(hookClass);
  if (!bCheckSuperClass || !checkClass)
    checkClass = hookClass;

  if (class_respondsToSelector(checkClass, hookSelector))
  {
    MSHookMessageEx(hookClass, hookSelector, ourMethod, theirMethod);
    return TRUE;
  }
  return FALSE;
}

// SECTIONCOMMENT
// c'tor - this sets up our class at runtime by 
// 1. subclassing from the base classes
// 2. adding new methods to our class
// 3. exchanging (hooking) base class methods with ours
// 4. register the classes to the objc runtime system
static __attribute__((constructor)) void initApplianceRuntimeClasses()
{
  // Hook into the popup manager and prevent any popups
  // the problem with popups is that when they disappear XBMC is
  // getting 100% transparent (invisible). This can be tested with
  // the new bluetooth feature in ios6 when a keyboard is connected
  // a popup is shown (its behind XBMCs window). When it disappears
  // XBMC does so too.
  safeHook(objc_getClass("BRPopUpManager"), @selector(_displayPopUp:), (IMP)&XBMCPopUpManager$_displayPopUp, nil, NO);
  
  // subclass BRApplianceInfo into KodiApplianceInfo
  Class KodiApplianceInfoCls = objc_allocateClassPair(objc_getClass("BRApplianceInfo"), "KodiApplianceInfo", 0);

  // and hook up our methods (implementation of the base class methods)
  // KodiApplianceInfo::key
  safeHook(KodiApplianceInfoCls,@selector(key), (IMP)&KodiApplianceInfo$key, nil);
  // KodiApplianceInfo::name
  safeHook(KodiApplianceInfoCls,@selector(name), (IMP)&KodiApplianceInfo$name, nil);
  // KodiApplianceInfo::localizedStringsFileName
  safeHook(KodiApplianceInfoCls,@selector(localizedStringsFileName), (IMP)&KodiApplianceInfo$localizedStringsFileName, nil);

  // and register the class to the runtime
  objc_registerClassPair(KodiApplianceInfoCls);

  // subclass BRBaseAppliance into KodiAppliance
  Class KodiApplianceCls = objc_allocateClassPair(objc_getClass("BRBaseAppliance"), "KodiAppliance", 0);
  // add our custom methods which are not part of the baseclass
  // KodiAppliance::XBMCfixUIDevice
  class_addMethod(KodiApplianceCls,@selector(XBMCfixUIDevice), (IMP)KodiAppliance$XBMCfixUIDevice, "v@:");
  class_addMethod(KodiApplianceCls,@selector(setTopShelfController:), (IMP)&KodiAppliance$setTopShelfController, "v@:@");
  class_addMethod(KodiApplianceCls,@selector(setApplianceCategories:), (IMP)&KodiAppliance$setApplianceCategories, "v@:@");

  // and hook up our methods (implementation of the base class methods)
  // KodiAppliance::init
  safeHook(KodiApplianceCls,@selector(init), (IMP)&KodiAppliance$init, (IMP*)&KodiAppliance$init$Orig);
  // KodiAppliance::identifierForContentAlias
  safeHook(KodiApplianceCls,@selector(identifierForContentAlias:), (IMP)&KodiAppliance$identifierForContentAlias, nil);
  // KodiAppliance::handleObjectSelection
  safeHook(KodiApplianceCls,@selector(handleObjectSelection:userInfo:), (IMP)&KodiAppliance$handleObjectSelection, nil);
  // KodiAppliance::applianceInfo
  safeHook(KodiApplianceCls,@selector(applianceInfo), (IMP)&KodiAppliance$applianceInfo, (IMP *)&KodiAppliance$applianceInfo$Orig);
  // KodiAppliance::topShelfController
  safeHook(KodiApplianceCls,@selector(topShelfController), (IMP)&KodiAppliance$topShelfController, nil);
  // KodiAppliance::applianceCategories
  safeHook(KodiApplianceCls,@selector(applianceCategories), (IMP)&KodiAppliance$applianceCategories, nil);
  // KodiAppliance::initWithApplianceInfo
  safeHook(KodiApplianceCls,@selector(initWithApplianceInfo:), (IMP)&KodiAppliance$initWithApplianceInfo, (IMP*)&KodiAppliance$initWithApplianceInfo$Orig);
  // KodiAppliance::controllerForIdentifier
  safeHook(KodiApplianceCls,@selector(controllerForIdentifier:args:), (IMP)&KodiAppliance$controllerForIdentifier, nil);

  // and register the class to the runtime
  objc_registerClassPair(KodiApplianceCls);
 
  // save this as static for referencing it in the macro at the top of the file
  BRApplianceCategoryCls = objc_getClass("BRApplianceCategory");
}