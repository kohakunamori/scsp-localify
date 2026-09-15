<div align="center">

# SCSP-localify

简体中文 | [English](readme_EN.md)

《偶像大师 闪耀色彩 棱镜之歌》（SCSP）DMM 版本地化/功能扩展插件。

**使用第三方插件可能违反游戏服务条款。由插件使用造成的账号或数据风险由使用者自行承担。**

</div>

## 项目状态

本 fork 基于上游 `scsp-localify` 持续维护，并针对 **SCSP 2.17** 补充了兼容性迁移、完整插件构建、本地化专用构建和静态兼容性探针。

当前 `main` 同时保留两类构建路径：

- **标准/兼容构建**：`generate.bat` + `build/ImasSCSP-localify.sln`，输出可直接作为代理 DLL 使用的 `version.dll`；GitHub Actions 也使用这条路径打包公开 artifact。
- **SCSP 2.17 维护构建**：
  - `build-full-2.17.bat`：完整插件，输出 `scsp_localify_plugin.dll`；
  - `build-lite.bat`：仅本地化功能的较小插件，输出 `scsp_localify_plugin.dll`；
  - `build-probe.bat`：2.17 方法/ICall 兼容性探针。

`scsp_localify_plugin.dll` 是插件形式的开发/集成目标，需要兼容的加载方式；**不要简单把它重命名成 `version.dll`**。普通用户优先使用标准构建或项目公开发布/Actions artifact。

翻译数据来自公开仓库 [kohakunamori/SCSPTranslationData](https://github.com/kohakunamori/SCSPTranslationData)，并通过 `resources/schinese` submodule 固定版本。

本公开仓库只维护插件源码、公开资源和构建资料。不要提交账号信息、启动参数、Token、日志、抓包、游戏文件、个人本地路径或私人项目资料。

## 功能概览

### 本地化

- Localify 主文本替换；
- `local2.json` UI/运行时字符串替换；
- 歌词替换；
- scenario/剧情 JSON 替换；
- 自定义字体与字体大小调整；
- 未翻译文本、歌词和 JSON Dump。

### 显示与性能

- 帧率上限；
- VSync / `vSyncCount`；
- 启动分辨率；
- 3D Render Scale；
- 窗口失焦不暂停；
- GUI 内实时调整部分性能选项。

SCSP 2.17 的 3D Render Scale 已迁移到当前实际使用的 URP `UniversalRenderPipelineAsset.renderScale` 路径；FPS/VSync 使用 Unity `Application.targetFrameRate` / `QualitySettings.vSyncCount` 的显式 setter/getter 路径。

### Live / MV

- Free Camera / FOV；
- 允许相同偶像多位置登场；
- MV 编队/偶像覆盖；
- 服装记录、替换与部分服装扩展功能；
- 强制服装/角色相关实验选项；
- 强制 separated vocal（仅适用于兼容歌曲/组合）。

### 角色与资源工具

- 角色身体参数实时编辑；
- MagicaCloth 参数调整；
- 运行时纹理提取/替换；
- 姿势数据复制/应用；
- 部分故事/服装解锁功能。

部分高级功能高度依赖当前客户端内部实现。README 中列出功能不代表所有组合都适用于所有游戏版本；2.17 的当前验证边界见 [docs/full-functionality-2.17.md](docs/full-functionality-2.17.md)。

## 快速使用

### 方法一：使用 GitHub Actions 构建产物

项目 CI 会生成 `scsp-localify` artifact，内容包括：

```text
version.dll
scsp-config.json
scsp_localify/
```

将这些文件放入游戏安装目录，使 `version.dll` 与 `imasscprism.exe` 位于同一级目录。

首次运行前建议先备份已有 `version.dll` / 配置文件。

启动游戏后，如果 `enableConsole=true` 且控制台正常出现，说明插件已加载。

> `showStartCommand` 会输出游戏启动参数，而启动参数可能包含敏感 Token。除非正在本机调试，否则不要开启，也不要把相关日志上传到 Issue/PR。

### 方法二：自行构建标准 version.dll

要求：

- Windows x64；
- Visual Studio 2022 / MSBuild；
- Python；
- Conan 2；
- CMake；
- Git。

克隆：

```bash
git clone --recursive https://github.com/kohakunamori/scsp-localify.git
cd scsp-localify
```

如果已经克隆：

```bash
git submodule update --init --recursive
```

生成依赖和工程：

```bat
generate.bat
```

然后用 Visual Studio 2022 打开：

```text
build/ImasSCSP-localify.sln
```

编译 `Release | x64`。输出位于：

```text
build/bin/x64/Release/version.dll
```

### 方法三：构建 SCSP 2.17 维护目标

完整插件：

```bat
build-full-2.17.bat
```

输出：

```text
build-full-2.17/bin/x64/Release/scsp_localify_plugin.dll
```

本地化专用插件：

```bat
build-lite.bat
```

输出：

```text
build-lite/bin/x64/Release/scsp_localify_plugin.dll
```

兼容性探针：

```bat
build-probe.bat
```

这些 2.17 目标主要用于当前兼容性开发、验证和集成，不等同于标准 `version.dll` 安装包。

## 翻译数据

默认翻译路径由：

```json
"localifyBasePath": "scsp_localify"
```

控制。

完整克隆本仓库后，翻译数据位于：

```text
resources/schinese/scsp_localify/
```

主要包括：

- `localify.json`
- `local2.json`
- `lyrics.json`
- `scenario/`
- 本地化资源包

如果 submodule 为空，请执行：

```bash
git submodule update --init --recursive
```

翻译仓库的详细使用与贡献说明见 [kohakunamori/SCSPTranslationData](https://github.com/kohakunamori/SCSPTranslationData)。

## 配置

配置文件：`scsp-config.json`。

下面列出常用配置。部分高级选项也可以在插件 GUI 中实时修改。

| 配置项 | 类型 | 默认/常用值 | 说明 |
| --- | --- | --- | --- |
| `enableConsole` | Bool | `true` | 显示插件控制台 |
| `showStartCommand` | Bool | `false` | 输出启动参数；**可能泄露 Token，不建议开启** |
| `localifyBasePath` | String | `scsp_localify` | 本地化数据目录 |
| `hotKey` | Char | `u` | `Ctrl + hotKey` 打开 GUI |
| `fontSizeOffset` | Int | `-3` | 字体大小偏移 |
| `customFontPath` | String | 见默认配置 | 自定义字体资源 |
| `dumpUntransLyrics` | Bool | `false` | Dump 未翻译歌词 |
| `dumpUntransLocal2` | Bool | `false` | Dump 未翻译 local2 文本 |
| `autoDumpAllJson` | Bool | `false` | Dump 游戏加载的 JSON |
| `blockOutOfFocus` | Bool | `true` | 阻止窗口失焦触发暂停 |
| `maxFps` | Int | `60` | 覆盖 Unity `targetFrameRate`；GUI 可实时修改 |
| `vSyncCount` | Int | 未设置 | 显式 VSync 覆盖；支持 Game/0/1/2 等语义 |
| `enableVSync` | Bool | `false` | 旧配置兼容；`true` 等价于请求 `vSyncCount=1` |
| `3DResolutionScale` | Float | `1.0` | 3D 渲染倍率；2.17 使用 URP renderScale |
| `startResolution` | Object | `1280x720` | 启动窗口宽高/全屏设置 |
| `baseFreeCamera.enable` | Bool | `false` | Free Camera |
| `baseFreeCamera.moveStep` | Float | `50` | Free Camera 移动速度 |
| `baseFreeCamera.mouseSpeed` | Float | `35` | 鼠标视角速度 |
| `allowSameIdol` | Bool | `false` | 允许相同偶像重复登场 |
| `saveAndReplaceCostumeChanges` | Bool | `false` | 保存/替换服装变化 |
| `unlockAllDress` | Bool | `false` | 服装解锁；属于高版本敏感功能 |
| `unlockPIdolAndSCharaEvents` | Bool | `false` | 故事/事件解锁相关功能 |
| `magicacloth_override` | Bool | `false` | 启用 MagicaCloth 参数覆盖 |
| `diagnosticFileTrace` | Bool | `false` | 开发诊断日志，不建议日常启用 |

`extraAssetBundlePaths` 等旧配置仍可能为了历史兼容被解析，但已经不是推荐配置入口。

## GUI

默认按：

```text
Ctrl + U
```

打开插件 GUI（取决于 `hotKey` 配置）。

GUI 包含或可访问：

- 性能/FPS/VSync/3D Render Scale；
- Free Camera；
- Live/MV 编队与服装相关功能；
- 角色参数；
- MagicaCloth；
- 资源提取/替换；
- 姿势工具；
- 诊断/开发选项。

涉及实时 Unity 对象的功能应在游戏进入对应场景后再使用。

## Free Camera

配置：

```json
{
  "baseFreeCamera": {
    "enable": true,
    "moveStep": 50,
    "mouseSpeed": 35
  }
}
```

默认操作：

| 操作 | 按键 |
| --- | --- |
| 前/后/左/右 | `W / S / A / D` |
| 上移 | `Alt`（旧配置可能仍为 `Space`） |
| 下移 | `Ctrl` |
| 复位 | `R` |
| 键盘转视角 | 方向键 |
| 鼠标转视角 | 右键按住，或切换鼠标模式 |
| FOV | `Q / E` 或鼠标滚轮 |

游戏版本更新可能改变 Camera/Transform 内部路径，因此 Free Camera 属于需要实际场景验证的功能。

## Live / MV 使用提示

### 相同偶像

启用：

```json
"allowSameIdol": true
```

当前 2.17 已针对常规 Live 与 MV 的重复偶像 consumer 做过迁移。重复偶像搭配不同服装时，插件使用按槽位的数据避免简单按角色 ID 覆盖全部位置。

### MV Unit Override

GUI 中可以保存/编辑各 Slot 的偶像与服装数据。

对于 5 人 MV，实际消费的是 Slot 0–4；更多槽位不会因为存在配置就自动被 5 人 MV 使用。

直接手改 JSON 时，请先备份数据并保持字段结构正确。

### Forced Separated Vocal

只建议在确认歌曲/组合支持时开启。对不兼容资源强制开启可能导致异常或卡住。

## 文本 Dump 与翻译

### localify.json

主 Localify 文本表。

### local2.json

不经过主 Localify 表的 UI/运行时文本。

### lyrics.json

歌词字符串映射。

### scenario

剧情/场景 JSON。

Dump 新原文后，请优先向 [SCSPTranslationData](https://github.com/kohakunamori/SCSPTranslationData) 提交数据/翻译，而不是把个人 Dump、日志或账号相关文件提交到本插件仓库。

翻译贡献者和翻译 Agent 请优先遵循 SCSPTranslationData 的 [AGENTS.md](https://github.com/kohakunamori/SCSPTranslationData/blob/TransData/AGENTS.md) 与 [CONTRIBUTING.md](https://github.com/kohakunamori/SCSPTranslationData/blob/TransData/CONTRIBUTING.md)。翻译仓库已经公开 SCSP 2.17.0 主 `localizetext` 的 5,631 tables / 138,036 source rows current-source snapshot、可验证 hash 与独立 coverage audit（当前 138,036/138,036 mapped、0 missing、0 actionable），同时保留术语/姓名表、Translation Memory、统一 QA、Agent batch/result schema、社区质量 Backlog、strict exact-source canonicalization 与 CI；GitHub 侧还提供翻译质量 / source update Issue 表单和 PR 模板，批量翻译与当前 localify 原文核验不需要依赖任何私人工作环境。

## 纹理提取和替换

提取功能会把符合筛选条件的纹理写入插件指定的 Dump 目录。

替换时，把与目标纹理名称匹配的文件放入：

```text
scsp_localify/textures/
```

资源提取可能产生大量文件，请不要把个人 Dump 结果直接提交到 Git。

## MagicaCloth

MagicaCloth 参数可通过 GUI 调整；配置文件中的 `magicacloth_* ` 值主要用于初始化。

属性含义可参考 Magica Cloth 官方资料。使用极端参数可能造成物理模拟异常，建议一次只修改少量参数并保留可恢复配置。

## 2.17 兼容性说明

当前仓库维护了一套公开的 2.17 静态/构建验证资料：

- [完整功能矩阵](docs/full-functionality-2.17.md)
- [手工交互检查表](docs/manual-acceptance-2.17.md)

目前已经有较强验证的 2.17 路径包括本地化基础链路、FPS/VSync、启动分辨率、URP 3D Render Scale、失焦控制，以及部分 Live/MV/Free Camera/重复偶像路径。

服装、故事解锁、角色参数、MagicaCloth、资源提取/姿势等高交互功能仍应按具体游戏版本和场景验证。不要仅因为代码能编译或 Hook 地址能解析，就假定所有功能在未来版本仍然可用。

## 开发

### 标准构建

```bat
generate.bat
```

再编译 `build/ImasSCSP-localify.sln`。

### 2.17 静态审计

```bash
python tools/audit_full_functionality.py
```

### 2.17 完整构建

```bat
build-full-2.17.bat
```

### 非游戏进程加载 Smoke Test

仓库包含：

```text
tools/smoke-full-plugin-non-game.ps1
```

用于验证 DLL 在非目标进程中加载/卸载时不会启动游戏 Hook 初始化。

## 安全与隐私

公开 Issue/PR/日志中请务必移除：

- DMM/游戏账号信息；
- Token、Cookie、启动参数；
- 本地用户名和绝对路径；
- TLS 私钥/证书；
- 抓包中的认证信息；
- 官方游戏资源或完整客户端文件。

`showStartCommand` 尤其容易把敏感启动参数输出到日志，默认保持关闭。

## 上游

本项目基于 [chinosk6/scsp-localify](https://github.com/chinosk6/scsp-localify) 持续维护。

翻译数据：

- [kohakunamori/SCSPTranslationData](https://github.com/kohakunamori/SCSPTranslationData)
- 上游社区数据：[ShinyGroup/SCSPTranslationData](https://github.com/ShinyGroup/SCSPTranslationData)

## License

详见 [LICENSE](LICENSE)。
