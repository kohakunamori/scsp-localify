# SCSP Localify 2.17 manual acceptance

Reviewed: 2026-09-14.

Only rendered/interactive behavior is delegated to manual acceptance. Static target resolution, build reproducibility, package integrity, startup/hook installation, trace attribution and persistent cleanup remain automated engineering gates.

Run manual checks only on the paired offline/local 2.17 profile.

## One-command manual profiles

From `D:\Project\scsp-relive`, run one group at a time:

```powershell
.\tools\run-full-localify-manual-group.ps1 -Group C
```

Replace `C` with `A` through `G`. The wrapper builds a group-specific full-plugin package under `runtime/local/package-full-manual-<group>-2.17`, launches the normal offline/local client, waits up to 30 minutes by default, and enables `-AllowEarlyExit`: when the check is finished, close the game normally and the harness restores the pre-run DLL/config/unlock bytes automatically.

Profiles only preconfigure existing supported JSON controls; they do not invent new plugin configuration semantics:

- A/F: normal SafeSmoke ownership with diagnostic tracing; interaction remains entirely in the game/GUI.
- B: additionally sets `3DResolutionScale=0.75` and `blockOutOfFocus=true`. B is optional because these paths already have automated runtime PASS.
- C: enables base free camera, `allowSameIdol=true`, and the costume-save baseline. B6 owns the remigrated regular-Live/MV duplicate-idol consumers and the accepted duplicate-idol per-slot costume isolation fix; MV-unit override and forced separated vocal remain explicit GUI toggles because their payload/song compatibility is interaction-specific.
- D: uses `FullDressOnly`, enabling only the maintained SCSP 2.17 full-plugin dress-unlock owner plus the costume-save baseline. Story unlock remains disabled; hidden-costume and auto-apply remain explicit GUI toggles.
- E: enables `magicacloth_override`; character-parameter editor remains an explicit GUI toggle.
- G: uses `FullStoryOnly`, enabling only the full-plugin story-unlock owner. Dress unlock remains disabled. The offline Story/Costume owners stay disabled in every generated full profile.

After each group, report only PASS/FAIL and the failing subfeature. Engineering cleanup checks persistent state separately; do not manually reset or clean the repository or game SVN working copy.

## Minimal grouped checks

- A — localization/story: readable Simplified Chinese with no tofu; open a translated story (preferred `s61_10211001_00`) and confirm translated dialogue renders and advances.
- B — display/focus: **optional visual sanity only**. FPS/VSync, configured start resolution, 3D render scale, and focus-loss enabled/disabled behavior are already automated runtime PASS; report B only if you notice a visual/UX defect.
- C — Live/MV: **PASS on accepted B6 (user manual acceptance, 2026-09-14)**. Verified same-idol multi-position selection, five-same-idol/five-distinct-costume isolation, explicit Override-MV Slot 0–4 replay, free-camera movement/FOV/clip behavior, and forced separated vocal on a known-supported song/unit. For a 5-person MV, Slot 5–7 remain intentionally inactive.
- D — costume: hidden costumes, unlock-all-dress, auto-apply, and saved costume replacement into MV. Use the full-owner profile only.
- E — character/cloth: one obvious character body parameter apply/reset and one sane MagicaCloth override/reset.
- F — assets/pose: one texture replacement, one narrow asset extraction type, and pose capture/change/restore.
- G — story unlock: one normally locked local test story becomes available and enters normally. Use the full-owner profile only.

## Result format

A terse report is enough, for example:

```text
A PASS
B PASS
C FAIL: MV Slot 2 override not applied
D PASS
E PASS
F PASS
G PASS
```

A screenshot is needed only for a visual defect or ambiguous result. Build hashes and logs are collected separately.
