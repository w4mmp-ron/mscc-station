@echo off
REM Post-build wrapper next to .cyprj (${ProjDir} = this .cydsn folder).
REM Invoked as:  cmd /c "${ProjDir}\post-build-release.bat"
"%~dp0..\copy-release.bat"
exit /b %ERRORLEVEL%
