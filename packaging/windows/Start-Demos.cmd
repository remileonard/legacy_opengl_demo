@echo off
setlocal
pushd "%~dp0"
if errorlevel 1 exit /b 1
buttonfly.exe menu
set "result=%errorlevel%"
popd
exit /b %result%
