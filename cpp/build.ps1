$ErrorActionPreference = "Stop"

$ProjectRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$PortableRaylibInclude = Join-Path $ProjectRoot ".deps\raylib\include"
$PortableRaylibLib = Join-Path $ProjectRoot ".deps\raylib\lib\libraylib.a"
$PortableToolchainBin = Join-Path $ProjectRoot ".deps\w64devkit\bin"

if ($env:RAYLIB_INCLUDE -and $env:RAYLIB_LIB -and $env:RAYLIB_TOOLCHAIN_BIN) {
    $RaylibInclude = $env:RAYLIB_INCLUDE
    $RaylibLib = $env:RAYLIB_LIB
    $ToolchainBin = $env:RAYLIB_TOOLCHAIN_BIN
}
elseif ((Test-Path (Join-Path $PortableRaylibInclude "raylib.h")) -and (Test-Path $PortableRaylibLib) -and (Test-Path (Join-Path $PortableToolchainBin "mingw32-make.exe"))) {
    $RaylibInclude = $PortableRaylibInclude
    $RaylibLib = $PortableRaylibLib
    $ToolchainBin = $PortableToolchainBin
}
else {
    $RaylibRoot = if ($env:RAYLIB_ROOT) { $env:RAYLIB_ROOT } else { "C:\raylib\raylib" }
    $ToolchainBin = if ($env:RAYLIB_TOOLCHAIN_BIN) { $env:RAYLIB_TOOLCHAIN_BIN } else { "C:\raylib\w64devkit\bin" }
    $RaylibInclude = Join-Path $RaylibRoot "src"
    $RaylibLib = Join-Path $RaylibRoot "src\libraylib.a"
}

$Make = Join-Path $ToolchainBin "mingw32-make.exe"

if (-not (Test-Path (Join-Path $RaylibInclude "raylib.h")) -or -not (Test-Path $RaylibLib) -or -not (Test-Path $Make)) {
    throw @"
Raylib dependencies were not found.

Options:
  1. Install local portable dependencies:
       .\install_deps.ps1
       .\build.ps1

  2. Install the standard Raylib Windows bundle at C:\raylib.

  3. Set custom environment variables:
       `$env:RAYLIB_INCLUDE = "path\to\include"
       `$env:RAYLIB_LIB = "path\to\libraylib.a"
       `$env:RAYLIB_TOOLCHAIN_BIN = "path\to\w64devkit\bin"
"@
}

function Invoke-Checked {
    param(
        [string] $FilePath,
        [string[]] $Arguments
    )

    & $FilePath @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "Command failed with exit code ${LASTEXITCODE}: $FilePath $($Arguments -join ' ')"
    }
}

$env:PATH = "$ToolchainBin;$env:PATH"
$Targets = if ($args.Count -gt 0) { $args } else { @("all") }
$MakeArguments = @(
    "RAYLIB_INCLUDE=$($RaylibInclude -replace '\\', '/')",
    "RAYLIB_LIB=$($RaylibLib -replace '\\', '/')",
    "TOOLCHAIN_BIN=$($ToolchainBin -replace '\\', '/')"
) + $Targets

Push-Location $ProjectRoot
try {
    Invoke-Checked $Make $MakeArguments
}
finally {
    Pop-Location
}

Write-Host ""
Write-Host "Done."
Write-Host "Launcher: $ProjectRoot\build\simulations.exe"
Write-Host "Direct sims: $ProjectRoot\build\sims"
