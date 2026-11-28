# NebulaDrive — command-line tool (older)

> **This is not the NebulaDrive app.** The app for Windows and macOS is the **Desktop App** — see [`../desktop`](../desktop/README.md) and the [latest release](../../../releases/latest).
>
> This folder is an earlier experiment, kept only for its small terminal tool (`NebulaDriveCLI`). The Windows window (`NebulaDrive.exe` built from this folder) was never released and has been replaced by the Desktop App.


It manages the same kind of private, local library as the web app (`../index.html`): your files stay on your computer — no servers, no subscriptions.

There are two builds, sharing one codebase:

| Target | Description | Status |
|---|---|---|
| **NebulaDriveCLI** (`NebulaDriveCLI`) | Terminal UI — a scriptable, cross-platform tool | Works on Windows, Linux, macOS |
| **NebulaDrive** (`NebulaDrive.exe`) | An early native Windows window (Win32 + GDI+) | Experiment, never released — use the Desktop App instead |

## What it does

- 📂 Files, folders, move, rename, search (folder + file level)
- ⭐ Favourites (star/unstar)
- 🗑️ Trash with restore and a true "empty trash" (bytes + records deleted)
- 💾 Backups — timestamped copies of your whole library under `backups/`
- 📊 Stats and an activity history log
- 🔐 Local-only: everything is a plain folder (`./NebulaDriveData` or `%APPDATA%\NebulaDrive`)
  you can back up, move or hand to another machine

## Requirements

| Tool | Version |
|---|---|
| CMake | 3.22+ |
| C++20 compiler | MSVC 2022 **or** MinGW-w64 64-bit (Windows) · GCC/Clang (Linux/macOS, CLI only) |
| SQLite | bundled (`vendor/sqlite/`, amalgamation 3.54.1 — no network needed) |

> The CLI is portable. The Windows window is an old experiment and is not maintained.

## Building

### Windows (desktop app + CLI)

```powershell
# MSVC (Visual Studio 2022, "Desktop development with C++")
cmake -B build -S . -A x64
cmake --build build --config Release

# …or MinGW
cmake -B build -S . -G "MinGW Makefiles"
cmake --build build --config Release
```

Result: `build\NebulaDrive.exe` (desktop) and `build\NebulaDriveCLI.exe` (CLI).

### Linux / macOS (CLI only)

```bash
cmake -B build -S .
cmake --build build -j
```

Result: `build/NebulaDriveCLI`.

## Running

### CLI

```bash
./NebulaDriveCLI --path ./NebulaDriveData   # optional path; this is the default
```

Run `help` inside the shell for the full command list:

```
list · import · search · mkdir · move <id> [folder] · delete · restore
favorite · backup · backups · trash · emptytrash · status · history · quit
```

## Files

```
pc/
├── CMakeLists.txt          # modern build (GUI + CLI + install rules + CPack)
├── main.cpp                # CLI entry point
├── gui_app.cpp             # desktop shell (Win32, Windows only)
├── nebula_ui.{h,cpp}       # GDI+ rendering toolkit (Windows only)
├── database.{h,cpp}        # SQLite layer (shared)
├── storage_handler.{h,cpp} # files/folders/trash/backups (shared)
├── NebulaDrive.rc          # version info
├── nebula.ico              # app icon
├── vendor/sqlite/          # vendored SQLite 3.54.1 amalgamation
├── .gitignore
└── README.md               # this file
```

## Status

- ✅ CLI builds and passes a full end-to-end run (import → folders → move →
  trash → restore → backup → empty-trash → status) on Linux — **built and
  verified with CMake + GCC 12**.
- 🖥️ Windows window: an early experiment that was never compiled or released. It has been replaced by the Desktop App (see `../desktop`).

## License

See `../LICENSE`. SQLite is in the public domain (see `vendor/sqlite/sqlite3.h`).
