param(
    [Parameter(Mandatory = $true)][string]$Executable,
    [switch]$ExpectDisabled
)
# Read-only configuration verification. A pass is NOT proof of a real logon/restart.
$ErrorActionPreference = 'Stop'
$exe = (Resolve-Path -LiteralPath $Executable).Path
$sid = [System.Security.Principal.WindowsIdentity]::GetCurrent().User.Value
$name = "WindowsProcessControlCenter.Startup.$sid"
$service = New-Object -ComObject 'Schedule.Service'
$service.Connect()
$root = $service.GetFolder('\')
$task = $null
try { $task = $root.GetTask($name) }
catch {
    if ($_.Exception.HResult -ne -2147024894) { throw }
}
function Assert-Startup([bool]$Condition, [string]$Message) {
    if (-not $Condition) { throw $Message }
}
function Resolve-UserSid([string]$User) {
    if ($User -match '^S-1-') { return $User }
    return ([System.Security.Principal.NTAccount]::new($User)).Translate([System.Security.Principal.SecurityIdentifier]).Value
}
if ($ExpectDisabled) {
    Assert-Startup (($null -eq $task) -or (-not $task.Enabled)) 'Startup task is still enabled.'
} else {
    Assert-Startup ($null -ne $task) 'Startup task was not found.'
    $definition = $task.Definition
    Assert-Startup $task.Enabled 'Task is disabled.'
    Assert-Startup ($definition.RegistrationInfo.Source -eq 'WPCC.Startup.v1:{6FDC4703-94B6-4E3D-98B1-B22588940D1E}') 'Ownership marker differs.'
    Assert-Startup ((Resolve-UserSid $definition.Principal.UserId) -eq $sid) 'Wrong task user.'
    Assert-Startup ($definition.Principal.LogonType -eq 3) 'Expected InteractiveToken.'
    Assert-Startup ($definition.Principal.RunLevel -eq 1) 'Expected HighestAvailable.'
    Assert-Startup ($definition.Actions.Count -eq 1) 'Expected one action.'
    $action = $definition.Actions.Item(1)
    Assert-Startup ($action.Path -ieq $exe) 'Executable path differs.'
    Assert-Startup ($action.Arguments -ceq '--minimized') 'Arguments differ.'
    Assert-Startup ($action.WorkingDirectory -ieq (Split-Path -Parent $exe)) 'Working directory differs.'
    Assert-Startup ($definition.Triggers.Count -eq 1) 'Expected one trigger.'
    $trigger = $definition.Triggers.Item(1)
    Assert-Startup ($trigger.Type -eq 9) 'Expected a logon trigger.'
    Assert-Startup $trigger.Enabled 'Trigger is disabled.'
    Assert-Startup ((Resolve-UserSid $trigger.UserId) -eq $sid) 'Wrong trigger user.'
    Assert-Startup ($trigger.Delay -eq 'PT10S') 'Unexpected logon delay.'
    Assert-Startup (-not $trigger.StartBoundary -and -not $trigger.EndBoundary) 'Trigger has a date restriction.'
    $settings = $definition.Settings
    Assert-Startup (-not $settings.DisallowStartIfOnBatteries -and -not $settings.StopIfGoingOnBatteries) 'Battery restriction present.'
    Assert-Startup (-not $settings.RunOnlyIfIdle -and -not $settings.RunOnlyIfNetworkAvailable) 'Idle/network restriction present.'
    Assert-Startup ($settings.ExecutionTimeLimit -eq 'PT0S') 'Unexpected execution time limit.'
    Assert-Startup ($settings.MultipleInstances -eq 2) 'Expected IgnoreNew instance policy.'
}
$run = Get-ItemPropertyValue -LiteralPath 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Run' -Name 'WindowsProcessControlCenter' -ErrorAction SilentlyContinue
Assert-Startup ($run -ine ('"' + $exe + '" --minimized')) 'Recognized legacy Run entry remains.'
Write-Host 'PASS: Windows startup configuration. Still perform real sign-out/sign-in and restart tests.'
