# NebulaDrive — Rust CLI

`nebuladrive` is a dependency-free Rust port of the NebulaDrive PC app
(`pc/main.cpp`). It is a local-first, no-server file storage CLI: files live
in one plain folder, metadata lives in a vendored SQLite database. Like the
original C++ app, it is **not** an Electron/native GUI — that is a separate
track (`gui_app.cpp`) and is intentionally out of scope here.

## Build

Requires: Rust ≥ 1.70 (built with 1.88.0 — the repo was verified with it),
plus **zig ≥ 0.11** as the linker driver for Windows (any `zig` on `PATH`,
`$ZIG_PATH`, or pip-installed `ziglang`). No crates.io access is needed — the
vendored `sqlite3.c` is compiled by `build.rs` and the binary links SQLite
directly.

```sh
cd pc/rust

# Native (Linux/macOS/Windows SDK-less via zig)
cargo build --release

# Windows .exe (x86_64-pc-windows-gnu) — this is the main target
cargo build --release --target x86_64-pc-windows-gnu
# binary: target/x86_64-pc-windows-gnu/release/nebuladrive.exe
```

The Windows target is wired up through `.cargo/config.toml` + `bin/zig-link.sh`
(zig as the linker), so no MinGW installation is required. Building from any
other cwd requires `--manifest-path`; the linker path is crate-relative, so run
cargo from `pc/rust`.

Output:

- `target/release/nebuladrive` (native)
- `target/x86_64-pc-windows-gnu/release/nebuladrive.exe` (Windows)

## Use

```sh
nebuladrive [--path <data-dir>] [--version] [--help]
nebuladrive            # interactive shell
```

Default data dir: `./NebulaDriveData` (the same default as the C++ app).

Commands (same names/behavior as `main.cpp`):

```
status  list  import <path>  search <query>  mkdir <folder>  move <id> [folder]
folders  favorite <id>  delete <id>  restore <id>  backups  backup  trash  emptytrash
vacuum  dbinfo  history  help  quit
```

## Data layout

```
<data-dir>/
  nebuladrive.db      # SQLite (WAL, synchronous=NORMAL, foreign keys on)
  files/              # imported files, with folder structure preserved
  trash/              # soft-deleted files (flat: "<id>_<filename>")
  backups/<name>/     # backup copies of files/
```

One file = one folder record + one file record. Deleting moves bytes into
`trash/` and sets `is_deleted=1`; restoring flips the flag back; `emptytrash`
removes the bytes and deletes the rows. Backups copy live files to
`backups/<name>/<path>` exactly as they appear in `files/`, preserving
relative subfolders. Names with spaces/non-ASCII characters are supported;
quote paths that contain spaces (`import "/some/path with spaces/file.txt"`).
