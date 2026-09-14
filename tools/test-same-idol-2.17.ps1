$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent $PSScriptRoot
$hook = Get-Content -LiteralPath (Join-Path $Root 'src\hook.cpp') -Raw
$main = Get-Content -LiteralPath (Join-Path $Root 'src\main.cpp') -Raw

foreach ($required in @(
    'LiveUnitMemberChangeViewModel_sameIdolPredicate_hook',
    '<>c__DisplayClass16_2',
    '<.ctor>b__5',
    'same-idol: regular Live duplicate predicate bypassed',
    'LiveMvUnitMemberChangeViewModel_buildIdolViewModel_hook',
    '<>c__DisplayClass7_0',
    '<.ctor>b__4',
    'LiveMvIdolListIdolViewModel_IsInSameUnit_field',
    'same-idol: MV IsInSameUnit cleared',
    'LiveMVUnit_GetMemberChangeRequestData_hook',
    'MvUnitSlotGenerator_NewMvUnitSlot_hook'
)) {
    if ($hook.IndexOf($required, [StringComparison]::Ordinal) -lt 0) {
        throw "Missing SCSP 2.17 same-idol invariant: $required"
    }
}

if ($main.IndexOf('g_allow_same_idol = document["allowSameIdol"].GetBool();', [StringComparison]::Ordinal) -lt 0) {
    throw 'allowSameIdol config is not wired to g_allow_same_idol.'
}

Write-Host 'SCSP 2.17 same-idol source checks: PASS'
