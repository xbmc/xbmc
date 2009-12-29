@ECHO OFF
SET target=none
FOR %%b in (%1, %2) DO (
	IF %%b==revision SET target=revision
	IF %%b==branch SET target=branch
)

IF %target%==branch (
  FOR /f "tokens=2* delims=]" %%a in ('find /v /n "&_&_&_&" ".svn\entries" ^| FIND  "[5]"') DO ECHO %%~Na
) ELSE (
  IF %target%==revision (FOR /F "Tokens=2* Delims=]" %%R IN ('FIND /v /n "&_&_&_&" ".svn\entries" ^| FIND "[11]"') DO ECHO %%R)
)