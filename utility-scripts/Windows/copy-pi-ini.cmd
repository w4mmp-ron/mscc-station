@echo off
rem Copy the safe radio settings from the Pi to the Windows servers (see copy-pi-ini.ps1).
rem   copy-pi-ini            copy (servers must be stopped; backs up first)
rem   copy-pi-ini -WhatIf    show what would be copied
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0copy-pi-ini.ps1" %*
