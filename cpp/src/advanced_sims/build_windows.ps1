$ErrorActionPreference = "Stop"

# Get the root directory of the project
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectRoot = Split-Path -Parent (Split-Path -Parent $ScriptDir)
$ToolchainBin = "C:\raylib\w64devkit\bin"
$RaylibInclude = "C:\raylib\raylib\src"
$Make = Join-Path $ToolchainBin "g++.exe"

if (-not (Test-Path $Make)) {
    throw "w64devkit g++.exe not found at $Make"
}

$OutDir = Join-Path $ProjectRoot "build\advanced"
if (-not (Test-Path $OutDir)) {
    New-Item -ItemType Directory -Force $OutDir | Out-Null
}

$ExePath = Join-Path $OutDir "gpu_particles.exe"

Write-Host "Compiling Compute Shader Particle Sim for Windows (RTX 4060)..."
Write-Host "Please wait..."

$Args = @(
    "-std=c++17",
    "-Wall",
    "-Wextra",
    "-O3",  # Maximum optimization
    "-I$($ProjectRoot)\include",
    "-I$RaylibInclude",
    "src\advanced_sims\gpu_particles.cpp",
    "-o", $ExePath,
    "-L$RaylibInclude",
    "-lraylib",
    "-lopengl32",
    "-lgdi32",
    "-lwinmm",
    "-static-libgcc",
    "-static-libstdc++"
)

$env:PATH = "$ToolchainBin;$env:PATH"

Push-Location $ProjectRoot
try {
    & $Make $Args
    if ($LASTEXITCODE -ne 0) {
        throw "Compilation failed!"
    }
}
finally {
    Pop-Location
}

Write-Host "`nBuild successful! Run the simulation with the following command:"
Write-Host ".\build\advanced\gpu_particles.exe" -ForegroundColor Green
