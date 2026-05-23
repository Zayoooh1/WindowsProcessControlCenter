param(
    [string]$Version = "0.1.0",
    [string]$Configuration = "Release",
    [switch]$SkipBuild
)

$ErrorActionPreference = "Stop"

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
$buildDir = Join-Path $repoRoot "build"
$distDir = Join-Path $repoRoot "dist"
$packageName = "WindowsProcessControlCenter-$Version-portable"
$packageDir = Join-Path $distDir $packageName
$zipPath = Join-Path $distDir "$packageName.zip"
$exePath = Join-Path $buildDir "$Configuration\WindowsProcessControlCenter.exe"
$webSource = Join-Path $repoRoot "web"

if (-not $SkipBuild) {
    cmake -S $repoRoot -B $buildDir -G "Visual Studio 17 2022" -A x64
    cmake --build $buildDir --config $Configuration
}

if (-not (Test-Path $exePath)) {
    throw "Release executable was not found: $exePath"
}

if (-not (Test-Path (Join-Path $webSource "index.html"))) {
    throw "Web frontend was not found: $webSource"
}

New-Item -ItemType Directory -Force -Path $distDir | Out-Null

$resolvedDist = (Resolve-Path $distDir).Path
if (Test-Path $packageDir) {
    $resolvedPackage = (Resolve-Path $packageDir).Path
    if (-not $resolvedPackage.StartsWith($resolvedDist, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to remove package directory outside dist: $resolvedPackage"
    }
    Remove-Item -LiteralPath $resolvedPackage -Recurse -Force
}

if (Test-Path $zipPath) {
    Remove-Item -LiteralPath $zipPath -Force
}

New-Item -ItemType Directory -Force -Path $packageDir | Out-Null

Copy-Item -LiteralPath $exePath -Destination (Join-Path $packageDir "WindowsProcessControlCenter.exe")
Copy-Item -LiteralPath $webSource -Destination (Join-Path $packageDir "web") -Recurse
Copy-Item -LiteralPath (Join-Path $repoRoot "README.md") -Destination (Join-Path $packageDir "README.md")
Copy-Item -LiteralPath (Join-Path $repoRoot "RELEASE_NOTES.md") -Destination (Join-Path $packageDir "RELEASE_NOTES.md")

$licensePath = Join-Path $repoRoot "LICENSE"
if (Test-Path $licensePath) {
    Copy-Item -LiteralPath $licensePath -Destination (Join-Path $packageDir "LICENSE")
}

Compress-Archive -Path (Join-Path $packageDir "*") -DestinationPath $zipPath -CompressionLevel Optimal

Write-Host "Portable release folder: $packageDir"
Write-Host "Portable release ZIP: $zipPath"
