@echo off
setlocal

for /F "tokens=1 delims=\ " %%a in ('call git rev-parse --short HEAD') do (
  set GitRevision=%%a
)
echo #define GIT_REV "%GitRevision%" > git_rev.h

endlocal