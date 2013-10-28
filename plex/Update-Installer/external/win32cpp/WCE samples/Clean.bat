
::A batch file to remove unnecessary files from
:: each Visual Studio project

::Remove directories
FOR /D %%f IN ("*.") DO RMDIR /S /Q %%f\emulatorDbg
FOR /D %%f IN ("*.") DO RMDIR /S /Q %%f\emulatorRel
FOR /D %%f IN ("*.") DO RMDIR /S /Q %%f\ARMV4Dbg
FOR /D %%f IN ("*.") DO RMDIR /S /Q %%f\ARMV4Rel
FOR /D %%f IN ("*.") DO RMDIR /S /Q %%f\X86Dbg
FOR /D %%f IN ("*.") DO RMDIR /S /Q %%f\X86Rel
FOR /D %%f IN ("*.") DO RMDIR /S /Q %%f\_UpgradeReport_Files
FOR /D %%f IN ("*.") DO RMDIR /S /Q %%f\"Pocket PC 2003 (ARMV4)"
FOR /D %%f IN ("*.") DO RMDIR /S /Q %%f\"Smartphone 2003 (ARMV4)"
FOR /D %%f IN ("*.") DO RMDIR /S /Q %%f\"Windows Mobile 5.0 Pocket PC SDK (ARMV4I)"
FOR /D %%f IN ("*.") DO RMDIR /S /Q %%f\"Windows Mobile 5.0 Smartphone SDK (ARMV4I)"

::Remove files
FOR /D %%f IN ("*.") DO DEL /Q %%f\"*.vcb"
FOR /D %%f IN ("*.") DO DEL /Q %%f\"*.vcl"
FOR /D %%f IN ("*.") DO DEL /Q %%f\"*.vco"
FOR /D %%f IN ("*.") DO DEL /Q %%f\"*.aps"
FOR /D %%f IN ("*.") DO DEL /Q %%f\"*.XML"
FOR /D %%f IN ("*.") DO DEL /Q %%f\"*.user"
FOR /D %%f IN ("*.") DO DEL /Q %%f\"*.ncb"
FOR /D %%f IN ("*.") DO DEL /Q %%f\"*.bak"
FOR /D %%f IN ("*.") DO DEL /Q /AH %%f\"*.suo"



