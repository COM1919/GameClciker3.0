@echo off
rem 双击即可打包。也可在命令行传参，例如：
rem   build-msix.cmd -Version 1.0.1.0
rem   build-msix.cmd -SkipSign
setlocal
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0build-msix.ps1" %*
set EXITCODE=%ERRORLEVEL%
endlocal & exit /b %EXITCODE%
