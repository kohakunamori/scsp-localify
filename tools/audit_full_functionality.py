#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "src"

def read(path: Path) -> str:
    return path.read_text(encoding="utf-8-sig")

def strip_line_comments(text: str) -> str:
    return "\n".join(line.split("//", 1)[0] for line in text.splitlines())

def parse_active_hooks(hook: str):
    lines = hook.splitlines()
    start = next(i for i, x in enumerate(lines) if "ADD_HOOK(SetResolution" in x)
    end = next(i for i, x in enumerate(lines[start:], start) if "tools::AddNetworkingHooks();" in x)
    active, disabled = [], []
    for i in range(start, end):
        s = lines[i].strip()
        if "ADD_HOOK" not in s:
            continue
        m = (
            re.search(r"ADD_HOOK_1\(([^)]+)\)", s)
            or re.search(r"ADD_HOOK\(([^,]+)", s)
            or re.search(r"ADD_HOOK_ADDR\([^,]+,[^,]+,[^,]+,\s*([^,]+)", s)
        )
        if not m:
            continue
        row = {"hook": m.group(1).strip(), "line": i + 1, "code": s}
        (disabled if s.startswith("//") else active).append(row)
    return active, disabled

def parse_probe_specs(probe: str):
    return [
        (m.group(1), m.group(2), m.group(3), m.group(4), int(m.group(5)))
        for m in re.finditer(
            r'\{"([^"]+)",\s*"([^"]*)",\s*"([^"]+)",\s*"([^"]+)",\s*(\d+)\}',
            probe,
        )
    ]

def find_method_target(clean_hook: str, name: str):
    pat = re.compile(
        rf"\b(?:auto|uintptr_t)\s+{re.escape(name)}_addr\s*=\s*"
        rf"(?:il2cpp_symbols(?:_logged)?::)?get_method_pointer\(\s*"
        r'"([^"]+)"\s*,\s*"([^"]*)"\s*,\s*"([^"]+)"\s*,\s*"([^"]+)"\s*,\s*(\d+)\s*\)',
        re.S,
    )
    ms = list(pat.finditer(clean_hook))
    if not ms:
        return None
    m = ms[-1]
    return (m.group(1), m.group(2), m.group(3), m.group(4), int(m.group(5)))

def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--json", type=Path)
    ap.add_argument("--markdown", type=Path)
    args = ap.parse_args()

    main_cpp = read(SRC / "main.cpp")
    hook_cpp = read(SRC / "hook.cpp")
    probe_cpp = read(SRC / "full_compat_probe.cpp")
    gui_cpp = read(SRC / "scgui" / "scGUILoop.cpp")
    readme = read(ROOT / "readme_EN.md")
    net_cpp = read(SRC / "tools" / "hook_networking.g.cpp")
    clean_hook = strip_line_comments(hook_cpp)

    globals_ = [
        {"name": m.group(1), "default": m.group(2).strip()}
        for m in re.finditer(
            r"^(?:bool|int|float|char|std::string)\s+(g_[A-Za-z0-9_]+)\s*=\s*([^;]+);",
            main_cpp,
            re.M,
        )
    ]
    config_keys = []
    for pat in [r'document\.HasMember\("([^"]+)"\)', r'ReadJsonKeyBinding\("([^"]+)"']:
        for m in re.finditer(pat, main_cpp):
            if m.group(1) not in config_keys:
                config_keys.append(m.group(1))

    gui_controls = []
    for i, line in enumerate(gui_cpp.splitlines(), 1):
        m = re.search(
            r'ImGui::(Checkbox|Button|CollapsingHeader|BeginTabItem|BeginTabBar|SliderFloat|InputFloat\d*|RadioButton|Combo)'
            r'\(\s*"([^"]+)',
            line,
        )
        if m:
            gui_controls.append({"line": i, "kind": m.group(1), "label": m.group(2), "code": line.strip()})

    active, disabled = parse_active_hooks(hook_cpp)
    probe_specs = parse_probe_specs(probe_cpp)
    probe_set = set(probe_specs)
    probe_icalls = set(
        re.findall(
            r'"(UnityEngine\.(?:Application|QualitySettings)::[^"]+)"',
            probe_cpp,
        )
    )

    inline = {
        "RefreshViewModels": ("PRISM.Adapters.dll", "PRISM.Adapters.CostumeChange", "CostumeChangeViewModel", "RefreshViewModels", 0),
        "ModifyPreview": ("PRISM.Adapters.dll", "PRISM.Adapters.CostumeChange", "CostumeChangeViewModel", "ModifyPreview", 1),
        "AssembleCharacter_ApplyParam": ("PRISM.Legacy.dll", "PRISM", "AssembleCharacter", "ApplyParam", 6),
    }

    hook_rows = []
    icall_targets = {
        "set_fps": "UnityEngine.Application::set_targetFrameRate(System.Int32)",
        "set_vsync_count": "UnityEngine.QualitySettings::set_vSyncCount(System.Int32)",
        "Unity_Quit": "UnityEngine.Application::Quit(System.Int32)",
    }
    for row in active:
        name = row["hook"]
        target = find_method_target(clean_hook, name) or inline.get(name)
        if target:
            kind, covered = "method", target in probe_set
        elif name == "LiveMVUnitMemberChangePresenter_initializeAsync_b_4_MoveNext":
            kind, covered = "nested_state_machine", True
        elif name in {
            "LiveUnitMemberChangeViewModel_sameIdolPredicate",
            "LiveMvUnitMemberChangeViewModel_buildIdolViewModel",
        }:
            kind, covered = "nested_same_idol_consumer", True
        elif name in icall_targets:
            target = icall_targets[name]
            kind, covered = "icall", target in probe_icalls
        elif name == "Subject_OnNext":
            kind, covered = "closed_generic_reflection", False
        else:
            kind, covered = "special", False
        hook_rows.append({**row, "kind": kind, "target": target, "probe_covered": covered})

    readme_functions = []
    in_functions = False
    for line in readme.splitlines():
        if line.strip() == "# Function List":
            in_functions = True
            continue
        if in_functions and line.startswith("# "):
            break
        if in_functions and line.strip().startswith("- "):
            readme_functions.append(line.strip()[2:])

    networking_types = re.findall(r"^DEFINE_GRPC_HOOK\(([^)]+)\);", net_cpp, re.M)
    networking_default = "__TOOL_HOOK_NETWORKING__" in read(ROOT / "premake5.lua")

    summary = {
        "readme_function_count": len(readme_functions),
        "global_flag_count": len(globals_),
        "config_key_count": len(config_keys),
        "gui_control_count": len(gui_controls),
        "active_main_hook_count": len(active),
        "disabled_main_hook_count": len(disabled),
        "probe_method_spec_count": len(probe_specs),
        "probe_icall_count": len(probe_icalls),
        "active_hook_probe_covered": sum(bool(x["probe_covered"]) for x in hook_rows),
        "active_hook_probe_open": sum(not bool(x["probe_covered"]) for x in hook_rows),
        "open_hook_names": [x["hook"] for x in hook_rows if not x["probe_covered"]],
        "networking_type_count": len(networking_types),
        "networking_generated_hook_count": len(networking_types) * 2,
        "networking_compiled_by_default": networking_default,
    }
    out = {
        "summary": summary,
        "readme_functions": readme_functions,
        "globals": globals_,
        "config_keys": config_keys,
        "gui_controls": gui_controls,
        "active_hooks": hook_rows,
        "disabled_hooks": disabled,
        "probe_specs": [
            {"assembly": x[0], "namespace": x[1], "class": x[2], "method": x[3], "argc": x[4]}
            for x in probe_specs
        ],
    }

    if args.json:
        args.json.parent.mkdir(parents=True, exist_ok=True)
        args.json.write_text(json.dumps(out, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")

    if args.markdown:
        lines = [
            "# SCSP Localify 2.17 full-functionality static inventory",
            "",
            "Generated by tools/audit_full_functionality.py.",
            "",
            "## Counts",
            "",
        ]
        for k, v in summary.items():
            lines.append(f"- {k}: {v}")
        lines += ["", "## Active hook probe coverage", "", "| Hook | Kind | Probe | Target |", "| --- | --- | --- | --- |"]
        for x in hook_rows:
            target = "" if not x["target"] else " / ".join(map(str, x["target"]))
            lines.append(f"| {x['hook']} | {x['kind']} | {'covered' if x['probe_covered'] else 'OPEN'} | {target} |")
        lines += ["", "## Disabled/commented hooks", ""]
        lines += [f"- {x['hook']}" for x in disabled]
        lines += ["", "## README-declared functions", ""]
        lines += [f"- {x}" for x in readme_functions]
        args.markdown.parent.mkdir(parents=True, exist_ok=True)
        args.markdown.write_text("\n".join(lines) + "\n", encoding="utf-8")

    print(json.dumps(summary, ensure_ascii=False, indent=2))
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
