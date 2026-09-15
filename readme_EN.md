<div align="center">

# SCSP-localify

[简体中文](README.md) | English

Localization and feature-extension plugin for the DMM version of THE IDOLM@STER Shiny Colors: Song for Prism (SCSP).

**Third-party plugins may violate the game's terms of service. You are responsible for account and data risks resulting from their use.**

</div>

## Project status

This fork continues upstream `scsp-localify` development and carries additional **SCSP 2.17** compatibility work, a full 2.17 target, a localization-only target, and a compatibility probe.

The current `main` keeps two build families:

- **Standard/compatible build**: `generate.bat` + `build/ImasSCSP-localify.sln`, producing `version.dll`. GitHub Actions packages this path as the public artifact.
- **SCSP 2.17 maintenance targets**:
  - `build-full-2.17.bat`: full plugin, producing `scsp_localify_plugin.dll`;
  - `build-lite.bat`: smaller localization-only plugin, producing `scsp_localify_plugin.dll`;
  - `build-probe.bat`: compatibility probe for 2.17 methods and icalls.

`scsp_localify_plugin.dll` is a plugin-form development/integration target and needs a compatible loader. **Do not simply rename it to `version.dll`.** Regular users should prefer the standard build or public release/Actions artifact.

Simplified-Chinese data is maintained in the public [kohakunamori/SCSPTranslationData](https://github.com/kohakunamori/SCSPTranslationData) repository and pinned through the `resources/schinese` submodule.

This public repository is for plugin source, public resources, and build material only. Do not commit account data, startup arguments, tokens, logs, captures, game files, personal paths, or private-project material.

## Features

### Localization

- primary Localify text replacement;
- `local2.json` runtime/UI string replacement;
- lyrics replacement;
- scenario JSON replacement;
- custom font and font-size adjustment;
- untranslated text/lyrics/JSON dumping.

### Display and performance

- frame-rate override;
- VSync / `vSyncCount`;
- startup resolution;
- 3D render scale;
- block pause-on-focus-loss;
- live GUI controls for several performance options.

For SCSP 2.17, 3D render scale has been migrated to the active URP `UniversalRenderPipelineAsset.renderScale` owner. FPS/VSync uses explicit Unity `Application.targetFrameRate` and `QualitySettings.vSyncCount` setter/getter paths.

### Live / MV

- Free Camera / FOV;
- same-idol multi-position support;
- MV unit/idol overrides;
- costume save/replace and related costume options;
- separated-vocal override for compatible songs/units.

### Character and resource tools

- live character body-parameter editing;
- MagicaCloth tuning;
- runtime texture extraction/replacement;
- pose copy/apply tools;
- selected story/costume unlock functions.

Some advanced features are tightly coupled to current game internals. Their presence in the source does not imply that every combination is stable on every game version. See [docs/full-functionality-2.17.md](docs/full-functionality-2.17.md) for the current 2.17 validation boundary.

## Quick start

### Use the GitHub Actions artifact

CI packages:

```text
version.dll
scsp-config.json
scsp_localify/
```

Place them in the game directory so that `version.dll` is next to `imasscprism.exe`.

Back up an existing `version.dll` and configuration before replacing anything.

If `enableConsole=true`, a console appearing at startup is a simple indication that the plugin loaded.

> `showStartCommand` prints game startup arguments. Those arguments may contain sensitive tokens. Keep this option disabled unless you are debugging locally, and never upload raw startup logs to an Issue or PR.

### Build the standard version.dll

Requirements:

- Windows x64;
- Visual Studio 2022 / MSBuild;
- Python;
- Conan 2;
- CMake;
- Git.

Clone:

```bash
git clone --recursive https://github.com/kohakunamori/scsp-localify.git
cd scsp-localify
```

For an existing clone:

```bash
git submodule update --init --recursive
```

Generate dependencies/project files:

```bat
generate.bat
```

Open:

```text
build/ImasSCSP-localify.sln
```

and build `Release | x64`.

Output:

```text
build/bin/x64/Release/version.dll
```

### Build the SCSP 2.17 maintenance targets

Full plugin:

```bat
build-full-2.17.bat
```

Output:

```text
build-full-2.17/bin/x64/Release/scsp_localify_plugin.dll
```

Localization-only plugin:

```bat
build-lite.bat
```

Output:

```text
build-lite/bin/x64/Release/scsp_localify_plugin.dll
```

Compatibility probe:

```bat
build-probe.bat
```

These 2.17 targets are primarily for compatibility development, validation, and integration. They are not drop-in replacements for the standard `version.dll` package.

## Translation data

The default path is:

```json
"localifyBasePath": "scsp_localify"
```

A recursive clone provides translation data under:

```text
resources/schinese/scsp_localify/
```

It currently includes:

- `localify.json`;
- `local2.json`;
- `lyrics.json`;
- `scenario/`;
- localization resources.

If the directory is empty:

```bash
git submodule update --init --recursive
```

See [kohakunamori/SCSPTranslationData](https://github.com/kohakunamori/SCSPTranslationData) for translation usage and contribution instructions.

Translation contributors and translation agents should follow the public [AGENTS.md](https://github.com/kohakunamori/SCSPTranslationData/blob/TransData/AGENTS.md) and [CONTRIBUTING.md](https://github.com/kohakunamori/SCSPTranslationData/blob/TransData/CONTRIBUTING.md). The translation repository now provides public terminology/name data, translation-memory generation, repository QA, agent batch/result schemas, and CI without depending on private environments.

## Configuration

Configuration file: `scsp-config.json`.

| Key | Type | Default/common value | Description |
| --- | --- | --- | --- |
| `enableConsole` | Bool | `true` | Show plugin console |
| `showStartCommand` | Bool | `false` | Print startup args; **may expose tokens** |
| `localifyBasePath` | String | `scsp_localify` | Localization data directory |
| `hotKey` | Char | `u` | `Ctrl + hotKey` opens the GUI |
| `fontSizeOffset` | Int | `-3` | Font-size offset |
| `customFontPath` | String | see default config | Custom font resource |
| `dumpUntransLyrics` | Bool | `false` | Dump untranslated lyrics |
| `dumpUntransLocal2` | Bool | `false` | Dump untranslated local2 strings |
| `autoDumpAllJson` | Bool | `false` | Dump loaded game JSON |
| `blockOutOfFocus` | Bool | `true` | Suppress pause-on-focus-loss |
| `maxFps` | Int | `60` | Override Unity `targetFrameRate`; also live-editable in GUI |
| `vSyncCount` | Int | unset | Explicit VSync override |
| `enableVSync` | Bool | `false` | Legacy compatibility; `true` requests `vSyncCount=1` |
| `3DResolutionScale` | Float | `1.0` | 3D render multiplier; SCSP 2.17 uses URP renderScale |
| `startResolution` | Object | `1280x720` | Startup width/height/fullscreen |
| `baseFreeCamera.enable` | Bool | `false` | Enable Free Camera |
| `baseFreeCamera.moveStep` | Float | `50` | Movement speed |
| `baseFreeCamera.mouseSpeed` | Float | `35` | Mouse-look speed |
| `allowSameIdol` | Bool | `false` | Allow duplicate idols in supported Live/MV paths |
| `saveAndReplaceCostumeChanges` | Bool | `false` | Save/replace costume changes |
| `unlockAllDress` | Bool | `false` | Dress-unlock feature; version-sensitive |
| `unlockPIdolAndSCharaEvents` | Bool | `false` | Story/event unlock feature |
| `magicacloth_override` | Bool | `false` | Enable MagicaCloth overrides |
| `diagnosticFileTrace` | Bool | `false` | Development diagnostics |

Some legacy keys such as `extraAssetBundlePaths` may still be parsed for compatibility but are not recommended for new configuration.

## GUI

By default:

```text
Ctrl + U
```

opens the GUI, depending on `hotKey`.

The GUI exposes controls for:

- FPS/VSync/3D render scale;
- Free Camera;
- Live/MV idol and costume options;
- character parameters;
- MagicaCloth;
- asset extraction/replacement;
- pose tools;
- diagnostics.

Features that operate on live Unity objects should be used only after entering the corresponding game scene.

## Free Camera

Example:

```json
{
  "baseFreeCamera": {
    "enable": true,
    "moveStep": 50,
    "mouseSpeed": 35
  }
}
```

Default controls:

| Action | Key |
| --- | --- |
| Move | `W / S / A / D` |
| Up | `Alt` (older configs may still use `Space`) |
| Down | `Ctrl` |
| Reset | `R` |
| Keyboard look | arrow keys |
| Mouse look | hold right mouse button or toggle mouse-look mode |
| FOV | `Q / E` or mouse wheel |

Camera/Transform internals can change between game versions, so scene-level validation is still required after updates.

## Live / MV notes

### Same idol

Enable:

```json
"allowSameIdol": true
```

The current 2.17 implementation handles the regular-Live and MV duplicate-idol consumers and keeps per-slot costume data to avoid simply broadcasting the last costume for the same character ID to every position.

### MV unit override

The GUI can store/edit idol and costume data per slot.

A 5-member MV consumes Slot 0–4. Additional slots are not automatically used by a 5-member MV.

Back up data before manually editing JSON and keep the expected object structure intact.

### Forced separated vocal

Use only with known-compatible songs/units. Forcing this mode on unsupported content may lead to invalid behavior.

## Text dumping and localization

- `localify.json`: primary Localify text tables;
- `local2.json`: runtime/UI strings outside the primary table;
- `lyrics.json`: lyric mappings;
- `scenario/`: story/scenario JSON.

When adding translation data, prefer contributing it to [SCSPTranslationData](https://github.com/kohakunamori/SCSPTranslationData). Do not submit personal dumps, raw logs, or account-related data to this plugin repository.

## Texture extraction and replacement

Extraction writes matching textures to plugin dump directories.

For replacement, place matching texture files under:

```text
scsp_localify/textures/
```

Asset dumping can generate many files. Do not commit personal dump output to Git.

## MagicaCloth

MagicaCloth values can be changed through the GUI. `magicacloth_*` config keys mainly provide initialization values.

Extreme values can destabilize cloth simulation. Change a small number of parameters at a time and keep a recoverable configuration.

## SCSP 2.17 compatibility

Public 2.17 engineering notes:

- [full feature/compatibility matrix](docs/full-functionality-2.17.md)
- [manual interaction checklist](docs/manual-acceptance-2.17.md)

Strongly validated 2.17 paths currently include the base localization chain, FPS/VSync, startup resolution, URP 3D render scale, focus-loss control, and selected Live/MV/Free-Camera/same-idol paths.

Costume, story-unlock, character, MagicaCloth, extraction, and pose features remain more interaction- and version-sensitive. Successful compilation or method resolution alone is not proof that every feature will work on future game versions.

## Development

Standard build:

```bat
generate.bat
```

2.17 static audit:

```bash
python tools/audit_full_functionality.py
```

2.17 full build:

```bat
build-full-2.17.bat
```

Non-game loader smoke test:

```text
tools/smoke-full-plugin-non-game.ps1
```

## Security and privacy

Before posting an Issue, PR, screenshot, or log, remove:

- DMM/game account information;
- tokens, cookies, and startup arguments;
- local usernames and absolute paths;
- private keys/certificates;
- authentication captures;
- official game resources or complete client files.

`showStartCommand` is particularly likely to expose sensitive startup parameters and should remain disabled by default.

## Upstream

Based on [chinosk6/scsp-localify](https://github.com/chinosk6/scsp-localify).

Translation data:

- [kohakunamori/SCSPTranslationData](https://github.com/kohakunamori/SCSPTranslationData)
- upstream community data: [ShinyGroup/SCSPTranslationData](https://github.com/ShinyGroup/SCSPTranslationData)

## License

See [LICENSE](LICENSE).
