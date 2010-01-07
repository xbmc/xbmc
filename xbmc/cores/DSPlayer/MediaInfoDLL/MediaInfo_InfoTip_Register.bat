@echo off
echo WARNING: Shell Extension registration is BETA (=not tested everywhere) version.
echo if there are more crash of Windows explore, UnRegister the DLL
echo and email me
echo .
echo What is new with this version?
echo if you have the mouse on a multimedia (AVI/MKV/OGG...) file, an InfoTip
echo will be showed
pause
regsvr32 MediaInfo_InfoTip.dll