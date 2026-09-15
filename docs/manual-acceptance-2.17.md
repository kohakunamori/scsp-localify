# SCSP Localify 2.17 manual acceptance

Reviewed: 2026-09-15.

Only rendered/interactive behavior is delegated to manual acceptance. Static target resolution, build reproducibility, package integrity, startup/hook installation, trace attribution and persistent cleanup remain automated engineering gates.

Run manual checks only on a disposable/test SCSP 2.17 setup that you can restore.

## Public manual-test setup

The private integration/automation harness used during development is intentionally not part of this public repository. Public verification should use only the artifacts and controls present here:

1. build `build-full-2.17.bat`;
2. load the resulting `scsp_localify_plugin.dll` with your compatible loader;
3. enable only the feature group being tested;
4. perform the matching check below;
5. restore your own DLL/configuration after the test.

Do not publish game paths, account information, startup arguments, tokens, authentication logs, or details of private integration environments when reporting results.

The test groups use existing supported JSON/GUI controls:

- A/F: baseline full-plugin configuration with diagnostic tracing; interaction remains entirely in the game/GUI.
- B: additionally sets `3DResolutionScale=0.75` and `blockOutOfFocus=true`. B is optional because these paths have automated runtime coverage. On current 2.17, 3D scale is owned by URP `UniversalRenderPipelineAsset.renderScale`; FPS/VSync and 3D scale also expose live GUI controls/readback. Their explanations are bilingual `(?)` hover tooltips rather than permanent inline text.
- C: enables base free camera, `allowSameIdol=true`, and the costume-save baseline. B6 owns the remigrated regular-Live/MV duplicate-idol consumers and the accepted duplicate-idol per-slot costume isolation fix; MV-unit override and forced separated vocal remain explicit GUI toggles because their payload/song compatibility is interaction-specific.
- D: tests the maintained SCSP 2.17 dress-unlock owner plus the costume-save baseline. Keep story-unlock disabled; hidden-costume and auto-apply remain explicit GUI toggles.
- E: enables `magicacloth_override`; character-parameter editor remains an explicit GUI toggle.
- G: tests the full-plugin story-unlock owner. Keep dress unlock disabled so the two ownership-sensitive paths are validated separately.

After each group, report only PASS/FAIL and the failing subfeature. Restore your own test environment after each run.

## Minimal grouped checks

- A — localization/story: readable Simplified Chinese with no tofu; open a translated story (preferred `s61_10211001_00`) and confirm translated dialogue renders and advances.
- B — display/focus: **optional visual sanity only**. FPS/VSync has setter/getter plus measured frame-progression validation; configured start resolution is runtime-proven; 3D scale is migrated to/read back from the SCSP 2.17 URP owner; focus-loss behavior is runtime-proven. In the GUI, hover the `(?)` beside Frame Rate Limit, VSync and 3D Render Scale for Chinese+English explanations. Report B only for a visible/UX defect or if runtime readback disagrees with the requested setting.
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
