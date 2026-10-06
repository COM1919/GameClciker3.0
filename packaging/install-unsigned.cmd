@echo off
rem Double-click to install / update the unsigned GC MSIX package for local testing.
rem Optional: install-unsigned.cmd -PackagePath "path\to\xxx.msix"  -KeepData
setlocal
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0install-unsigned.ps1" %*
set EXITCODE=%ERRORLEVEL%
endlocal & exit /b %EXITCODE%
