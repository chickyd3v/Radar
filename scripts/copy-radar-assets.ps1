# Copies icons.png and default config from a PoeFixer install into the Radar plugin tree.
param(
    [string]$HostRoot = (Resolve-Path "$PSScriptRoot\..\..\PoeFixer").Path,
    [string]$PluginRoot = (Resolve-Path "$PSScriptRoot\..").Path
)

$iconSrc = Join-Path $HostRoot "Resources\radar\icons.png"
$iconDst = Join-Path $PluginRoot "assets\icons.png"
if (-not (Test-Path $iconSrc)) {
    Write-Error "Missing $iconSrc — install PoeFixer with radar resources first."
    exit 1
}
New-Item -ItemType Directory -Force -Path (Split-Path $iconDst) | Out-Null
Copy-Item $iconSrc $iconDst -Force
Write-Host "Copied icons.png to $iconDst"

$targets = @("acts.json", "endgame.json", "ignore.json")
foreach ($t in $targets) {
    $s = Join-Path $HostRoot "Resources\radar\$t"
    $d = Join-Path $PluginRoot "config\targets\$t"
    if (Test-Path $s) {
        New-Item -ItemType Directory -Force -Path (Split-Path $d) | Out-Null
        Copy-Item $s $d -Force
        Write-Host "Copied $t"
    }
}
