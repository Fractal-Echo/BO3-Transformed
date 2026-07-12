param(
    [ValidatePattern('^v?\d+\.\d+(?:\.\d+)?$')]
    [string]$Version = "v3.0",

    [ValidatePattern('^[A-Fa-f0-9]{64}$')]
    [string]$ExpectedSha256 = "7dd3243fe1b260a0e9b0b9e49d672ae32e3398bee18c97e7e8569d0ef0eca92d"
)

$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
$profileRoot = Join-Path $repoRoot "profiles\bo3-vulkan"
$binRoot = Join-Path $profileRoot "bin"
$downloadRoot = Join-Path $profileRoot "downloads"

New-Item -ItemType Directory -Force -Path $binRoot, $downloadRoot | Out-Null

$tag = if ($Version.StartsWith("v")) { $Version } else { "v$Version" }
$defaultDigest = "7dd3243fe1b260a0e9b0b9e49d672ae32e3398bee18c97e7e8569d0ef0eca92d"
if ($tag -ne "v3.0" -and $ExpectedSha256 -eq $defaultDigest) {
    throw "A non-default DXVK version requires its explicit -ExpectedSha256 digest."
}
$release = Invoke-RestMethod -Uri "https://api.github.com/repos/doitsujin/dxvk/releases/tags/$tag"

$asset = $release.assets | Where-Object { $_.name -match '^dxvk-.*\.tar\.gz$' } | Select-Object -First 1
if (-not $asset) {
    throw "No dxvk tar.gz release asset found for $($release.tag_name)."
}

$archive = Join-Path $downloadRoot $asset.name
$extractRoot = Join-Path $downloadRoot ([IO.Path]::GetFileNameWithoutExtension([IO.Path]::GetFileNameWithoutExtension($asset.name)))

Write-Host "Downloading $($asset.browser_download_url)"
Invoke-WebRequest -Uri $asset.browser_download_url -OutFile $archive

$actualSha256 = (Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash.ToLowerInvariant()
$expectedSha256 = $ExpectedSha256.ToLowerInvariant()
if ($actualSha256 -ne $expectedSha256) {
    Remove-Item -LiteralPath $archive -Force
    throw "DXVK archive SHA-256 mismatch: expected $expectedSha256, got $actualSha256."
}

if ($asset.digest -and $asset.digest -ne "sha256:$expectedSha256") {
    Remove-Item -LiteralPath $archive -Force
    throw "GitHub release digest does not match the operator-pinned SHA-256."
}

Write-Host "Verified SHA-256 $actualSha256"

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
