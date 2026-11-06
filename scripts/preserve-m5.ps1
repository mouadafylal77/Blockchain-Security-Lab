param([switch]$Create)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$archive = Join-Path $root 'build/m5-reference'
function Hash([string]$path) { (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant() }
if ($Create -and -not (Test-Path -LiteralPath "$archive/preservation.json")) {
    $ref = Get-Content -LiteralPath "$root/analysis/m5_reference.json" -Raw | ConvertFrom-Json
    $copies = @{}
    foreach ($entry in $ref.files.PSObject.Properties) { $copies[$entry.Name] = $entry.Value }
    $copies['docs/DEVELOPMENT_LOG.md'] = Hash "$root/docs/DEVELOPMENT_LOG.md"
    foreach ($file in Get-ChildItem -LiteralPath "$root/analysis" -File -Recurse) {
        $relative = $file.FullName.Substring($root.Length + 1).Replace('\','/')
        $copies[$relative] = Hash $file.FullName
    }
    foreach ($entry in $ref.binaries.PSObject.Properties) {
        $source = if ([IO.Path]::IsPathRooted($entry.Name)) { $entry.Name } else { Join-Path $root $entry.Name }
        if ((Hash $source) -ne $entry.Value) { throw "Frozen binary differs: $source" }
        $relative = if ([IO.Path]::IsPathRooted($entry.Name)) { 'build/ucrt64/' + (Split-Path -Leaf $source) } else { $entry.Name }
        $destination = Join-Path $archive $relative
        if (Test-Path -LiteralPath $destination) {
            if ((Hash $destination) -ne $entry.Value) { throw "Archive differs: $destination" }
            $copies[$relative] = $entry.Value
            continue
        }
        New-Item -ItemType Directory -Path (Split-Path -Parent $destination) -Force | Out-Null
        Copy-Item -LiteralPath $source -Destination $destination
        $copies[$relative] = $entry.Value
    }
    foreach ($entry in $copies.GetEnumerator()) {
        $destination = Join-Path $archive $entry.Key
        if (Test-Path -LiteralPath $destination) {
            if ((Hash $destination) -ne $entry.Value) { throw "Archive differs: $destination" }
            continue
        }
        $source = Join-Path $root $entry.Key
        if ((Hash $source) -ne $entry.Value) { throw "Frozen input differs: $source" }
        New-Item -ItemType Directory -Path (Split-Path -Parent $destination) -Force | Out-Null
        Copy-Item -LiteralPath $source -Destination $destination
    }
    $outputs = @{}
    foreach ($file in Get-ChildItem -LiteralPath "$root/results/raw/m5_30seeds" -Recurse -File) {
        $outputs[$file.FullName.Substring($root.Length + 1).Replace('\','/')] = Hash $file.FullName
    }
    @{ archive_files = $copies; historical_outputs = $outputs; original_commit = $ref.git_commit } |
        ConvertTo-Json -Depth 8 | Set-Content -LiteralPath "$archive/preservation.json" -Encoding UTF8
}
if (-not (Test-Path -LiteralPath "$archive/preservation.json")) { throw 'Run ./scripts/preserve-m5.ps1 -Create before changing the frozen engine.' }
$proof = Get-Content -LiteralPath "$archive/preservation.json" -Raw | ConvertFrom-Json
foreach ($entry in $proof.archive_files.PSObject.Properties) {
    if ((Hash (Join-Path $archive $entry.Name)) -ne $entry.Value) { throw "Archive differs: $($entry.Name)" }
    if ($entry.Name -like 'analysis/*' -or $entry.Name -like 'configs/*' -or $entry.Name -eq 'exo4.cpp') {
        if ((Hash (Join-Path $root $entry.Name)) -ne $entry.Value) { throw "Historical document/configuration differs: $($entry.Name)" }
    }
}
foreach ($entry in $proof.historical_outputs.PSObject.Properties) {
    if ((Hash (Join-Path $root $entry.Name)) -ne $entry.Value) { throw "Historical output differs: $($entry.Name)" }
}
Write-Host "M5 preserved: $(@($proof.archive_files.PSObject.Properties).Count) archived files; $(@($proof.historical_outputs.PSObject.Properties).Count) historical files verified."
Write-Host "Reference: $archive (local, ignored by Git; preserve this directory)."
