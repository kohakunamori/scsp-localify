param(
    [string]$Plugin = ""
)

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $PSScriptRoot
if (-not $Plugin) {
    $Plugin = Join-Path $Root "build-full-2.17\bin\x64\Release\scsp_localify_plugin.dll"
}
$Plugin = [IO.Path]::GetFullPath($Plugin)
if (-not (Test-Path -LiteralPath $Plugin -PathType Leaf)) {
    throw "Plugin DLL not found: $Plugin"
}

$VsWhere = "C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path $VsWhere)) {
    throw "vswhere.exe not found"
}
$VcVars = & $VsWhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -find "VC\Auxiliary\Build\vcvars64.bat" | Select-Object -First 1
if (-not $VcVars) {
    throw "vcvars64.bat not found"
}

$BuildDir = Join-Path $Root "build-full-2.17\smoke"
New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null
$Exe = Join-Path $BuildDir "full_plugin_non_game_smoke.exe"
$Source = Join-Path $PSScriptRoot "full_plugin_non_game_smoke.cpp"

$Command = 'call "{0}" >nul && cl /nologo /EHsc /std:c++20 /O2 /Fe:"{1}" "{2}"' -f $VcVars, $Exe, $Source
cmd.exe /d /s /c $Command
if ($LASTEXITCODE -ne 0) {
    throw "Smoke host compilation failed: $LASTEXITCODE"
}

& $Exe $Plugin
exit $LASTEXITCODE
