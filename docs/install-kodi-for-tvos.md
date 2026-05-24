# HOW-TO: Install Kodi on Apple TV HD and 4K

This page is for Apple TV HD and Apple TV 4K models that run tvOS.

The current stable release for tvOS is Kodi v21.3 Omega.

## Requirements

To install Kodi on Apple TV you must have one of the following:

- an Apple TV HD or Apple TV 4K and a way to sideload a Kodi tvOS `.ipa` file
- a Mac with Xcode or Apple Configurator
- a jailbroken Apple TV and a way to install a Kodi tvOS `.deb` file

Apple TV HD can be connected to a computer with USB-C. Apple TV 4K does not have USB-C, so normal sideloading is done over the local network after pairing the Apple TV with the computer.

## Which install method should I use?

Most users should use a Kodi tvOS `.ipa` file and one of the non-jailbroken methods below.

| Method | Best for | Notes |
| --- | --- | --- |
| Sideloadly | Most Windows or macOS users | Supports Apple TV and can install tvOS `.ipa` files. |
| AltServer direct IPA sideloading | macOS users who already use AltStore tools | AltStore itself is not installed on Apple TV. Use AltServer from the Mac. |
| Xcode | Developers or Mac users | Good for pairing Apple TV 4K and installing a signed app. |
| Apple Configurator | Managed or Mac-based installs | Can install apps on Apple TV when the app is already signed correctly. |
| Jailbroken Apple TV | Jailbroken devices only | Uses the Kodi tvOS `.deb` package. |

If you use a free Apple developer account, Kodi normally has to be re-signed and re-installed or refreshed every 7 days.

## Download Kodi package

Some methods use a Kodi tvOS `.ipa` file. Jailbroken Apple TVs use a Kodi tvOS `.deb` file.

- Latest stable Kodi package: <https://kodi.tv/download/tvos/>
- Stable and older tvOS builds: <https://mirrors.kodi.tv/releases/darwin/>
- Nightly builds: <https://mirrors.kodi.tv/nightlies/darwin/>

Note: Development and nightly builds are test builds. ALWAYS BACKUP YOUR USERDATA FOLDER WHEN USING DEVELOPMENT BUILDS!

## Pair Apple TV with your computer

Apple TV 4K must be paired over the network before most install tools can see it.

1. Make sure the Apple TV and your computer are on the same network.
2. On the Apple TV, open Settings.
3. Open Remotes and Devices.
4. Open Remote App and Devices.
5. Keep the Apple TV on this screen.
6. On your Mac, open Xcode.
7. Open Window -> Devices and Simulators.
8. Select your Apple TV and click Pair.
9. Enter the code shown on the Apple TV.

Your Apple TV is now paired with your Mac. Some tools, including Sideloadly, can also detect Apple TV when it is on the Remote App and Devices screen.

## Non-jailbroken Apple TV

### Option A: Install using Sideloadly

This method works from Windows or macOS.

Make sure you have the following software:

- Sideloadly: <https://sideloadly.io/>
- A Kodi tvOS `.ipa` file

Procedure, step by step:

1. Turn on your Apple TV and make sure it is on the same network as your computer.
2. If you have an Apple TV HD and want to use USB-C, connect it to your computer.
3. If you have an Apple TV 4K, open Settings -> Remotes and Devices -> Remote App and Devices and keep the Apple TV on that screen.
4. Open Sideloadly.
5. Select your Apple TV when it appears.
6. Drag the Kodi tvOS `.ipa` file into Sideloadly.
7. Enter the Apple account you want to use for signing.
8. Press Start and wait until the installation is done.
9. Kodi should appear on your Apple TV main screen.

Note: Apps installed with a free Apple account expire after 7 days. Sideloadly can enroll apps for automatic refresh, but your Apple TV and computer must be available for refresh.

### Option B: Install using AltServer direct IPA sideloading

AltStore is not installed on Apple TV. On tvOS, AltServer on macOS can sideload a tvOS `.ipa` directly to Apple TV.

1. Install AltServer on your Mac.
2. Pair your Apple TV with the Mac.
3. Download the Kodi tvOS `.ipa` file to your Mac.
4. Use AltServer's direct IPA sideloading option.
5. Select your Apple TV.
6. Select the Kodi tvOS `.ipa` file.
7. Wait for the installation to complete.
8. Kodi should appear on your Apple TV main screen.

Note: Apps installed with a free Apple account expire after 7 days and must be refreshed.

### Option C: Install using Xcode

This method is for users who want to install a signed Kodi build from a Mac.

1. Install Xcode from the Mac App Store.
2. Open Xcode.
3. Add your Apple account in Xcode -> Settings -> Accounts.
4. Pair your Apple TV with Xcode.
5. Build Kodi for tvOS or use a Kodi tvOS `.ipa` signed with your Apple account.
6. Open Window -> Devices and Simulators.
7. Select your Apple TV.
8. Press the `+` button in the Installed Apps section.
9. Select the Kodi tvOS `.ipa`.
10. Wait until Kodi appears on your Apple TV main screen.

### Option D: Install using Apple Configurator

Apple Configurator for Mac can install apps on Apple TV when the app is signed correctly.

1. Install Apple Configurator from the Mac App Store.
2. Pair or connect your Apple TV.
3. Open Apple Configurator.
4. Select your Apple TV.
5. Add the Kodi tvOS `.ipa` file.
6. Wait for the installation to complete.

If the app is not signed for your Apple TV, Apple Configurator will not be able to install it.

## Jailbroken Apple TV

If your Apple TV is not jailbroken, please follow one of the non-jailbroken methods above.

Kodi tvOS `.deb` packages are for jailbroken Apple TVs. Use the package manager or jailbreak tools that are recommended for your jailbreak.

### Option A: Install from a package manager

1. Open your jailbreak package manager.
2. Refresh sources.
3. Search for Kodi.
4. Select the Kodi package for tvOS.
5. Install the package.
6. Respring or restart the Apple TV if prompted.

### Option B: Install a `.deb` file manually

1. Download the Kodi tvOS `.deb` file from the Kodi mirror.
2. Copy it to the Apple TV with `scp`.
3. Connect to the Apple TV with `ssh`.
4. Install the package with `dpkg -i`.
5. Refresh the icon cache with `uicache`.
6. Kodi should appear on your Apple TV main screen.

Example commands:

```sh
scp kodi-tvos.deb root@APPLE_TV_IP:/var/root/
ssh root@APPLE_TV_IP
dpkg -i /var/root/kodi-tvos.deb
uicache
```

Change `APPLE_TV_IP` and the `.deb` filename to match your Apple TV and downloaded package.

## Build Kodi and create a tvOS IPA

This section is for developers.

1. Build Kodi for tvOS using the normal Kodi tvOS build instructions.
2. Open the generated Xcode project.
3. Select your development team and signing certificate.
4. Build the tvOS `ipa` target for a real Apple TV.
5. Find the generated `.ipa` in the Darwin embedded packaging output directory.

The resulting `.ipa` can be installed with Sideloadly, AltServer direct IPA sideloading, Apple Configurator or Xcode, depending on how it was signed.

## Methods no longer recommended

Cydia Impactor is no longer recommended for free Apple accounts. The old method of converting a `.deb` package to `.ipa` by hand is also no longer recommended for normal users. Use a native tvOS `.ipa` package when one is available.
