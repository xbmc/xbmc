@ECHO OFF

SET cur_dir=%CD%

SET base_dir=%cur_dir%\..\..
SET bin_dir=%cur_dir%\..\BuildDependencies\bin\json-rpc
SET jsonrpc_path=%base_dir%\xbmc\interfaces\json-rpc
SET jsonrpc_schema_path=%jsonrpc_path%\schema
SET output=ServiceDescription.h

"%bin_dir%\JsonSchemaBuilder.exe" "%jsonrpc_schema_path%\version.txt" "%jsonrpc_schema_path%\license.txt" "%jsonrpc_schema_path%\methods.json" "%jsonrpc_schema_path%\types.json" "%jsonrpc_schema_path%\notifications.json"
MOVE /Y %output% "%jsonrpc_path%\%output%"