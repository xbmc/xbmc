# HOW-TO: Install Kodi for iOS

The current stable release for iOS and iPadOS is available from the Kodi iOS download page.

## Requirements

To install Kodi for iOS or iPadOS you must have one of the following:

- a normal iPhone or iPad and a way to sideload a Kodi `.ipa` file
- a jailbroken iPhone or iPad and a package manager such as Sileo, Zebra or Cydia
- a Mac with Xcode if you want to build and sign Kodi yourself

Current Kodi releases run on iOS 11 and later. Older iOS versions require older Kodi releases.

## Which install method should I use?

Most users should use one of the non-jailbroken sideloading methods below.

| Method | Best for | Notes |
| --- | --- | --- |
| AltStore Classic | Most non-jailbroken users | Apps expire after 7 days with a free Apple account and must be refreshed. |
| SideStore | Users who want to refresh on-device | Requires a computer for the initial setup, then can refresh apps without a computer. |
| Sideloadly | Simple install from Windows or macOS | Can install and automatically refresh apps when your device is seen by the computer. |
| TrollStore | Older supported iOS versions | Only works on supported iOS versions. Not a general current-iOS method. |
| Jailbreak package manager | Jailbroken devices | Uses the Kodi `.deb` package. |
| Xcode build | Developers | Build Kodi yourself and export or package an `.ipa`. |

## Download Kodi package

Some methods use a Kodi `.ipa` file. Jailbroken devices use a Kodi `.deb` file.

- Latest stable Kodi package: <https://kodi.tv/download/ios/>
- Stable and older iOS builds: <https://mirrors.kodi.tv/releases/darwin/>
- Nightly builds: <https://mirrors.kodi.tv/nightlies/darwin/>

Note: Development and nightly builds are test builds. ALWAYS BACKUP YOUR USERDATA FOLDER WHEN USING DEVELOPMENT BUILDS!

## Non-jailbroken devices

These methods install Kodi by signing the app with your own Apple account. If you use a free Apple developer account, Kodi normally has to be refreshed every 7 days. If Kodi expires, do not uninstall Kodi unless you want to remove its data. Refresh or reinstall it with the same Apple account and bundle ID.

### Option A: Install using AltStore Classic

This method uses AltStore Classic on your iPhone or iPad and AltServer on your computer.

1. Install AltServer on your Mac or Windows PC.
2. Install AltStore Classic to your iPhone or iPad by following the AltStore instructions.
3. Download the Kodi `.ipa` file to your device or to iCloud Drive.
4. Open AltStore on your iPhone or iPad.
5. Open the My Apps tab and press the `+` button.
6. Select the Kodi `.ipa` file.
7. Wait for AltStore to finish installing Kodi.
8. If prompted, go to Settings -> General -> VPN & Device Management and trust the Apple ID used to install Kodi.

Note: Apps installed with AltStore Classic expire after 7 days when using a free Apple account. AltStore can refresh apps, but AltServer must be available on the same network.

### Option B: Install using SideStore

This method is similar to AltStore, but after the initial setup it can refresh apps directly from the device.

1. Install SideStore by following the SideStore instructions.
2. Complete the first-time setup on your iPhone or iPad.
3. Download the Kodi `.ipa` file.
4. Open SideStore.
5. Choose the Kodi `.ipa` file and install it.
6. If prompted, trust the Apple ID used to install Kodi in Settings -> General -> VPN & Device Management.

Note: SideStore still uses Apple's free-account signing limits. Refresh Kodi before it expires.

### Option C: Install using Sideloadly

Note: Windows or macOS can be used for this method.

Make sure you have the following software on your computer:

- Sideloadly: <https://sideloadly.io/>
- A Kodi `.ipa` file

Procedure, step by step:

1. Connect your iPhone or iPad to your computer and make sure that it is turned on.
2. Open Sideloadly and make sure your device is detected.
3. Drag the Kodi `.ipa` file into Sideloadly.
4. Enter the Apple account you want to use for signing.
5. Press Start and wait until Sideloadly reports that the install is done.
6. Once you see the Kodi icon on your device, go to Settings -> General -> VPN & Device Management and trust the Apple ID used for installation.

Note: Sideloadly can enroll apps for automatic refresh. A computer is still needed for refresh, either over USB or Wi-Fi.

### Option D: Install using TrollStore

TrollStore can permanently install `.ipa` files on supported iOS versions. This is an advanced method and is only available for specific iOS versions. If your device is not supported by TrollStore, use AltStore, SideStore or Sideloadly instead.

## Jailbroken devices

If your device is not jailbroken, please follow one of the methods in the non-jailbroken devices section.

Kodi `.deb` packages are for jailbroken devices. You do not need Cydia specifically. Use the package manager that is recommended for your jailbreak, such as Sileo, Zebra or Cydia.

### Option A: Install from a package manager

1. Open your jailbreak package manager.
2. Refresh sources.
3. Search for Kodi.
4. Select the Kodi package for iOS.
5. Press Install or Get.
6. Confirm the installation.
7. Wait for the package manager to finish.
8. Respring if prompted.

Updates for stable releases of Kodi are handled by your package manager and will show up when available after refreshing sources.

### Option B: Install a `.deb` file manually

1. Download the Kodi `.deb` file from the Kodi mirror.
2. Open the file in Filza or another jailbreak file manager.
3. Choose Install.
4. Wait for the installation to complete.
5. Respring if prompted.

Note: `.deb` packages are not recommended for normal non-jailbroken sideloading. Use a Kodi `.ipa` file instead.

### Removing settings

Kodi will uninstall from your device but will leave certain things, including databases and userdata, behind to make it easier to reinstall. To delete those, remove:

- `/private/var/mobile/Library/Preferences/XBMC/`
- `/private/var/mobile/Library/Preferences/Kodi/`

## Build Kodi and create an IPA

This section is for developers.

For day-to-day development, open the generated Xcode project, select the Kodi scheme and target device, then build and run Kodi directly from Xcode. The `ipa` target is only needed when producing an archive for sideloading or distribution.

1. Build Kodi for iOS using the normal Kodi iOS build instructions.
2. Open the generated Xcode project.
3. Select your development team and signing certificate.
4. Build the iOS `ipa` target for a real device.
5. Find the generated `.ipa` in the build directory under `tools/darwin/packaging/darwin_embedded`.

The `ipa` target is generated by CPack. It can also be built from the command line:

```sh
cd $HOME/kodi-build
xcodebuild -target ipa
```

The resulting `.ipa` can be installed with AltStore, SideStore, Sideloadly, Apple Configurator or Xcode, depending on how it was signed.

## Methods no longer recommended

Cydia Impactor is no longer recommended for free Apple accounts. The old method of converting a `.deb` package to `.ipa` by hand is also no longer recommended for normal users. Use a native `.ipa` package when one is available.
