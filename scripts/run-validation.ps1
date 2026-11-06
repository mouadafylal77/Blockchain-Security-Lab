param([switch]$Run, [switch]$Smoke, [string]$OutputRoot = '')
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$protocol = Get-Content -LiteralPath "$root/configs/interactive/validation_protocol.json" -Raw | ConvertFrom-Json
if (-not $OutputRoot) {
    $index=1
    do { $OutputRoot=Join-Path $root "results/raw/pow_validation_v2/run_$index"; $index++ } while (Test-Path -LiteralPath $OutputRoot)
}
$OutputRoot=[IO.Path]::GetFullPath($OutputRoot)
if ($OutputRoot -match '(?i)[\\/](m5_30seeds|m5-reference)([\\/]|$)') { throw 'Historical M5 is read-only' }
if (Test-Path -LiteralPath $OutputRoot) { throw 'Output already exists; nothing overwritten' }
New-Item -ItemType Directory -Path "$OutputRoot/configs" -Force | Out-Null
Copy-Item -LiteralPath "$root/configs/interactive/validation_protocol.json" -Destination "$OutputRoot/protocol.json"
$binary=Join-Path $root 'build/ucrt64/blockchain_sim.exe'
$powers=if($Smoke){@(30,85)}else{@($protocol.powers_percent)}
$seeds=if($Smoke){@(2001)}else{@($protocol.validation_seed_first..$protocol.validation_seed_last)}
$detectors=if($Smoke){@('fixed')}else{@($protocol.detectors)}
$jobs=[Collections.Generic.List[object]]::new()
foreach($power in $powers){
    if($detectors -contains 'adaptive'){
        $c=Get-Content -LiteralPath "$root/configs/interactive/attack_51.json" -Raw | ConvertFrom-Json
        $c.nodes[0].hash_power=$power;$c.nodes[1].hash_power=(100-$power)/2;$c.nodes[2].hash_power=(100-$power)/2
        $c.seed=$protocol.calibration_seed;$c.target_height=0;$c.max_blocks=$protocol.max_blocks;$c.duration_ticks=$protocol.duration_ticks
        $c.attack.confirmations=$protocol.confirmations;$c.attack.enabled=$false;$c.security.mode='calibration';$c.security.detector='adaptive'
        $jobs.Add(@{name="calibration_q$power";config=$c;output="$OutputRoot/calibration/q$power"})
    }
    foreach($seed in $seeds){foreach($detector in $detectors){foreach($mode in $protocol.modes){
        $c=Get-Content -LiteralPath "$root/configs/interactive/attack_51.json" -Raw | ConvertFrom-Json
        $c.nodes[0].hash_power=$power;$c.nodes[1].hash_power=(100-$power)/2;$c.nodes[2].hash_power=(100-$power)/2
        $c.seed=$seed;$c.target_height=0;$c.max_blocks=$protocol.max_blocks;$c.duration_ticks=$protocol.duration_ticks
        $c.attack.confirmations=$protocol.confirmations;$c.attack.enabled=$mode -in @('attacked','protected')
        $c.security.mode=if($mode -eq 'honest_protected'){'protected'}else{$mode}
        $c.security.detector=$detector
        if($detector -eq 'adaptive'){$c.security | Add-Member -NotePropertyName reference_file -NotePropertyValue "$OutputRoot/calibration/q$power/calibration.json"}
        $name="q${power}_${seed}_${detector}_${mode}"
        $c.scenario=$name
        $jobs.Add(@{name=$name;config=$c;output="$OutputRoot/runs/$name"})
    }}}
}
$savedPath=$env:PATH
try {
    $env:PATH="C:\msys64\ucrt64\bin;$savedPath"
    foreach($job in $jobs){
        $path="$OutputRoot/configs/$($job.name).json"
        $job.config | ConvertTo-Json -Depth 15 | Set-Content -LiteralPath $path -Encoding utf8NoBOM
        $record=@{name=$job.name;config_path=$path;config_sha256=(Get-FileHash -LiteralPath $path).Hash;status='prepared';output=$job.output}
        if($Run){
            $record.binary_sha256=(Get-FileHash -LiteralPath $binary).Hash
            & $binary --config $path --output $job.output
            $record.exit_code=$LASTEXITCODE
            $record.status=if($LASTEXITCODE -eq 0){'completed'}else{'failed'}
            if($LASTEXITCODE -eq 0){$record.result_sha256=(Get-FileHash -LiteralPath "$($job.output)/results.json").Hash}
        }
        $record | ConvertTo-Json -Compress | Add-Content -LiteralPath "$OutputRoot/manifest.jsonl" -Encoding utf8NoBOM
        if($Run -and $record.exit_code -ne 0){throw "Simulation failed: $($job.name); manifest preserved"}
    }
} finally { $env:PATH=$savedPath }
Write-Host "$($jobs.Count) jobs $($(if($Run){'executed'}else{'prepared only'})); root=$OutputRoot"
