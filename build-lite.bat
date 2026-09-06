@echo off
setlocal
cd /d "%~dp0"

utils\bin\premake5 --file=premake-lite.lua vs2022 || exit /b 1

set "MSBUILD=%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe"
if not exist "%MSBUILD%" (
  for /f "usebackq tokens=*" %%I in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe`) do set "MSBUILD=%%I"
)
if not exist "%MSBUILD%" (
  echo MSBuild.exe not found.
  exit /b 2
)

"%MSBUILD%" build-lite\SCSP-Localify-Lite.sln /m /p:Configuration=Release /p:Platform=x64 /v:minimal
exit /b %ERRORLEVEL%
