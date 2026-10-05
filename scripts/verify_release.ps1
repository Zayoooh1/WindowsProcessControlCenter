# Run only on the disposable Windows CI runner: silently installs and uninstalls Setup.
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$version = (Get-Content (Join-Path $root 'version.txt') -Raw).Trim()
$exe = Join-Path $root 'build/Release/WindowsProcessControlCenter.exe'
$dist = Join-Path $root 'dist'
$zip = Join-Path $dist "WindowsProcessControlCenter-v$version-Portable.zip"
$setup = Join-Path $dist "WindowsProcessControlCenter-v$version-Setup.exe"
$expected = (Get-FileHash $exe -Algorithm SHA256).Hash
if ((Get-Item $exe).VersionInfo.FileVersion -ne "$version.0") {
    throw 'Built executable version does not match version.txt'
}

$mt = Get-ChildItem 'C:/Program Files (x86)/Windows Kits/10/bin/*/x64/mt.exe' |
    Sort-Object FullName -Descending | Select-Object -First 1
if (-not $mt) { throw 'Windows SDK manifest tool not found' }
$manifest = Join-Path $env:RUNNER_TEMP 'wpcc-embedded.manifest'
& $mt.FullName "-inputresource:$exe;#1" "-out:$manifest"
if ($LASTEXITCODE -ne 0) { throw 'Cannot extract embedded EXE manifest' }
if ((Get-Content $manifest -Raw) -notmatch 'level="requireAdministrator"') {
    throw 'Expected elevation manifest is missing from the built executable'
}

$portable = Join-Path $env:RUNNER_TEMP 'wpcc-portable-check'
Expand-Archive -LiteralPath $zip -DestinationPath $portable
function Assert-Payload([string]$Directory) {
    if ((Get-FileHash (Join-Path $Directory 'WindowsProcessControlCenter.exe') -Algorithm SHA256).Hash -ne $expected) {
        throw "EXE differs from Release build: $Directory"
    }
    foreach ($asset in @('index.html', 'app.js', 'settings-queue.js')) {
        $built = Join-Path $root "build/Release/web/$asset"
        $packaged = Join-Path $Directory "web/$asset"
        if ((Get-FileHash $built).Hash -ne (Get-FileHash $packaged).Hash) {
            throw "Web asset differs from Release build: $asset"
        }
    }
}
Assert-Payload $portable
if (-not (Test-Path (Join-Path $portable 'scripts/verify_startup.ps1'))) {
    throw 'Portable startup verifier missing'
}
$installed = Join-Path $env:RUNNER_TEMP 'WPCC setup verification'
$process = Start-Process -FilePath $setup -ArgumentList @('/VERYSILENT', '/SUPPRESSMSGBOXES', '/NORESTART', '/SP-', "/DIR=`"$installed`"") -PassThru -Wait
if ($process.ExitCode -ne 0) { throw "Setup failed: $($process.ExitCode)" }
Assert-Payload $installed
$uninstaller = Join-Path $installed 'unins000.exe'
$process = Start-Process -FilePath $uninstaller -ArgumentList @('/VERYSILENT', '/SUPPRESSMSGBOXES', '/NORESTART') -PassThru -Wait
if ($process.ExitCode -ne 0) { throw "Uninstall failed: $($process.ExitCode)" }
if (Test-Path (Join-Path $installed 'WindowsProcessControlCenter.exe')) { throw 'Uninstall left the executable behind' }

@($setup, $zip) | ForEach-Object {
    $hash = Get-FileHash $_ -Algorithm SHA256
    '{0}  {1}' -f $hash.Hash.ToLowerInvariant(), (Split-Path $_ -Leaf)
} | Set-Content (Join-Path $dist 'SHA256SUMS.txt') -Encoding utf8
Write-Host 'Verified versions, embedded manifest, Portable payload, silent Setup and clean uninstall.'
Write-Host 'Interactive startup after logon/reboot remains untested.'
