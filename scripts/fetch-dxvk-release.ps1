param(
    [string]$Version = "latest"
)

$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
$profileRoot = Join-Path $repoRoot "profiles\bo3-vulkan"
$binRoot = Join-Path $profileRoot "bin"
$downloadRoot = Join-Path $profileRoot "downloads"

New-Item -ItemType Directory -Force -Path $binRoot, $downloadRoot | Out-Null

if ($Version -eq "latest") {
    $release = Invoke-RestMethod -Uri "https://api.github.com/repos/doitsujin/dxvk/releases/latest"
}
else {
    $tag = if ($Version.StartsWith("v")) { $Version } else { "v$Version" }
    $release = Invoke-RestMethod -Uri "https://api.github.com/repos/doitsujin/dxvk/releases/tags/$tag"
}

$asset = $release.assets | Where-Object { $_.name -match '^dxvk-.*\.tar\.gz$' } | Select-Object -First 1
if (-not $asset) {
    throw "No dxvk tar.gz release asset found for $($release.tag_name)."
}

$archive = Join-Path $downloadRoot $asset.name
$extractRoot = Join-Path $downloadRoot ([IO.Path]::GetFileNameWithoutExtension([IO.Path]::GetFileNameWithoutExtension($asset.name)))

Write-Host "Downloading $($asset.browser_download_url)"
Invoke-WebRequest -Uri $asset.browser_download_url -OutFile $archive

if (Test-Path $extractRoot) {
    Remove-Item -LiteralPath $extractRoot -Recurse -Force
}

tar -xzf $archive -C $downloadRoot

$x64Source = Join-Path $extractRoot "x64"
$x32Source = Join-Path $extractRoot "x32"
if (-not (Test-Path (Join-Path $x64Source "d3d11.dll"))) {
    throw "Extracted DXVK x64 files not found under $x64Source."
}

Copy-Item -LiteralPath $x64Source -Destination (Join-Path $binRoot "x64") -Recurse -Force
if (Test-Path $x32Source) {
    Copy-Item -LiteralPath $x32Source -Destination (Join-Path $binRoot "x32") -Recurse -Force
}

Write-Host "DXVK $($release.tag_name) staged under $binRoot"

