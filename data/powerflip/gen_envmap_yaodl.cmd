@echo off
rem gen_envmap_yaodl.cmd - Windows version of gen_envmap_yaodl.sh
rem
rem Takes a YAODL file and an environment map image and writes a new YAODL
rem file: the original one with the environment map applied.
rem
rem   gen_envmap_yaodl.cmd {LINEAR^|CONTOUR^|SPHEREMAP} model.ydl texture.rgb output.ydl
rem
rem The binary .ydl files cannot go through the console as with "cat": the
rem output is a file, made with "copy /b" of the start, the model and the end.
rem The texture is searched by powerflip in the current directory, then in
rem the directory of the output file.
setlocal

if "%~4"=="" goto usage
set "GEN_TYPE=%~1"
set "YAODL=%~2"
set "TEXTURE=%~3"
set "OUTPUT=%~4"

if not exist "%YAODL%" goto nomodel
if /i "%GEN_TYPE%"=="LINEAR" goto start
if /i "%GEN_TYPE%"=="CONTOUR" goto start
if /i "%GEN_TYPE%"=="SPHEREMAP" goto start
echo %~nx0: One of {LINEAR^|CONTOUR^|SPHEREMAP} expected as first argument 1>&2
exit /b 1

:start
set "HEAD=%TEMP%\envmap_head_%RANDOM%.txt"
set "TAIL=%TEMP%\envmap_tail_%RANDOM%.txt"
> "%HEAD%" echo (group
> "%TAIL%" echo   :
>> "%TAIL%" echo   ^(textures "%TEXTURE%"^),
goto %GEN_TYPE%

:LINEAR
>> "%TAIL%" echo   ^(sContours "linear", "object", 10.0 0.0 10.0 0.0^),
>> "%TAIL%" echo   ^(tContours "linear", "object", -7.0 7.0 7.0 0.0^),
goto finish

:CONTOUR
>> "%TAIL%" echo   ^(sContours "contour", "eye", 5.0 0.0 5.0 0.0^),
>> "%TAIL%" echo   ^(tContours "contour", "eye", 0.0 5.0 -5.0 0.0^),
goto finish

:SPHEREMAP
>> "%TAIL%" echo   ^(sContours "spheremap", "object"^),
>> "%TAIL%" echo   ^(tContours "spheremap", "object"^),
goto finish

:finish
>> "%TAIL%" echo )
copy /b "%HEAD%" + "%YAODL%" + "%TAIL%" "%OUTPUT%" > nul
set "RESULT=%errorlevel%"
del "%HEAD%" "%TAIL%"
exit /b %RESULT%

:nomodel
echo %~nx0: can't find %YAODL% 1>&2
exit /b 1

:usage
echo Usage: %~nx0 {LINEAR^|CONTOUR^|SPHEREMAP} ^<Yaodl file^> ^<texture file^> ^<output file^> 1>&2
exit /b 1
