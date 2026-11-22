# NebulaDrive PC — Changelog

## 1.1.0 (2026-11-22) — modernized

### Build & packaging
- SQLite 3.54.1 vendored under `vendor/sqlite/` — **no network or external folder needed**
  (the previous build referenced a missing `sqlite/` folder and failed out of the box).
- Modern CMake (`3.22+`): shared `nebuladrive_core` library + `NebulaDrive` (GUI, Windows)
  and `NebulaDriveCLI` (cross-platform) targets, install rules and optional CPack package.
- CLI is now a first-class, cross-platform target (was orphaned by the build).
- Version bumped to 1.1.0 (`.rc`, CMake, database settings, CLI).

### Bug fixes
- **`copyFileWithProgress` silent truncation** (dead code, removed — files were silently
  cut to the last full 8 KB block).
- **`updateFile` never updated `parent_folder` for root moves** — files moved to the root
  folder kept their old parent path and showed up in the wrong folder. Now always bound.
- **Console mojibake**: `storage_handler.cpp` contained double-encoded UTF-8 (✓/✗ rendered
  as `âœ“`) — fixed.
- **Windows console mojibake in the CLI** — console code page set to UTF-8.
- **`Database::computeChecksum`** — dead, broken code (`std::chrono::clock_cast` doesn't
  exist in C++20) — removed; checksum now computed on file **contents** (FNV-1a), not size/mtime.
- **Empty trash didn't delete bytes** — `emptyTrash()` now removes the bytes on disk as
  well as the records (GUI and CLI).
- **Search wildcard injection** — search input is escaped (`ESCAPE '\\'`)
  so `%`/`_` match literally.
- **`isValidFileName`** now also rejects names Windows can't hold
  (trailing space/dot, device names `CON`/`NUL`/`COM1`…, etc.).
- **`resolveDataDir()`** — library now lives in `%APPDATA%\NebulaDrive`
  (portable installs with an existing `NebulaDriveData` next to the exe keep it).

### CLI improvements
- UTF-8 console output (emoji, box drawing) on Windows.
- `--path <dir>` / `-p`, `--version`, `--help`; a bare positional arg is the data dir.
- New `move` command (and `folders` already existed) — `move <id> [folder]`,
  empty/omitted folder moves to root; quotes supported for folder names with spaces.

### Code hygiene
- Removed dead code: `copyFileWithProgress`, `cleanFileName`, unused includes.
- Normalized line endings/encoding (CRLF + UTF-8 BOM → LF, no BOM) across the tree.
- CLI `emptytrash` now uses the shared `StorageHandler::emptyTrash()` (bytes + records).
