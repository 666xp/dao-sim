#!/usr/bin/env python3
"""Run phase0–3 benches + audit, emit web/public/lights.json for the traffic panel."""
from __future__ import annotations

import argparse
import json
import os
import re
import subprocess
import sys
from pathlib import Path

LINE_RE = re.compile(
    r"^(PASS|FAIL):\s+(\S+)\s+(\S+)(?:\s+\(threshold\s+([^)]+)\))?\s*$"
)

PHASE_OF = {
    "isolated_count": "0",
    "left_right_direct": "0",
    "sediment_non_dao_neighbor": "0",
    "all_cross_via_dao": "0",
    "long_delay_delivery": "0",
    "conduction_ok": "0",
    "dao_zc": "2",
    "dao_amp_ratio": "2",
    "dao_isolated_oscillation": "2",
    "dao_D_stable": "2",
    "field_modulation": "2",
    "fields_roche_integration": "2",
    "dao_D_stable_with_sigma": "2",
    "field_modulation_with_sigma": "2",
    "sigma_g_bounded": "2",
    "sigma_slow_control": "2",
    "lr_alternating_ratio": "3",
    "bitstream_nonzero": "3",
    "ones_density_band": "3",
    "ones_stats_similar": "3",
    "sequences_differ": "3",
    "output_nonpreset": "3",
    "ablation_A_dao_adapt": "3",
    "ablation_A_global_rate_ratio": "3",
    "ablation_B_noise_convergence": "3",
    "ablation_C_east_perturb": "3",
    "phase3_ablations": "3",
    "audit_output_strings": "4",
    "audit_forbidden_semantics": "4",
    "audit_telemetry_no_text": "4",
    "phase4_audit_complete": "4",
}


def parse_lines(text: str) -> list[dict]:
    out = []
    for raw in text.splitlines():
        m = LINE_RE.match(raw.strip())
        if not m:
            continue
        status, name, value, thr = m.group(1), m.group(2), m.group(3), m.group(4)
        try:
            v = float(value)
        except ValueError:
            v = value
        try:
            t = float(thr) if thr is not None else None
        except ValueError:
            t = thr
        out.append(
            {
                "status": status,
                "name": name,
                "value": v,
                "threshold": t,
                "phase": PHASE_OF.get(name, "?"),
                "line": raw.strip(),
            }
        )
    return out


def run(cmd: list[str], cwd: Path) -> str:
    print("+", " ".join(cmd), flush=True)
    p = subprocess.run(cmd, cwd=cwd, capture_output=True, text=True)
    text = (p.stdout or "") + (p.stderr or "")
    sys.stdout.write(p.stdout or "")
    if p.returncode != 0 and "audit" not in " ".join(cmd):
        # still harvest PASS/FAIL lines
        pass
    return text


def exe(root: Path, name: str) -> Path:
    for c in [
        root / "sim" / "build" / name,
        root / "sim" / "build" / "Debug" / f"{name}.exe",
        root / "sim" / "build" / "Release" / f"{name}.exe",
        root / "sim" / "build" / f"{name}.exe",
    ]:
        if c.exists():
            return c
    raise FileNotFoundError(name)


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--inject-fail", action="store_true", help="flip conduction_ok to FAIL for panel test")
    ap.add_argument("--out", default="web/public/lights.json")
    args = ap.parse_args()

    root = Path(__file__).resolve().parents[1]
    items: list[dict] = []

    # Audit (cross-platform)
    text = run([sys.executable, str(root / "scripts" / "audit.py")], root)
    items.extend(parse_lines(text))

    # Phase 0 validate
    try:
        v = exe(root, "sim_validate")
        items.extend(parse_lines(run([str(v), "42"], root)))
    except FileNotFoundError as e:
        print("missing", e, file=sys.stderr)

    benches = [
        (["sim_dao_bench", "42", "2000"],),
        (["sim_fields_bench", "42", "3000", "500"],),
        (["sim_sigma_bench", "42", "4000", "800"],),
        (["sim_transport_bench", "42", "3000"],),
        (["sim_output_bench", "3000", "400"],),
        (["sim_ablation_bench", "42", "4000"],),
    ]
    for args_cmd in benches:
        name = args_cmd[0][0]
        try:
            bin_path = exe(root, name)
            cmd = [str(bin_path)] + list(args_cmd[0][1:])
            items.extend(parse_lines(run(cmd, root)))
        except FileNotFoundError:
            print("skip missing", name, file=sys.stderr)

    if args.inject_fail:
        for it in items:
            if it["name"] == "conduction_ok":
                it["status"] = "FAIL"
                it["value"] = 0
                it["line"] = "FAIL: conduction_ok 0 (threshold 1)"

    # Deduplicate by name keeping last
    by_name = {}
    for it in items:
        by_name[it["name"]] = it
    final = list(by_name.values())
    final.sort(key=lambda x: (x["phase"], x["name"]))

    payload = {
        "ok": all(i["status"] == "PASS" for i in final),
        "items": final,
        "inject_fail": bool(args.inject_fail),
    }
    out = root / args.out
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(json.dumps(payload, indent=2), encoding="utf-8")
    print(f"wrote {out} ok={payload['ok']} n={len(final)}")
    return 0 if payload["ok"] and not args.inject_fail else (0 if args.inject_fail else 1)


if __name__ == "__main__":
    sys.exit(main())
