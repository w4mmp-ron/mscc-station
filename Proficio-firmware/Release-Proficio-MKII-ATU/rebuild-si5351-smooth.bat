@echo off
REM Incremental rebuild of Si5351 smooth-tune sources for Proficio-MKII-ATU.
REM Run from a machine with licensed Keil C51 (as used by PSoC Creator).
REM Prefer: open Proficio-MKII-ATU.cywrk in PSoC Creator and Build there.
REM This script only recompiles si5351.c + si5351a.c and relinks.

setlocal
cd /d "%~dp0Proficio-MKII-ATU.cydsn"
if not exist si5351.c (
  echo ERROR: si5351.c not found. Run this from Release-Proficio-MKII-ATU folder layout.
  exit /b 1
)

where C51.exe >nul 2>&1
if errorlevel 1 (
  echo ERROR: C51.exe not on PATH. Open "PSoC Creator Command Prompt" or add Keil C51\BIN.
  exit /b 1
)

set DBG=.\DP8051\DP8051_Keil_951\Debug

echo === Compiling si5351.c ===
C51.exe si5351.c NOIV LARGE MODDP2 OMF2 VB(1) NOIP "INCDIR(., Generated_Source\PSoC3)" FF(3) DB WL(2) PR(%DBG%/si5351.lst) CD OT(5 ,Speed) OA OJ(%DBG%\si5351.obj)
if errorlevel 2 goto fail

echo === Compiling si5351a.c ===
C51.exe si5351a.c NOIV LARGE MODDP2 OMF2 VB(1) NOIP "INCDIR(., Generated_Source\PSoC3)" FF(3) DB WL(2) PR(%DBG%/si5351a.lst) CD OT(5 ,Speed) OA OJ(%DBG%\si5351a.obj)
if errorlevel 2 goto fail

if not exist %DBG%\si5351.obj (
  echo ERROR: si5351.obj not produced. Check Keil license ^(C500 LIC warning^).
  exit /b 1
)

echo === Linking via keilLinkCmds.lnp ===
LX51.exe @%DBG%\keilLinkCmds.lnp
if errorlevel 1 goto fail

echo === OHx51 ihx ===
pushd %DBG%
OHx51.exe Proficio-MKII-ATU.omf HEXFILE(Proficio-MKII-ATU.ihx)
popd

echo.
echo Done. Next: run CyHexTool / PSoC Creator post-link to produce .cyacd or .hex
echo   Objects: %DBG%\si5351.obj  %DBG%\si5351a.obj
echo   OMF/IHX: %DBG%\Proficio-MKII-ATU.omf  %DBG%\Proficio-MKII-ATU.ihx
echo Verify listing contains MIN_SMOOTH_HZ in %DBG%\si5351.lst
exit /b 0

:fail
echo BUILD FAILED
exit /b 1
