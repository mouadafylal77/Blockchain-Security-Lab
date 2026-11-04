$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$ucrtRoot = 'C:\msys64\ucrt64'
$vsTools = 'C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake'

function Find-BuildTool([string]$name, [string]$fallback) {
    $msysPath = Join-Path "$ucrtRoot\bin" "$name.exe"
    if (Test-Path -LiteralPath $msysPath) { return $msysPath }
    $found = Get-Command "$name.exe" -ErrorAction SilentlyContinue
    if ($found) { return $found.Source }
    if (Test-Path -LiteralPath $fallback) { return $fallback }
    throw "Outil introuvable : $name. Installer les paquets UCRT64 cmake et ninja."
}

$cmake = Find-BuildTool 'cmake' "$vsTools\CMake\bin\cmake.exe"
$ninja = Find-BuildTool 'ninja' "$vsTools\Ninja\ninja.exe"
$ctest = Join-Path (Split-Path -Parent $cmake) 'ctest.exe'
foreach ($required in @("$ucrtRoot\bin\g++.exe", "$ucrtRoot\include\openssl\sha.h", $ctest)) {
    if (-not (Test-Path -LiteralPath $required)) { throw "Dependance absente : $required" }
}
Write-Host "CMake : $cmake"
Write-Host "Ninja : $ninja"
Write-Host "CTest : $ctest"
Write-Host "GCC / OpenSSL : $ucrtRoot"

$savedPath = $env:PATH
Push-Location $repoRoot
try {
    $env:PATH = "$ucrtRoot\bin;$(Split-Path -Parent $cmake);$(Split-Path -Parent $ninja);$savedPath"
    & $cmake --preset ucrt64 "-DCMAKE_MAKE_PROGRAM=$($ninja.Replace('\', '/'))"
    if ($LASTEXITCODE -ne 0) { throw "Configuration CMake echouee ($LASTEXITCODE)" }
    & $cmake --build --preset ucrt64 --parallel 2
    if ($LASTEXITCODE -ne 0) { throw "Compilation echouee ($LASTEXITCODE)" }
    & $ctest --preset ucrt64 --output-on-failure
    if ($LASTEXITCODE -ne 0) { throw "Tests echoues ($LASTEXITCODE)" }
    & "$repoRoot\build\ucrt64\blockchain_sim.exe" --demo
    if ($LASTEXITCODE -ne 0) { throw "Demonstration echouee ($LASTEXITCODE)" }
} finally {
    $env:PATH = $savedPath
    Pop-Location
}
