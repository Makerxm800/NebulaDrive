// build.rs — compiles the vendored SQLite source into a single .o that gets
// linked into the final binary.
//
// Requirements:
//   - On the Windows target: `zig` (resolved exactly like bin/zig-link.sh).
//   - Otherwise: a `cc`/`gcc` toolchain (or $CC).
//
// The output object lives in OUT_DIR. It is rebuilt only when sqlite3.c is
// newer than the existing object.

use std::env;
use std::fs;
use std::path::{Path, PathBuf};
use std::process::Command;

fn main() {
    let manifest_dir = PathBuf::from(env::var("CARGO_MANIFEST_DIR").unwrap());
    let sqlite_src = manifest_dir.join("../vendor/sqlite/sqlite3.c");
    if !sqlite_src.is_file() {
        panic!(
            "sqlite3.c not found at {} — expected the repo's vendored copy at pc/vendor/sqlite/sqlite3.c",
            sqlite_src.display()
        );
    }
    let out_dir = PathBuf::from(env::var("OUT_DIR").unwrap());
    let object = out_dir.join("sqlite3.o");

    // Re-check whether sqlite3.c changed since the build script last ran.
    println!("cargo:rerun-if-changed={}", sqlite_src.display());

    if !needs_rebuild(&sqlite_src, &object) {
        println!("cargo:rustc-link-arg={}", object.display());
        return;
    }

    let target = env::var("TARGET").unwrap();
    compile(&target, &sqlite_src, &object);

    // Re-print the link directive every build — cargo caches build-script
    // output between runs, so an empty rerun must not drop the link arg.
    println!("cargo:rustc-link-arg={}", object.display());
}

fn needs_rebuild(source: &Path, object: &Path) -> bool {
    if !object.exists() {
        return true;
    }
    let src_mtime = fs::metadata(source).and_then(|m| m.modified()).unwrap_or(std::time::SystemTime::UNIX_EPOCH);
    let obj_mtime = fs::metadata(object).and_then(|m| m.modified()).unwrap_or(std::time::SystemTime::UNIX_EPOCH);
    obj_mtime < src_mtime
}

fn compile(target: &str, source: &Path, object: &Path) {
    if target.contains("windows") {
        let zig = find_zig().unwrap_or_else(|| {
            panic!("zig not found. Set ZIG_PATH or install zig, then rebuild.")
        });
        // Rust triples (e.g. "x86_64-pc-windows-gnu") are not zig triples
        // (e.g. "x86_64-windows-gnu") — strip the "-pc-" prefix.
        let zig_target = target.replace("-pc-", "-");
        let status = Command::new(&zig)
            .arg("cc")
            .arg("-O2")
            .arg("-c")
            .arg(format!("--target={zig_target}"))
            .arg(source)
            .arg("-o")
            .arg(object)
            .status()
            .unwrap_or_else(|e| panic!("failed to run zig cc ({:?}): {e}", zig));
        assert!(status.success(), "zig cc failed with {status}");
    } else {
        let cc = env::var("CC").unwrap_or_else(|_| "cc".to_string());
        let status = Command::new(&cc)
            .args(["-O2", "-c"])
            .arg(source)
            .arg("-o")
            .arg(object)
            .status()
            .unwrap_or_else(|e| panic!("failed to run {cc}: {e}"));
        assert!(status.success(), "{cc} failed with {status}");
    }
}

fn find_zig() -> Option<String> {
    if let Ok(path) = env::var("ZIG_PATH") {
        if !path.is_empty() && Path::new(&path).is_file() {
            return Some(path);
        }
    }
    if let Ok(path) = which("zig") {
        return Some(path);
    }
    // pip-packaged ziglang (pip install ziglang)
    let script = "import os, ziglang; print(os.path.join(os.path.dirname(ziglang.__file__), 'zig'))";
    if let Ok(out) = Command::new("python3").args(["-c", script]).output() {
        if out.status.success() {
            let path = String::from_utf8_lossy(&out.stdout).trim().to_string();
            if !path.is_empty() && Path::new(&path).is_file() {
                return Some(path);
            }
        }
    }
    None
}

fn which(name: &str) -> Result<String, ()> {
    // Fallback `which` implementation: scan PATH and test executability.
    let path = env::var("PATH").map_err(|_| ())?;
    for dir in env::split_paths(path.as_str()) {
        let candidate = dir.join(name);
        if candidate.is_file() {
            if let Ok(metadata) = fs::metadata(&candidate) {
                #[cfg(unix)]
                {
                    use std::os::unix::fs::PermissionsExt;
                    if metadata.permissions().mode() & 0o111 != 0 {
                        return Ok(candidate.to_string_lossy().to_string());
                    }
                }
                #[cfg(not(unix))]
                {
                    return Ok(candidate.to_string_lossy().to_string());
                }
            }
        }
    }
    Err(())
}
