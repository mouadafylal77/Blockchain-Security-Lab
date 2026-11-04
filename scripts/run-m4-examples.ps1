$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$savedPath = $env:PATH
Push-Location $repoRoot
try {
    $env:PATH = "C:\msys64\ucrt64\bin;$savedPath"
    $binary = Join-Path $repoRoot 'build\ucrt64\blockchain_sim.exe'
    if (-not (Test-Path -LiteralPath $binary)) { throw 'Construire avec scripts/test-ucrt64.ps1 avant ces exemples.' }
    $summary = @()
    foreach ($name in @('attack_51','selfish_mining','block_withholding','sybil')) {
        & $binary --config "configs/m4/${name}_calibration.json" --output "results/raw/m4/${name}_calibration"
        if ($LASTEXITCODE -ne 0) { throw "Calibration echouee : $name" }
        foreach ($detector in @('fixed','adaptive')) {
            foreach ($mode in @('baseline','attacked','protected')) {
                $scenario = "${name}_${detector}_${mode}"
                & $binary --config "configs/m4/$scenario.json" --output "results/raw/m4/$scenario"
                if ($LASTEXITCODE -ne 0) { throw "Scenario echoue : $scenario" }
                $result = Get-Content -LiteralPath "results/raw/m4/$scenario/results.json" -Raw | ConvertFrom-Json
                $summary += [pscustomobject]@{
                    attack=$name; detector=$detector; mode=$mode; seed=$result.config.seed
                    success=$result.attack.success; alerts=$result.security.alerts.Count
                    tp=$result.security.evaluation.tp; fp=$result.security.evaluation.fp
                    fn=$result.security.evaluation.fn; tn=$result.security.evaluation.tn
                    precision=$result.security.evaluation.precision; recall=$result.security.evaluation.recall
                    f1=$result.security.evaluation.f1; latency_ticks=$result.security.evaluation.latency_ticks
                    payment_loss=$result.security.economic.payment_loss_coin_units
                    pool_reward=$result.security.economic.pool_reward_units
                    active_adversary_share=$result.attack.active_block_share_at_target
                    messages=$result.metrics.messages_sent; events=$result.metrics.events_processed
                    operations=$result.security.overhead.feature_evaluations
                    delayed_messages=$result.security.overhead.deferred_messages
                    uncredited_shares=$result.security.overhead.shares_uncredited
                    reconnections=$result.security.overhead.reconnections
                    accepted_at=$result.attack.merchant_accepted_at
                }
            }
        }
    }
    $summary | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath 'results/raw/m4/comparison.json' -Encoding utf8
    $summary | Select-Object attack,detector,mode,success,tp,fp,fn,latency_ticks,delayed_messages,uncredited_shares,reconnections | Format-Table -AutoSize
} finally {
    $env:PATH = $savedPath
    Pop-Location
}
