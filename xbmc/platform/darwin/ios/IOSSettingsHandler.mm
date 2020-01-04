/*
 *  Copyright (C) 2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "IOSSettingsHandler.h"

#include "CompileInfo.h"
#include "ServiceBroker.h"
#include "filesystem/SpecialProtocol.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/Setting.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

#import <UIKit/UIKit.h>

NSString* const DisableSafeAreaDefaultsKey = @"DisableSafeArea";

IOSSettingsHandler::IOSSettingsHandler()
{
  std::set<std::string> watchedSettings{CSettings::SETTING_DEBUG_SHARE_LOG,
                                        CSettings::SETTING_LOOKANDFEEL_EXTENDUIUNDERNOTCH};
  CServiceBroker::GetSettingsComponent()->GetSettings()->RegisterCallback(this, watchedSettings);
}

IOSSettingsHandler::~IOSSettingsHandler()
{
  CServiceBroker::GetSettingsComponent()->GetSettings()->UnregisterCallback(this);
}

void IOSSettingsHandler::OnSettingAction(std::shared_ptr<const CSetting> setting)
{
  if (!setting || setting->GetId() != CSettings::SETTING_DEBUG_SHARE_LOG)
    return;

  std::string lowerAppName{CCompileInfo::GetAppName()};
  StringUtils::ToLower(lowerAppName);
  auto path = URIUtils::AddFileToFolder(CSpecialProtocol::TranslatePath("special://logpath/"),
                                        lowerAppName + ".log");

  dispatch_async(dispatch_get_main_queue(), ^{
    auto infoDic = NSBundle.mainBundle.infoDictionary;
    auto subject =
        [NSString stringWithFormat:@"iOS log - Kodi %@ %@", infoDic[@"CFBundleShortVersionString"],
                                   infoDic[static_cast<NSString*>(kCFBundleVersionKey)]];

    auto activityVc = [[UIActivityViewController alloc]
        initWithActivityItems:@[ [NSURL fileURLWithPath:@(path.c_str())] ]
        applicationActivities:nil];
    // hacky way to set email subject instead of providing UIActivityItemSource object
    [activityVc setValue:subject forKey:@"subject"];

    // make sure that the sharing sheet is displayed on iOS device's screen
    auto iosDeviceScreen = UIScreen.mainScreen;
    auto iosDeviceWindow = UIApplication.sharedApplication.keyWindow;
    if (iosDeviceWindow.screen != iosDeviceScreen)
    {
      for (UIWindow* window in UIApplication.sharedApplication.windows)
      {
        if (window.screen == iosDeviceScreen)
        {
          iosDeviceWindow = window;
          break;
        }
      }
    }

    auto rootVc = iosDeviceWindow.rootViewController;
    [rootVc presentViewController:activityVc animated:YES completion:nil];

    // iPad must present the sharing sheet in a popover
    activityVc.popoverPresentationController.sourceView = rootVc.view;
    activityVc.popoverPresentationController.sourceRect = CGRectMake(0.0, 0.0, 10.0, 10.0);
  });
}

void IOSSettingsHandler::OnSettingChanged(std::shared_ptr<const CSetting> setting)
{
  if (!setting || setting->GetId() != CSettings::SETTING_LOOKANDFEEL_EXTENDUIUNDERNOTCH)
    return;

  // the value is also saved to native settings because Kodi settings
  // are unavailable on app start when `XBMCController` is loaded
  // @todo figure out how to resize `IOSEAGLView` properly, without app restart
  auto disableSafeArea = std::static_pointer_cast<const CSettingBool>(setting)->GetValue();
  [NSUserDefaults.standardUserDefaults setBool:static_cast<BOOL>(disableSafeArea)
                                        forKey:DisableSafeAreaDefaultsKey];
}
