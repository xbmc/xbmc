![Kodi Logo](resources/banner_slim.png)

# Tizen SDK setup for Samsung TV
This guide covers everything that has to be in place **before** a Kodi WASM build can be packaged, signed and installed on a Samsung TV: the Tizen SDK, a paired TV, and the Samsung certificates a retail TV insists on.

Building Kodi itself is covered by the **[WebAssembly (WASM) build guide](README.WASM.md)**, which also documents the `package_tizen`, `package_tizen_wgt` and `install_tizen_wgt` build targets. Do the setup here once, then follow that guide for every build.

This guide has been tested on **macOS** against a **Samsung S95F** running **Tizen 9.0**, with the **Tizen 10.0 SDK** (`tz` 10.0.3). Linux hosts work the same way apart from the installer and the Certificate Manager launcher path.

## Table of Contents
1. **[Document conventions](#1-document-conventions)**
2. **[Prerequisites](#2-prerequisites)**
3. **[Install the Tizen SDK](#3-install-the-tizen-sdk)**
  3.1. **[Install Tizen Studio](#31-install-tizen-studio)**
  3.2. **[Put the CLI tools on PATH](#32-put-the-cli-tools-on-path)**
  3.3. **[Install the TV extension and Certificate Manager](#33-install-the-tv-extension-and-certificate-manager)**
4. **[Enable Developer Mode on the TV](#4-enable-developer-mode-on-the-tv)**
5. **[Connect to the TV with sdb](#5-connect-to-the-tv-with-sdb)**
  5.1. **[Find the TV IP address](#51-find-the-tv-ip-address)**
  5.2. **[Pair the host with the TV](#52-pair-the-host-with-the-tv)**
6. **[Create the signing certificates](#6-create-the-signing-certificates)**
  6.1. **[Read the TV DUID](#61-read-the-tv-duid)**
  6.2. **[Run the Certificate Manager wizard](#62-run-the-certificate-manager-wizard)**
  6.3. **[Verify the active profile](#63-verify-the-active-profile)**
7. **[Troubleshooting](#7-troubleshooting)**

## 1. Document conventions
The conventions used here are the same as in the **[WASM build guide](README.WASM.md#1-document-conventions)**: commands are run in a terminal one at a time, and strings in angle brackets (`<tv-ip>`) are placeholders you replace.

This guide uses `$HOME/tizen-studio` as the SDK location, which is the installer default.

**[back to top](#table-of-contents)**

## 2. Prerequisites
* A **Samsung TV** running Tizen, on the same network as the build host, with a wired or wireless connection you can reach by IP.
* A **[Samsung account](https://account.samsung.com/)**. The distributor certificate is issued by Samsung and cannot be created offline.
* A **JDK**. The Tizen package manager refuses to run without one; any recent OpenJDK works (`brew install openjdk` on macOS).

> [!NOTE]
> A retail TV only runs packages signed with a Samsung distributor certificate that names the TV's own device ID (DUID). The Tizen SDK's built-in distributor certificate is rejected, so a Samsung account is not optional — hence sections **[5](#5-connect-to-the-tv-with-sdb)** and **[6](#6-create-the-signing-certificates)**.

**[back to top](#table-of-contents)**

## 3. Install the Tizen SDK

### 3.1. Install Tizen Studio
Download the installer from **[samsungtizenos.com/tools-download](https://samsungtizenos.com/tools-download/)** (the old `developer.tizen.org` download page now redirects there). Pick **Tizen SDK with CLI**, which is enough for everything in this guide; the full IDE also works.

> [!WARNING]
> The download page has a **Mirror Location** selector, and the mirror you pick is baked into the installation — every later package fetch goes through it. At the time of writing the **USA** mirror (`usa.sdk-dl.tizen.org`) answered `403` on every path, while **Brazil** (`brazil.sdk-dl.tizen.org`) and Origin (`download.tizen.org`) worked. If package downloads fail, switch mirrors rather than retrying.

Check which mirror an existing installation uses:
```
$HOME/tizen-studio/package-manager/package-manager-cli.bin show-info
```

### 3.2. Put the CLI tools on PATH
Two binaries matter, and they live in different directories: `sdb` (device connection) in `tools/`, and `tz` (the tizen-core CLI that packages, signs and installs) in `tools/tizen-core/`. Nothing is added to `PATH` by the installer.

Add both to your shell startup file:
```
export TIZEN_SDK="$HOME/tizen-studio"
export PATH="$TIZEN_SDK/tools:$TIZEN_SDK/tools/tizen-core:$PATH"
```

> [!TIP]
> Exporting `TIZEN_SDK` is not just for your own convenience: Kodi's CMake looks for `tz` under `TIZEN_CLI_PATH`, `TIZEN_TOOLS_PATH`, `TIZEN_SDK` and `TIZEN_SDK_ROOT` before falling back to `PATH`.

Verify:
```
tz --version
sdb version
```

### 3.3. Install the TV extension and Certificate Manager
A stock install has neither the Samsung TV profile nor a way to create Samsung certificates. Add them with the package manager CLI:
```
$HOME/tizen-studio/package-manager/package-manager-cli.bin install --accept-license \
  TV-SAMSUNG-Public,Certificate-Manager,cert-add-on
```

| Package | Why it is needed |
|---|---|
| `TV-SAMSUNG-Public` | The `tv-samsung` profile that `config.xml` and `tizen_web_project.yaml` target |
| `Certificate-Manager` | Standalone Certificate Manager application |
| `cert-add-on` | Samsung Certificate Extension |

These come from extension repositories rather than the base SDK. They are active by default; list them with `package-manager-cli.bin extra --list --detail` if an install cannot find a package.

> [!WARNING]
> `Certificate-Manager` and `cert-add-on` are **not** interchangeable. The Samsung Certificate Extension on its own only installs an Eclipse plugin, which is unusable in a CLI-only installation — the standalone application is what section **[6](#6-create-the-signing-certificates)** uses.

Confirm what is installed:
```
$HOME/tizen-studio/package-manager/package-manager-cli.bin show-pkgs --installed
```

**[back to top](#table-of-contents)** | **[back to section top](#3-install-the-tizen-sdk)**

## 4. Enable Developer Mode on the TV
On the TV:

1. Open **Apps** from the home screen.
2. Type `12345` on the remote. A **Developer mode** dialog appears.
3. Switch **Developer mode** to **On**.
4. Enter the **IP address of the build host** — not the TV's own address.
5. Restart the TV.

Find the host address to enter (macOS; replace `en0` with the interface actually in use):
```
ipconfig getifaddr en0
```

> [!WARNING]
> The host IP entered here is an allowlist of exactly one machine. `sdb connect` fails with a bare "failed to connect" if the host you connect from is not the one the TV has stored — including after your router hands the host a different DHCP lease.

**[back to top](#table-of-contents)**

## 5. Connect to the TV with sdb

### 5.1. Find the TV IP address
Samsung TVs advertise themselves over mDNS, so the address can be discovered without touching the TV's menus (macOS):
```
dns-sd -B _samsungmsf._tcp
```

Resolve the instance name that appears — a `uuid:...` string — to get the address:
```
dns-sd -L "<instance-name>" _samsungmsf._tcp
```

The address is in the `se=` field of the TXT record, for example `se=http://192.168.1.51:8001/api/v2/`.

That URL is an unauthenticated device-info endpoint, useful both for confirming you have the right TV and for reading back what it thinks the developer host is:
```
curl -s http://<tv-ip>:8001/api/v2/ | python3 -m json.tool
```

Interesting fields in the response:

| Field | Meaning |
|---|---|
| `device.developerMode` | `"1"` once Developer Mode is on |
| `device.developerIP` | The host IP the TV will accept `sdb` connections from |
| `device.model`, `device.name` | Confirms which TV answered |

> [!WARNING]
> The `device.duid` field here is the TV's mDNS UUID, **not** the DUID the distributor certificate needs. Read that one over `sdb` as shown in **[6.1](#61-read-the-tv-duid)**.

### 5.2. Pair the host with the TV
```
sdb connect <tv-ip>
sdb devices
```

A paired TV shows up as `<tv-ip>:26101` with a device name. If `sdb devices` is empty, work through **[7. Troubleshooting](#7-troubleshooting)** before going any further — nothing after this point works without a connected device.

**[back to top](#table-of-contents)** | **[back to section top](#5-connect-to-the-tv-with-sdb)**

## 6. Create the signing certificates
Signing needs two certificates: an **author** certificate you generate locally, and a **distributor** certificate issued by Samsung and bound to the DUIDs of the TVs you want to install on.

### 6.1. Read the TV DUID
With the TV connected:
```
sdb shell 0 getduid
```

The value is a short alphanumeric string with no separators.

### 6.2. Run the Certificate Manager wizard
Launch the standalone application (macOS):
```
open $HOME/tizen-studio/tools/certificate-manager/Certificate-manager.app
```

In the wizard:

1. Choose **Samsung** as the certificate type, then **TV** as the device type.
2. Choose a **profile name** — this becomes the directory under `$HOME/SamsungCertificate/` and the name of the `tz` signing profile.
3. **Create a new author certificate**. The password must be **at least 8 characters**.
4. Sign in with your Samsung account when prompted.
5. For the distributor certificate, pick **Public** privilege. Kodi does not use any partner-level API, and Partner privilege requires a seller account.
6. Select the connected TV so the DUID is filled in automatically, or paste the DUID from **[6.1](#61-read-the-tv-duid)**.

The wizard writes `author.p12` and `distributor.p12` (plus their `.pwd` files) to `$HOME/SamsungCertificate/<profile>/`, registers them as a `tz` signing profile, and makes that profile active.

> [!NOTE]
> A Public distributor certificate is bound to the listed DUIDs and nothing else — it places no constraint on the application or package ID, so Kodi's own IDs in `tools/wasm/tizen/config.xml` need no adjustment. Installing on a second TV does require re-running the wizard with that TV's DUID added.

### 6.3. Verify the active profile
```
tz security-profiles list
```

Expected output for a profile named `kodi-samsung`:
```
Current Active Profile: kodi-samsung

kodi-samsung
  Author      : /Users/<user>/SamsungCertificate/kodi-samsung/author.p12
  Distributor : /Users/<user>/SamsungCertificate/kodi-samsung/distributor.p12
  Distributor2:
```

Kodi's `tizen_web_project.yaml` deliberately leaves `signing_profile` empty, which means `tz` signs with whichever profile is active here. If the active profile is a plain Tizen one, packaging succeeds but installation fails — see **[7. Troubleshooting](#7-troubleshooting)**.

Certificates created outside the wizard can be registered manually:
```
tz security-profiles add -n <profile> -A \
  -a <author.p12> -p <author-password> \
  -d <distributor.p12> -P <distributor-password>
```

**[back to top](#table-of-contents)** | **[back to section top](#6-create-the-signing-certificates)**

## 7. Troubleshooting

**`sdb connect` prints "failed to connect", but port 26101 is open**
The TV's stored developer host does not match the machine you are connecting from. Compare `device.developerIP` from `curl -s http://<tv-ip>:8001/api/v2/` against `ipconfig getifaddr en0` and correct it in the TV's Developer mode dialog (**[4](#4-enable-developer-mode-on-the-tv)**).

**`tz install` fails at ~28% with `install failed[118, -12] ... Invalid certificate chain with certificate in signature`**
The package was signed with the SDK's built-in Tizen distributor certificate. Create a Samsung distributor certificate (**[6](#6-create-the-signing-certificates)**), make its profile active, and build the package again — signing happens while packaging, so an existing `.wgt` keeps the old signature.

**`package-manager-cli.bin` refuses with "Tizen Studio programs are running under installed location"**
The GUI Package Manager (or Device Manager) holds a lock on the install directory. Quit it and rerun.

**Certificate Manager will not start: "the application ... is missing"**
Only `cert-add-on` is installed. It is an Eclipse plugin with no launcher of its own; install the `Certificate-Manager` package as shown in **[3.3](#33-install-the-tv-extension-and-certificate-manager)**.

**`tz cert` rejects the password: "the length must be no less than 8"**
Certificate passwords have an 8 character minimum.

**`sdb shell` closes immediately, `sdb dlog` prints nothing**
Expected on retail firmware. Only the `sdb shell 0 <command>` forms (`getduid`, `runningapp`, ...) are available, and application logs are not exposed. Use `tz run -d` to attach the Web Inspector instead, which gives real console output from the WASM runtime.

**[back to top](#table-of-contents)**
