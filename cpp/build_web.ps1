param(
    [switch] $Force
)

$ErrorActionPreference = "Stop"

$ProjectRoot = Split-Path -Parent $MyInvocation.MyCommand.Path

# Default paths, assuming the user might provide their own via environment variables
# We prioritize the full Raylib source directory (which contains the Makefile) over the include-only folder.
$RaylibSource = if ($env:RAYLIB_SRC) { $env:RAYLIB_SRC } elseif (Test-Path "C:\raylib\raylib\src") { "C:\raylib\raylib\src" } elseif (Test-Path "$ProjectRoot\.deps\raylib\src") { "$ProjectRoot\.deps\raylib\src" } else { "$ProjectRoot\.deps\raylib\include" }

$RaylibInclude = if ($env:RAYLIB_INCLUDE) { $env:RAYLIB_INCLUDE } elseif (Test-Path "C:\raylib\raylib\src") { "C:\raylib\raylib\src" } else { "$ProjectRoot\.deps\raylib\include" }

# Determine the toolchain make executable
$MakePath = if ($env:RAYLIB_TOOLCHAIN_BIN) { Join-Path $env:RAYLIB_TOOLCHAIN_BIN "mingw32-make.exe" } elseif (Test-Path "C:\raylib\w64devkit\bin\mingw32-make.exe") { "C:\raylib\w64devkit\bin\mingw32-make.exe" } else { "mingw32-make.exe" }

if (-not (Get-Command emcc -ErrorAction SilentlyContinue)) {
    throw "emcc not found in PATH. Please install Emscripten (emsdk) and activate it."
}

if (-not (Get-Command emmake -ErrorAction SilentlyContinue)) {
    throw "emmake not found in PATH. Please install Emscripten (emsdk) and activate it."
}

# For web, libraylib.a MUST be compiled with Emscripten.
$RaylibWebLib = if ($env:RAYLIB_WEB_LIB) { $env:RAYLIB_WEB_LIB } else { "$ProjectRoot\.deps\raylib\lib\libraylib_web.a" }

if (-not (Test-Path $RaylibWebLib) -or $Force) {
    Write-Host "Raylib WebAssembly binary ($RaylibWebLib) not found or -Force used."
    Write-Host "Compiling Raylib from source ($RaylibSource) using Emscripten..."

    $RaylibSrcTemp = "$ProjectRoot\.deps\raylib_src_web"
    if (Test-Path $RaylibSrcTemp) {
        Remove-Item -Recurse -Force $RaylibSrcTemp
    }

    # Copy raylib sources to a temporary directory to avoid polluting the original source directory
    if (-not (Test-Path $RaylibSource)) {
        throw "Raylib source directory not found at $RaylibSource. Please ensure you have the Raylib source code installed."
    }

    Copy-Item -Recurse -Path $RaylibSource -Destination $RaylibSrcTemp

    Push-Location $RaylibSrcTemp
    try {
        if (-not (Test-Path "Makefile")) {
            throw "Makefile not found in $RaylibSource. Cannot compile Raylib for web. Ensure you have the Raylib source code (containing Makefile and .c files)."
        }

        # Build Raylib for Web
        Write-Host "Building Raylib for WebAssembly (this may take a minute)..."
        & emmake $MakePath clean PLATFORM=PLATFORM_WEB RAYLIB_RELEASE_PATH=.
        & emmake $MakePath PLATFORM=PLATFORM_WEB RAYLIB_RELEASE_PATH=.

        if ($LASTEXITCODE -ne 0) {
            throw "Raylib compilation for web failed."
        }

        $LibDir = Split-Path -Parent $RaylibWebLib
        if (-not (Test-Path $LibDir)) {
            New-Item -ItemType Directory -Force -Path $LibDir | Out-Null
        }

        if (-not (Test-Path "libraylib.a")) {
            throw "libraylib.a was not found after compilation."
        }

        Copy-Item -Path "libraylib.a" -Destination $RaylibWebLib -Force
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

# Check for minshell.html
$ShellFile = Join-Path $RaylibInclude "minshell.html"
if (-not (Test-Path $ShellFile)) {
    # Fallback to standard location
    $ShellFile = "C:/raylib/raylib/src/minshell.html"
}

$EmccArgs = @(
    "-std=c++17",
    "-Wall",
    "-Iinclude",
    "-I$($RaylibInclude -replace '\\', '/')",
    "-s", "USE_GLFW=3",
    "-s", "ASYNCIFY",
    "-s", "MAX_WEBGL_VERSION=2",
    "-s", "MIN_WEBGL_VERSION=2",
    "-s", "EXPORTED_RUNTIME_METHODS=['HEAPF32']",
    "--shell-file", "$($ShellFile -replace '\\', '/')"
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
