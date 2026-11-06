# Targeted real terminal sessions only. Never runs M5 or changes its artifacts.
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$binary=Join-Path $root 'build/ucrt64/blockchain_sim.exe'
$index=1
do { $output=Join-Path $root "results/raw/terminal_review/checks_$index"; $index++ } while(Test-Path -LiteralPath $output)
New-Item -ItemType Directory -Path $output -Force | Out-Null
New-Item -ItemType Directory -Path "$root/results/raw/interactive" -Force | Out-Null
function Simple-Input([int]$attack,[bool]$defense,[int]$power=40){
    $lines=@('1','5','2',$(if($attack -eq 4){'15'}else{'30'}),"$attack")
    if($attack -ne 4){$lines+=@('-1',"$power")}
    else{$lines+='9'} # invalid security choice, then corrected
    $lines+=@($(if($defense){'1'}else{'0'}),'1','0')
    return $lines
}
function Custom-Input([int]$attack){
    $lines=@('2','5','2','20',"$attack",'1','42','40')
    if($attack -ne 4){$lines+='1'}
    $lines+='60';if($attack -ne 4){$lines+='0'}
    $lines+=@('4','3','7','3')
    if($attack -eq 1){$lines+='6'}
    if($attack -eq 3){$lines+='1'}
    if($attack -eq 4){$lines+=@('1','0','0','0','1','2','3','4')}
    $lines+=@($(if($attack -eq 3){'200'}else{'50'}),$(if($attack -eq 3){'2'}else{'1'}),
        '1000000','100000','100','100000','100000','1',$(if($attack -eq 3){'10000'}else{'2000'}),'10','500')
    if($attack -eq 1){$lines+=@('0.5','0.005','500')}
    if($attack -eq 2){$lines+=@('40','0.25','0.005')}
    if($attack -eq 3){$lines+=@('3','64')}
    if($attack -eq 4){$lines+=@('0.8','500')}
    $lines+='500'
    if($attack -eq 1){$lines+=@('2000','3','100','0','1')}
    if($attack -eq 2){$lines+='100'}
    if($attack -eq 4){$lines+=@('4','1')}
    return $lines+@('0','0','1','0')
}
$cases=@()
foreach($attack in 1..4){foreach($defense in @($false,$true)){
    $cases+=@{name="simple_${attack}_$defense";lines=@(Simple-Input $attack $defense)}
}}
foreach($attack in 1..4){$cases+=@{name="custom_$attack";lines=@(Custom-Input $attack)}}
foreach($defense in @($false,$true)){$cases+=@{name="majority_51_$defense";lines=@(Simple-Input 1 $defense 85)}}
$records=@();$savedPath=$env:PATH
Push-Location $root
try {
    $env:PATH="C:\msys64\ucrt64\bin;$savedPath"
    foreach($case in $cases){
        $before=@(Get-ChildItem -LiteralPath "$root/results/raw/interactive" -Directory | Select-Object -ExpandProperty Name)
        $case.lines | Set-Content -LiteralPath "$output/$($case.name).input.txt" -Encoding utf8
        $text=$case.lines | & $binary --interactive 2>&1
        $code=$LASTEXITCODE
        $text | Set-Content -LiteralPath "$output/$($case.name).stdout.txt" -Encoding utf8
        if($code -ne 0){throw "Terminal session failed: $($case.name), exit=$code"}
        $created=@(Get-ChildItem -LiteralPath "$root/results/raw/interactive" -Directory | Where-Object Name -NotIn $before)
        if($created.Count -ne 1){throw "Expected exactly one exported terminal run: $($case.name)"}
        $resultPath=Join-Path $created[0].FullName 'results.json'
        $r=Get-Content -LiteralPath $resultPath -Raw | ConvertFrom-Json
        $record=[ordered]@{case=$case.name;result_path=$resultPath;sha256=(Get-FileHash -LiteralPath $resultPath).Hash;
            seed=$r.config.seed;attack=$r.config.attack.type;mode=$r.security.mode;outcome=$r.outcome;execution=$r.execution;
            blocks=$r.metrics.blocks_produced;forks=$r.metrics.fork_observations;reorganizations=$r.metrics.reorganizations;
            alerts=@($r.security.alerts).Count;actions=$r.security.overhead.mitigation_actions;retained=$r.attack.retained;
            publications=$r.attack.publications;active_share=$r.attack.active_block_share_at_target;filtered=$r.attack.filtered_messages;
            deferred=$r.security.overhead.deferred_messages;uncredited=$r.security.overhead.shares_uncredited;
            reconnections=$r.security.overhead.reconnections;accepted=$r.attack.merchant_accepted;accepted_at=$r.attack.merchant_accepted_at}
        $records+=@($record)
        $record | ConvertTo-Json -Depth 10 -Compress | Add-Content -LiteralPath "$output/manifest.jsonl" -Encoding utf8
        Write-Host "$($case.name): $($r.outcome), blocks=$($r.metrics.blocks_produced), alerts=$(@($r.security.alerts).Count), actions=$($r.security.overhead.mitigation_actions)"
    }
    $records | ConvertTo-Json -Depth 12 | Set-Content -LiteralPath "$output/summary.json" -Encoding utf8
} finally {Pop-Location;$env:PATH=$savedPath}
Write-Host "$($records.Count) real terminal sessions completed; evidence=$output"
