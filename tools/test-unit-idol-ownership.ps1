$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent $PSScriptRoot
$hook = Get-Content -LiteralPath (Join-Path $Root 'src\hook.cpp') -Raw
$gui = Get-Content -LiteralPath (Join-Path $Root 'src\scgui\scGUILoop.cpp') -Raw
$std = Get-Content -LiteralPath (Join-Path $Root 'src\stdinclude.cpp') -Raw

$dangerous = @(
    @{ Name = 'lastSavedCostume shallow assignment'; Pattern = 'lastSavedCostume\s*=\s*[^=]' ; Text = $hook },
    @{ Name = 'override slot shallow assignment'; Pattern = 'overridenMvUnitIdols\s*\[[^\]]+\]\s*=\s*[^=]' ; Text = $gui },
    @{ Name = 'saved costume shallow assignment'; Pattern = 'savedCostumes\s*\[[^\]]+\]\s*=\s*[^=]' ; Text = $hook }
)
foreach ($check in $dangerous) {
    if ([regex]::IsMatch($check.Text, $check.Pattern)) {
        throw "Unsafe UnitIdol ownership regression: $($check.Name)"
    }
}

foreach ($required in @(
    'lastSavedCostume.CopyFrom(data)',
    'overridenMvUnitIdols[i].CopyFrom(lastSavedCostume)',
    'overridenMvUnitIdols[i].CopyFrom(parsed)',
    'savedCostumes[data.CharaId].CopyFrom(data)',
    'UnitIdol::UnitIdol(const UnitIdol& other)',
    'UnitIdol& UnitIdol::operator=(const UnitIdol& other)',
    'UnitIdol::UnitIdol(UnitIdol&& other) noexcept',
    'UnitIdol& UnitIdol::operator=(UnitIdol&& other) noexcept',
    'void UnitIdol::CopyFrom(const UnitIdol& other)',
    'copiedAccessoryIds = new int[copiedAccessoryIdsLength]'
)) {
    if (($hook + $gui + $std).IndexOf($required, [StringComparison]::Ordinal) -lt 0) {
        throw "Missing UnitIdol ownership invariant: $required"
    }
}

Write-Host 'UnitIdol ownership checks: PASS'
