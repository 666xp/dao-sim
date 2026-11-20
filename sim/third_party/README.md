# Vendored third-party sources

Pinned copies so configure/build does **not** need GitHub (FetchContent removed).

| Package | Version | Path |
|---------|---------|------|
| IXWebSocket | v11.4.4 | `IXWebSocket/` |
| Catch2 | v3.5.4 | `Catch2/` |

Upstream docs/tests/.github trees were trimmed to keep the repo small; CMake + library sources remain enough to build offline.

Do not edit these trees for product features; bump by replacing the directory with a new tagged release.
