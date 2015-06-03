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

$ADDON_GUILIB = get-content "$GENERATED_ADDON_GUILIB.in"
foreach ($i in 0 .. ($ADDON_GUILIB.Length -1)) {
    $ADDON_GUILIB[$i] = $ADDON_GUILIB[$i].Replace("@guilib_version@", $LIB_VERSION).Replace("@guilib_version_min@", $LIB_VERSION_MIN)
}

#WriteAllLines does not overwrite so remove the existing file here.
del $GENERATED_ADDON_GUILIB -Force

#create utf8 encoding without bom
$U8 = New-Object System.Text.UTF8Encoding($False)
[System.IO.File]::WriteAllLines($GENERATED_ADDON_GUILIB, $ADDON_GUILIB, $U8)
