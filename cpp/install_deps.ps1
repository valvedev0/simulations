param(
    [switch] $Force
)

$ErrorActionPreference = "Stop"

$ProjectRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$DepsRoot = Join-Path $ProjectRoot ".deps"
$DownloadsDir = Join-Path $DepsRoot "downloads"
$RaylibDir = Join-Path $DepsRoot "raylib"
$ToolchainDir = Join-Path $DepsRoot "w64devkit"
$RaylibVersion = "5.5"

function New-CleanDirectory {
    param([string] $Path)

    if (Test-Path $Path) {
        Remove-Item -Recurse -Force $Path
    }
    New-Item -ItemType Directory -Force $Path | Out-Null
}

function Download-File {
    param(
        [string] $Url,
        [string] $OutFile
    )

    Write-Host "Downloading $Url"
    Invoke-WebRequest -Uri $Url -OutFile $OutFile
}

function Get-GithubAssetUrl {
    param(
        [string] $ApiUrl,
        [string] $Pattern
    )

    $release = Invoke-RestMethod -Uri $ApiUrl -Headers @{ "User-Agent" = "raylib-simulations-setup" }
    $asset = $release.assets | Where-Object { $_.name -like $Pattern } | Select-Object -First 1
    if (-not $asset) {
        throw "Could not find GitHub release asset matching '$Pattern' at $ApiUrl"
    }
    return $asset.browser_download_url
}

function Copy-RaylibPackage {
    param([string] $ExtractedDir)

    $raylibHeader = Get-ChildItem -Path $ExtractedDir -Recurse -Filter "raylib.h" | Select-Object -First 1
    $raylibLibrary = Get-ChildItem -Path $ExtractedDir -Recurse -Filter "libraylib.a" | Select-Object -First 1

    if (-not $raylibHeader -or -not $raylibLibrary) {
        throw "Downloaded raylib package did not contain raylib.h and libraylib.a"
    }

    New-CleanDirectory $RaylibDir
    $includeDir = Join-Path $RaylibDir "include"
    $libDir = Join-Path $RaylibDir "lib"
    New-Item -ItemType Directory -Force $includeDir, $libDir | Out-Null

    Copy-Item -Force (Join-Path $raylibHeader.DirectoryName "*.h") $includeDir
    Copy-Item -Force $raylibLibrary.FullName $libDir
}

function Copy-W64DevkitPackage {
    param([string] $ExtractedDir)

    $make = Get-ChildItem -Path $ExtractedDir -Recurse -Filter "mingw32-make.exe" | Select-Object -First 1
    $compiler = Get-ChildItem -Path $ExtractedDir -Recurse -Filter "g++.exe" | Select-Object -First 1

    if (-not $make -or -not $compiler) {
        throw "Downloaded w64devkit package did not contain mingw32-make.exe and g++.exe"
    }

    $binDir = $make.Directory
    $root = $binDir.Parent

    New-CleanDirectory $ToolchainDir
    Copy-Item -Recurse -Force (Join-Path $root.FullName "*") $ToolchainDir
}

$existingRaylib = (Test-Path (Join-Path $RaylibDir "include\raylib.h")) -and (Test-Path (Join-Path $RaylibDir "lib\libraylib.a"))
$existingToolchain = (Test-Path (Join-Path $ToolchainDir "bin\g++.exe")) -and (Test-Path (Join-Path $ToolchainDir "bin\mingw32-make.exe"))

if ($existingRaylib -and $existingToolchain -and -not $Force) {
    Write-Host "Dependencies already exist in .deps. Use .\install_deps.ps1 -Force to reinstall."
    exit 0
}

New-Item -ItemType Directory -Force $DownloadsDir | Out-Null

$raylibZip = Join-Path $DownloadsDir "raylib-$RaylibVersion-win64-mingw.zip"
$raylibExtract = Join-Path $DownloadsDir "raylib-extract"
$raylibAssetUrl = Get-GithubAssetUrl `
    "https://api.github.com/repos/raysan5/raylib/releases/tags/$RaylibVersion" `
    "raylib-$RaylibVersion`_win64_mingw-w64.zip"

Download-File $raylibAssetUrl $raylibZip
New-CleanDirectory $raylibExtract
Expand-Archive -Force $raylibZip $raylibExtract
Copy-RaylibPackage $raylibExtract

$toolchainZip = Join-Path $DownloadsDir "w64devkit.zip"
$toolchainExtract = Join-Path $DownloadsDir "w64devkit-extract"
$toolchainAssetUrl = Get-GithubAssetUrl `
    "https://api.github.com/repos/skeeto/w64devkit/releases/latest" `
    "w64devkit*.zip"

Download-File $toolchainAssetUrl $toolchainZip
New-CleanDirectory $toolchainExtract
Expand-Archive -Force $toolchainZip $toolchainExtract
Copy-W64DevkitPackage $toolchainExtract

Write-Host ""
Write-Host "Dependencies installed:"
Write-Host "  Raylib:     $RaylibDir"
Write-Host "  W64Devkit:  $ToolchainDir"
Write-Host ""
Write-Host "Build with:"
Write-Host "  .\build.ps1"
