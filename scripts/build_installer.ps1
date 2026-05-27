param(
    [string]$Version = ""
)

$ErrorActionPreference = "Stop"

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")

if (-not $Version) {
    $resourceH = Join-Path $repoRoot "resources\resource.h"
    if (Test-Path $resourceH) {
        $versionMatch = Get-Content $resourceH | Select-String 'VER_PRODUCT_VERSION_STR\s+"([^"]+)"'
        if ($versionMatch) {
            $rawVersion = $versionMatch.Matches[0].Groups[1].Value
            $Version = $rawVersion -replace '\.0$', ''
        }
    }
}

if (-not $Version) {
    $Version = "0.1.4" # Fallback
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
