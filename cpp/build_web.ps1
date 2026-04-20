$ErrorActionPreference = "Stop"

$ProjectRoot = Split-Path -Parent $MyInvocation.MyCommand.Path

# Default paths, assuming the user might provide their own via environment variables
$RaylibInclude = if ($env:RAYLIB_INCLUDE) { $env:RAYLIB_INCLUDE } elseif (Test-Path "$ProjectRoot\.deps\raylib\include") { "$ProjectRoot\.deps\raylib\include" } else { "C:\raylib\raylib\src" }

# Determine the toolchain make executable
$MakePath = if ($env:RAYLIB_TOOLCHAIN_BIN) { Join-Path $env:RAYLIB_TOOLCHAIN_BIN "mingw32-make.exe" } elseif (Test-Path "C:\raylib\w64devkit\bin\mingw32-make.exe") { "C:\raylib\w64devkit\bin\mingw32-make.exe" } else { "mingw32-make.exe" }

if (-not (Get-Command emcc -ErrorAction SilentlyContinue)) {
    throw "emcc not found in PATH. Please install Emscripten (emsdk) and activate it."
}

# For web, libraylib.a MUST be compiled with Emscripten.
$RaylibWebLib = if ($env:RAYLIB_WEB_LIB) { $env:RAYLIB_WEB_LIB } else { "$ProjectRoot\.deps\raylib\lib\libraylib_web.a" }

if (-not (Test-Path $RaylibWebLib)) {
    Write-Host "Raylib WebAssembly binary ($RaylibWebLib) not found."
    Write-Host "Compiling Raylib from source ($RaylibInclude) using Emscripten..."

    $RaylibSrcTemp = "$ProjectRoot\.deps\raylib_src_web"
    if (Test-Path $RaylibSrcTemp) {
        Remove-Item -Recurse -Force $RaylibSrcTemp
    }

    # Copy raylib sources to a temporary directory to avoid polluting the original source directory
    Copy-Item -Recurse -Path $RaylibInclude -Destination $RaylibSrcTemp

    Push-Location $RaylibSrcTemp
    try {
        & $MakePath PLATFORM=PLATFORM_WEB RAYLIB_RELEASE_PATH=.
        if ($LASTEXITCODE -ne 0) {
            throw "Failed to compile Raylib for WebAssembly."
        }

        $LibDir = Split-Path -Parent $RaylibWebLib
        if (-not (Test-Path $LibDir)) {
            New-Item -ItemType Directory -Force -Path $LibDir | Out-Null
        }

        Copy-Item -Path "libraylib.a" -Destination $RaylibWebLib
    }
    finally {
        Pop-Location
        Remove-Item -Recurse -Force $RaylibSrcTemp
    }
    Write-Host "Raylib WebAssembly library built successfully.`n"
}

$OutputDir = Join-Path $ProjectRoot "build\web"
if (-not (Test-Path $OutputDir)) {
    New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null
}

$Sources = @(
    "src/main.cpp",
    "src/simulation_app.cpp"
)
$Sources += Get-ChildItem -Path "src\simulations\*.cpp" | Resolve-Path -Relative | ForEach-Object { $_ -replace '\\', '/' }

$EmccArgs = @(
    "-std=c++17",
    "-Wall",
    "-Iinclude",
    "-I$($RaylibInclude -replace '\\', '/')",
    "-s", "USE_GLFW=3",
    "-s", "ASYNCIFY",
    "--shell-file", "C:/raylib/raylib/src/minshell.html"
) + $Sources + @(
    "$($RaylibWebLib -replace '\\', '/')",
    "-o", "build/web/simulations.html"
)

Push-Location $ProjectRoot
try {
    Write-Host "Building Raylib workspace for WebAssembly..."
    Write-Host "emcc $($EmccArgs -join ' ')"
    & emcc @EmccArgs

    if ($LASTEXITCODE -ne 0) {
        throw "Compilation failed with exit code $LASTEXITCODE"
    }
}
finally {
    Pop-Location
}

Write-Host "`nBuild successful!"
Write-Host "Web build created at: $ProjectRoot\build\web\simulations.html"
Write-Host "To test, run a local web server (e.g., 'python -m http.server -d build/web' or 'npx serve build/web')"
