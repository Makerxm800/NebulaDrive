#!/bin/bash
# The linker driver Rust uses when building for x86_64-pc-windows-gnu.
#
# Rust's default linker for that target (x86_64-w64-mingw32-gcc) is not
# available in restricted build environments, so we use zig as the linker
# driver instead. zig bundles a MinGW-compatible CRT and lld, which covers
# everything the windows-gnu target links against.
#
# rustc passes the full linker argv to this wrapper; anything zig does not
# accept directly (e.g. `-fno-use-linker-plugin`, MinGW-only library names
# like `-l:libpthread.a`) is filtered out — zig brings its own CRT.
#
# zig is resolved, in order, from: $ZIG_PATH, $PATH (`zig`), the pip-packaged
# `ziglang` module (`python3 -c "import ziglang"`).

set -u

if [ -n "${ZIG_PATH:-}" ] && [ -x "${ZIG_PATH:-}" ]; then
    ZIG="$ZIG_PATH"
elif command -v zig >/dev/null 2>&1; then
    ZIG="$(command -v zig)"
else
    ZIG="$(python3 -c 'import os, ziglang; print(os.path.join(os.path.dirname(ziglang.__file__), "zig"))' 2>/dev/null)"
fi

if [ -z "${ZIG:-}" ] || [ ! -x "$ZIG" ]; then
    echo "error: could not find zig. Set ZIG_PATH or install zig." >&2
    exit 1
fi

args=()
for a in "$@"; do
    case "$a" in
        # zig driver flags rustc passes that the wrapper must swallow:
        -fno-use-linker-plugin|-Wl,--dynamicbase|-Wl,--disable-auto-image-base|-Wl,--high-entropy-va|-nodefaultlibs|-nostartfiles|-no-pie|-lgcc_eh|-l:libpthread.a|-lmsvcrt|-lmingwex|-lmingw32|-lgcc|-l:libgcc_eh.a|-l:crt2.o)
            ;;
        *) args+=("$a") ;;
    esac
done

exec "$ZIG" c++ --target=x86_64-windows-gnu -fexceptions "${args[@]}"
