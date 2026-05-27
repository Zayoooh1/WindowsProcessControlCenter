param(
    [string]$Version = ""
)

$ErrorActionPreference = "Stop"

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")

$versionFile = Join-Path $repoRoot "version.txt"
if (-not $Version -and (Test-Path $versionFile)) {
    $Version = (Get-Content $versionFile).Trim()
}

if (-not $Version) {
    $Version = "0.1.4" # Fallback
}

# Parse version (supporting X.Y.Z or X.Y.Z.W)
$major = 0
$minor = 0
$patch = 0
$build = 0

if ($Version -match '^(\d+)\.(\d+)\.(\d+)(?:\.(\d+))?$') {
    $major = $Matches[1]
    $minor = $Matches[2]
    $patch = $Matches[3]
    if ($Matches[4]) {
        $build = $Matches[4]
    }
} else {
    throw "Invalid version format: $Version. Must be X.Y.Z or X.Y.Z.W"
}

$semanticVersion = "$major.$minor.$patch"
$quadVersion = "$major.$minor.$patch.$build"

Write-Host "Syncing version $Version ($quadVersion) to codebase files..."

# 1. Update resources/resource.h
$resourceHPath = Join-Path $repoRoot "resources\resource.h"
if (Test-Path $resourceHPath) {
    Write-Host "Updating $resourceHPath..."
    $content = Get-Content $resourceHPath -Raw
    $content = $content -replace '#define VER_FILE_VERSION_MAJOR\s+\d+', "#define VER_FILE_VERSION_MAJOR $major"
    $content = $content -replace '#define VER_FILE_VERSION_MINOR\s+\d+', "#define VER_FILE_VERSION_MINOR $minor"
    $content = $content -replace '#define VER_FILE_VERSION_PATCH\s+\d+', "#define VER_FILE_VERSION_PATCH $patch"
    $content = $content -replace '#define VER_FILE_VERSION_BUILD\s+\d+', "#define VER_FILE_VERSION_BUILD $build"
    $content = $content -replace '#define VER_FILE_VERSION_STR\s+"[^"]+"', "#define VER_FILE_VERSION_STR `"$quadVersion`""
    $content = $content -replace '#define VER_PRODUCT_VERSION_STR\s+"[^"]+"', "#define VER_PRODUCT_VERSION_STR `"$quadVersion`""
    Set-Content $resourceHPath $content
}

# 2. Update resources/WindowsProcessControlCenter.rc
$rcPath = Join-Path $repoRoot "resources\WindowsProcessControlCenter.rc"
if (Test-Path $rcPath) {
    Write-Host "Updating $rcPath..."
    $content = Get-Content $rcPath -Raw
    $content = $content -replace 'FILEVERSION\s+\d+,\d+,\d+,\d+', "FILEVERSION     $major,$minor,$patch,$build"
    $content = $content -replace 'PRODUCTVERSION\s+\d+,\d+,\d+,\d+', "PRODUCTVERSION  $major,$minor,$patch,$build"
    Set-Content $rcPath $content
}

# 3. Update web/app.js
$appJsPath = Join-Path $repoRoot "web\app.js"
if (Test-Path $appJsPath) {
    Write-Host "Updating $appJsPath..."
    $content = Get-Content $appJsPath -Raw
    $content = $content -replace 'const CURRENT_VERSION\s*=\s*"[^"]+";', "const CURRENT_VERSION = `"$semanticVersion`";"
    Set-Content $appJsPath $content
}

$installerScript = Join-Path $repoRoot "installer\WindowsProcessControlCenter.iss"
$installerOutput = Join-Path $repoRoot "dist\installer\WindowsProcessControlCenter-$Version-setup.exe"

function Find-InnoSetupCompiler {
    $fromPath = Get-Command "ISCC.exe" -ErrorAction SilentlyContinue
    if ($fromPath) {
        return $fromPath.Source
    }

    $standardPaths = @(
        "C:\Program Files (x86)\Inno Setup 6\ISCC.exe",
        "C:\Program Files\Inno Setup 6\ISCC.exe"
    )

    foreach ($path in $standardPaths) {
        if (Test-Path $path) {
            return $path
        }
    }

    return $null
}

$iscc = Find-InnoSetupCompiler
if (-not $iscc) {
    Write-Error @"
Inno Setup Compiler was not found.

Install Inno Setup 6 from https://jrsoftware.org/isinfo.php and either:
- add ISCC.exe to PATH, or
- install it into C:\Program Files (x86)\Inno Setup 6\ or C:\Program Files\Inno Setup 6\.

Then run:
.\scripts\build_installer.ps1
"@
}

& (Join-Path $repoRoot "scripts\package_release.ps1") -Version $Version

if (-not (Test-Path $installerScript)) {
    throw "Inno Setup script was not found: $installerScript"
}

& $iscc "/DMyAppVersion=$Version" $installerScript

if (-not (Test-Path $installerOutput)) {
    throw "Installer was not generated: $installerOutput"
}

Write-Host "Installer EXE: $installerOutput"
