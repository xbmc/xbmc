@echo off
if "%XSRC%" == "" set XSRC=.
test -f %XSRC%/tests/stateful-check.org
if errorlevel 1 update %XSRC%/tests/stateful-check %XSRC%/tests/stateful-check.org
sed -f %XSRC%/djgpp/stateful-check.sed %XSRC%/tests/stateful-check.org > stateful-check
if errorlevel 1 goto SedError
update ./stateful-check %XSRC%/tests/stateful-check
rm -f ./stateful-check

test -f %XSRC%/tests/stateless-check.org
if errorlevel 1 update %XSRC%/tests/stateless-check %XSRC%/tests/stateless-check.org
sed -f %XSRC%/djgpp/stateless-check.sed %XSRC%/tests/stateless-check.org > stateless-check
if errorlevel 1 goto SedError
update ./stateless-check %XSRC%/tests/stateless-check
rm -f ./stateless-check

test -f %XSRC%/tests/failuretranslit-check.org
if errorlevel 1 update %XSRC%/tests/failuretranslit-check %XSRC%/tests/failuretranslit-check.org
sed -f %XSRC%/djgpp/translit-check.sed %XSRC%/tests/failuretranslit-check.org > failuretranslit-check
if errorlevel 1 goto SedError
update ./failuretranslit-check %XSRC%/tests/failuretranslit-check
rm -f ./failuretranslit-check

test -f %XSRC%/tests/translit-check.org
if errorlevel 1 update %XSRC%/tests/translit-check %XSRC%/tests/translit-check.org
sed -f %XSRC%/djgpp/translit-check.sed %XSRC%/tests/translit-check.org > translit-check
if errorlevel 1 goto SedError
update ./translit-check %XSRC%/tests/translit-check
rm -f ./translit-check
goto End

:SedError
echo test script editing failed!

:End
