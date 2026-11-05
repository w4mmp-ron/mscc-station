@echo off
rem This program receives as input two arguments:
rem Arg1 is the name of the image (including directory)
rem Arg2 is the extension of the image file
rem In other words, this program is expected to be called from a
rem post bild step of "copyWithDate ${ImageDir} ${ImageName}"
rem This program produces one output:
rem It renames the ImageName to be the value of the date

if "%1"=="" goto bad
if "%2"=="" goto bad
if "%3"=="" goto bad
if "%4"=="" goto bad
set dateWithSlashes=%date:~4,13%
rem remove the / chars
set dateNoSlashes=%dateWithSlashes:/=_%
echo copying %1 to %4.%2.%dateNoSlashes%.%3
copy %1 %4.%2.%dateNoSlashes%.%3
goto end
:bad
echo Expected to be called as in post build step:  "copyWithDate ${ImageDir} ${ImageName}"
:end

