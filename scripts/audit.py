#!/usr/bin/env python3
"""Phase-4 audit: simulation output path only (UI copy excluded)."""
from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def gate(pass_: bool, name: str, value: float, thr: float) -> bool:
    print(f"{'PASS' if pass_ else 'FAIL'}: {name} {value:g} (threshold {thr:g})")
    return pass_


def main() -> int:
    ok = True
    out_cpp = (ROOT / "sim/src/output.cpp").read_text(encoding="utf-8")
    lit = 0
    for line in out_cpp.splitlines():
        s = line.strip()
        if s.startswith("#include") or s.startswith("//"):
            continue
        # string literals in code (ignore empty)
        lit += len(re.findall(r'"[^"]*"', line))
    ok &= gate(lit == 0, "audit_output_strings", lit, 0)

    forbidden = 0
    pat = re.compile(r"conscious|意识开关|semantic_map|neuron_to_word")
    for p in (ROOT / "sim/src").rglob("*"):
        if p.suffix not in {".cpp", ".h", ".hpp", ".cc"}:
            continue
        txt = p.read_text(encoding="utf-8", errors="ignore")
        forbidden += len(pat.findall(txt))
    ok &= gate(forbidden == 0, "audit_forbidden_semantics", forbidden, 0)

    text_fields = 0
    for p in [ROOT / "sim/src/telemetry.h", ROOT / "sim/src/telemetry.cpp"]:
        txt = p.read_text(encoding="utf-8")
        if re.search(r"\bstd::string\b", txt) or re.search(r"\bchar\s+\w+\s*\[", txt):
            text_fields += 1
    ok &= gate(text_fields == 0, "audit_telemetry_no_text", text_fields, 0)

    ok &= gate(ok, "phase4_audit_complete", 1 if ok else 0, 1)
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
