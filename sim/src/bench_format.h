#pragma once

#include <iostream>
#include <string>

namespace dao {

// Canonical Phase-4 light-panel line:
//   PASS|FAIL: <name> <value> (threshold <t>)
inline void bench_gate(bool pass, const std::string& name, double value, double threshold) {
  std::cout << (pass ? "PASS" : "FAIL") << ": " << name << " " << value << " (threshold "
            << threshold << ")\n";
}

inline void bench_note(const std::string& text) { std::cout << "NOTE: " << text << "\n"; }

}  // namespace dao
