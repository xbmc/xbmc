param (
[string]$ProjectDir
)

$LIB_INTERFACE = "$ProjectDir\..\..\..\..\..\addons\library.kodi.guilib\libKODI_guilib.h"
$GENERATED_ADDON_GUILIB = "$ProjectDir\..\..\..\..\..\addons\kodi.guilib\addon.xml"

$LIB_VERSION = ""
$LIB_VERSION_MIN = ""
cat $LIB_INTERFACE | %{
if ($_ -match 'KODI_GUILIB_API_VERSION\s*"(.*)"'){
    $LIB_VERSION = $matches[1]
    }
if ($_ -match 'KODI_GUILIB_MIN_API_VERSION\s*"(.*)"'){
    $LIB_VERSION_MIN = $matches[1]
    }
}

$ADDON_GUILIB = $GENERATED_ADDON_GUILIB + ".in"
cat $ADDON_GUILIB | % { $_ -replace "@guilib_version@", $LIB_VERSION `
                           -replace "@guilib_version_min@", $LIB_VERSION_MIN} > $GENERATED_ADDON_GUILIB