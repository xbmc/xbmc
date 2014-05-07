@ECHO OFF

SET cur_dir=%CD%

SET base_dir=%cur_dir%\..\..
SET builddeps_dir=%cur_dir%\..\BuildDependencies
SET bin_dir=%builddeps_dir%\bin
SET msys_bin_dir=%builddeps_dir%\msys\bin
SET jsonrpc_path=%base_dir%\xbmc\interfaces\json-rpc
SET jsonrpc_schema_path=%jsonrpc_path%\schema
SET jsonrpc_output=ServiceDescription.h

SET xbmc_json_path=%base_Dir%\addons\xbmc.json
SET xbmc_json_output=addon.xml

SET /p version=<"%jsonrpc_schema_path%\version.txt"
"%msys_bin_dir%\sed.exe" s/@jsonrpc_version@/%version%/g "%xbmc_json_path%\%xbmc_json_output%.in" > "%xbmc_json_path%\%xbmc_json_output%"

"%bin_dir%\json-rpc\JsonSchemaBuilder.exe" "%jsonrpc_schema_path%\version.txt" "%jsonrpc_schema_path%\license.txt" "%jsonrpc_schema_path%\methods.json" "%jsonrpc_schema_path%\types.json" "%jsonrpc_schema_path%\notifications.json"
MOVE /Y %jsonrpc_output% "%jsonrpc_path%\%jsonrpc_output%"