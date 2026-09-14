$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent $PSScriptRoot
$hook = Get-Content -LiteralPath (Join-Path $Root 'src\hook.cpp') -Raw

foreach ($required in @(
    'int sameCharaCount = 0;',
    'if (it != savedCostumes.end() && sameCharaCount <= 1)',
    'else if (it != savedCostumes.end() && sameCharaCount > 1)',
    'costume-cache: duplicate idol detected; preserving per-slot costume',
    'if (g_overrie_mv_unit_idols)',
    'overridenMvUnitIdols[i].ApplyTo(item, true)'
)) {
    if ($hook.IndexOf($required, [StringComparison]::Ordinal) -lt 0) {
        throw "Missing duplicate-idol costume-isolation invariant: $required"
    }
}

$cacheApply = $hook.IndexOf('if (it != savedCostumes.end() && sameCharaCount <= 1)', [StringComparison]::Ordinal)
$slotApply = $hook.IndexOf('overridenMvUnitIdols[i].ApplyTo(item, true)', [StringComparison]::Ordinal)
if ($cacheApply -lt 0 -or $slotApply -lt 0 -or $cacheApply -ge $slotApply) {
    throw 'Explicit per-slot MV override must remain after the ambiguous CharaId costume-cache pass.'
}

Write-Host 'Duplicate-idol costume isolation checks: PASS'
